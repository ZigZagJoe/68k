#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <sys/poll.h>
#include <ctype.h>
#include <wchar.h>
#include <sys/time.h>
#include <string.h>

int sergetc(int fd);
int serputc(int fd, char c);
void serflush(int fd);
void monitor(int fd);
uint8_t has_data(int fd);

#define msleep(x) usleep((x) * 1000)

#define CMD_RESET 0xCF
#define CMD_BOOT 0xCB
#define CMD_GETCRC 0xCC
#define CMD_HIRAM 0xCE
#define CMD_SREC 0xCD
#define CMD_SET_BOOT 0xC1
#define CMD_SET_FLWR 0xC2
#define CMD_SET_LDWR 0xC3

#define IOSSDATALAT    _IOW('T', 0, unsigned long)
#define IOSSIOSPEED    _IOW('T', 2, speed_t)

// flag bits
#define ALLOW_FLASH   (1)
#define ALLOW_LOADER  (2)


char port_def[] ="/dev/cu.usbserial-M68KV1BB";
const int pin_rts = TIOCM_RTS;

#define ROL(num, bits) (((num) << (bits)) | ((num) >> (32 - (bits))))

uint32_t crc_update (uint32_t inp, uint8_t v) {
	return ROL(inp ^ v, 1);
}

void command(int fd, uint8_t instr) {
    
    ioctl(fd, TIOCMBIS, &pin_rts); // assert RTS
    usleep(100*1000);
    serputc(fd, instr);
    usleep(100*1000);
    ioctl(fd, TIOCMBIC, &pin_rts); // deassert RTS
    usleep(100*1000);

}

int fd;

uint8_t readb() {
    return sergetc(fd);
}

uint16_t readw() {
    return (((uint16_t)readb()) << 8) | readb();
}

uint32_t readl() {
    return (((uint32_t)readw()) << 16) | readw();
}

uint8_t parseSREC(uint8_t * start, uint32_t len, uint8_t fl);


uint64_t millis() {
    struct timeval tv;

    gettimeofday(&tv, NULL);
    
    return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}


int main (int argc, char ** argv) {
 
    // vars n shit bitches
    char * filename = 0;
    char * port = (char*)&port_def;
    
    bool no_terminal = false;
    bool verbose = false;
    bool term_only = false;
    bool program_reset = false;
    
    bool set_boot_srec = false;
    bool go_hiram = false;
    bool srec = false;
    
    bool flash_wr = false;
    bool loader_wr = false;
    
    int BAUD_RATE = 28000;
    int BAUD_DELAY = 200;
    
    if (argc > 1) {
        char * arg;
        int dash = 0;
        for (int ac = 1; ac < argc; ac++) {
            arg=argv[ac];
            dash = 0;
            while (*arg == '-') { arg++; dash = 1; }
            
            if (dash) {
                switch (*arg) {
                    case 'h': printf("I PITY THE FOOL\n"); return 1;
                    case 't': term_only = true; break;
                    case 'n': no_terminal = true; break;
                    case 'v': verbose = true; break;
					case 'r': program_reset = true; break;
                    case 's': srec = true; break;
                    case 'l': loader_wr = true; break;
                    case 'f': flash_wr = true; break;
                }
            } else
                filename = arg;
        }
    }
    
    printf("Open serial... ");
    fd = open(port,O_RDWR);
    
    if (fd <= 0) {
        printf("Failed to open %s\n",port);
        return 1;
    }
    
    printf("OK\n");
    
    serflush(fd);
    
    /*// unset nodelay
     flags = fcntl(fd, F_GETFL);
     flags &= ~O_NDELAY;
     fcntl(fd,F_SETFL,flags);*/
    
    unsigned long mics = 1UL; // set latency to 1 microsecond
    ioctl(fd, IOSSDATALAT, &mics);
    
    speed_t speed = BAUD_RATE; // Set baud
    
    // since the FTDI driver is miserable, hard rate the data rate 
    // to the max possible by baud rate (in chunks of 512)
    BAUD_DELAY = (int)(1000.0F / (BAUD_RATE / 9) * 512);
    
    ioctl(fd, IOSSIOSPEED, &speed);
    ioctl(fd, TIOCMBIC, &pin_rts); // deassert RTS
    
    if (term_only) {
        monitor(fd);
        return 0;
    }
    
    if (program_reset) {
    	printf("Resetting program...\n");
    	
    	command(fd, CMD_RESET);
        serflush(fd);
        command(fd, CMD_BOOT);
        
        if (!no_terminal)
       		monitor(fd);
       
        return 0;
    }
  
    if (filename == 0) {
        printf("Missing filename to program\n");
        return 1;
    }
    
    FILE *in = fopen(filename,"rb");
    if (!in) {
        printf("Failed to open %s\n",filename);
        return 1;
    }
    
    fseek (in, 0, SEEK_END);   // non-portable
    int size = ftell (in);
    rewind(in);
    
    fprintf(stderr, "Program size: %d\n", size);
    
    uint8_t *data = (uint8_t*)malloc(size+1);
    uint8_t *readback = (uint8_t*)malloc(size+1);
    
    if (!data || !readback) {
        printf("Failed to allocate memory\n");
        return 1;
    }
    
    if (fread(data, 1, size, in) != size) {
        printf("Did not read entire file\n");
        return 1;
    }
    
    fclose(in);
    
    data[size] = 0;
    size++;

    uint8_t flags = 0;
    
    if (flash_wr)
        flags |= ALLOW_FLASH;
        
    if (loader_wr)
        flags |= ALLOW_LOADER;
            
    if (srec) {
        uint8_t ret = parseSREC(data, size, flags);
        if (ret) {
            printf("Failed to validate file as S-record.\n");
            printf("Error %x: ", ret);
            for (int i = 7; i >= 0; i--) {
                printf ("%c",((ret >> i) & 0x1) + '0');
                if (i == 4) printf(" ");
            }
        
            printf("\n");
            printf("Programming aborted.\n"); 
            return 1;
        }
        go_hiram = true;
        set_boot_srec = true;
    }
    
    ////////////////////////////////
    
    printf("Sending reboot command...\n");
    
    bool programmedOK = false;

    while (!programmedOK) {
        programmedOK = true;
        
        // reset board
        command(fd, CMD_RESET);
        msleep(200);
        serflush(fd);
        
        if (go_hiram) {
        
            printf("Setting HIRAM... ");
            fflush(stdout);
        
            command(fd, CMD_HIRAM);
            uint32_t ok = readl();
        
            if (ok != 0xCE110C00) {
                printf("Sync error setting HIRAM. Reset board and try again.\nGot %08X, expected %08X\n\n", ok, 0xCE110C00);
                return 1;
            }
        
            printf("OK.\n");
        
        }
        
        //sergetc(fd); // first junk char from uart

        printf("Uploading...\n");
        
        char ch;
        int rbi = 0;
        int i;
        int wrSz = 16;
        
        int expct_crc = 0xDEADC0DE;
        for (i = 0; i < size; i ++)
            expct_crc = crc_update(expct_crc,data[i]);
        
        for (i = 0; i < size; i += wrSz) {
           // serputc(fd, data[i]);
           if ((size-i) < 16){
          	    wrSz = (size-i);
          	   // printf("final write of %d bytes",wrSz);
            }
           
            if (write(fd, &data[i], wrSz) != wrSz) {
        		printf("Serial write failed!");
       			exit(1);
   			}
    
            if (i % 512 == 0) {
                printf("%6.2f %%\x8\x8\x8\x8\x8\x8\x8\x8",((float)i * 100) / size);
                fflush(stdout);
                msleep(BAUD_DELAY);
                // sleep because the ftdi driver is terrible, so hard rate limit
            }
            
			while (has_data(fd)) 
				if (read(fd, &ch, 1) == 1) {
					readback[rbi++] = ch;
				} else {
					printf("Serial read error.\n");
					return 1;
				}
                
        }
        
        printf("%6.2f %%\n\n",((float)100));
        
        uint64_t start = millis();
        
        while (1) {
            if (has_data(fd))
            	if(read(fd, &ch, 1) == 1) {
					readback[rbi] = ch;
					start = millis();
					rbi++;
            	} else {
                	printf("Serial read error.\n");
                	return 1;
                }
            
            if ((millis() - start) > 250 || (rbi > size*2)) 
            	break;
        }
        
        if (size/10 > rbi) {
            printf("Only %d bytes read. Reset board and try again.\n\n", rbi);
            close(fd);
            return 1;
        }
        
        command(fd, CMD_GETCRC);
        
        uint16_t intro = readw();
        uint32_t crc = readl();
        uint8_t nul = readb();
        
        if (intro != 0xFCAC || nul != 0) { 
            printf("Sync error reading CRC. Reset board and try again.\n");
            printf("Error: %s\n\n",(nul != 0)?"Intro invalid.":"Tail invalid."); 
            return 1;
        }
        
        if (crc != expct_crc) {
            printf("CRC ERROR: Got %08X, expected %08X\n",crc,expct_crc);
            printf("Retrying.\n\n");
            continue;
        } else
            printf("CRC verified: 0x%08X\n", crc);
        
        int errors = 0;
               
        for (int i = 0; i < size; i++) {
            if (data[i] != readback[i]) {
                if (errors == 0 || verbose) {
                    printf("Mismatch at byte %d: expected %d, got %d\n",i, data[i],readback[i]);
                    programmedOK = false;
                }
                errors++;
            }
        }
        
        if (errors) {
            printf("Verification FAILED with %d errors. Retrying.\n\n",errors);
        } else {
            printf("Upload verified.\nBooting program.\n\n");
            if (srec) {
                printf("Setting boot from SREC... ");
                fflush(stdout);
        
                command(fd, CMD_SET_BOOT);
                uint32_t ok = readl();
    
                if (ok != 0xD0B07CDE) {
                    printf("Sync error setting boot from srec. Reset board and try again.\nGot %08X, expected %08X\n\n", ok, 0xD0B07CDE);
                    return 1;
                }
                
                printf("OK\n");
                
                if (flash_wr) {
                    printf("Setting flash write... ");
                    fflush(stdout);
        
                    command(fd, CMD_SET_FLWR);
                    ok = readl();
    
                    if (ok != 0xF1A5C0DE) {
                        printf("Sync error setting flash write. Reset board and try again.\nGot %08X, expected %08X\n\n", ok, 0xD0B07CDE);
                        return 1;
                    }
                
                    printf("OK\n");
                
                }
        
                if (loader_wr) {
                    printf("Setting loader write... ");
                    fflush(stdout);
        
                    command(fd, CMD_SET_LDWR);
                    ok = readl();
    
                    if (ok != 0x10ADC0DE) {
                        printf("Sync error setting loader write. Reset board and try again.\nGot %08X, expected %08X\n\n", ok, 0x10ADC0DE);
                        return 1;
                    }
                
                    printf("OK\n");
                }
                
                printf("Programming SREC");
                
                command(fd, CMD_SREC);
                
                uint32_t greet = readl();
                
                if (greet != 0xD0E881CC) {
                    printf("\nSync error executing srec (bad greeting). Reset board and try again.\nGot %08X, expected %08X\n\n", greet, 0xD0E881CC);
                    return 1;
                } else {
                    printf(" in progress... ");
                    fflush(stdout);
                }
                
                uint64_t started = millis();
                
                uint8_t wr_flags = readb();
    
                /*char ch;
                
                while ((ch = sergetc(fd)) != 0xC0) {
                    printf("%c",ch);
                    fflush(stdout);
                }*/
                
                uint32_t c0de = readw();
    
                if (c0de != 0xC0DE) {
                    printf("\nSync error executing srec (bad c0de). Reset board and try again.\nGot %08X, expected %08X\n\n", c0de, 0xC0DE);
                    return 1;
                }
                
                uint8_t ret_code = readb();
                
                uint16_t tail = readw();
                
                if (tail != 0xEF00) {
                    printf("\nSync error executing srec (bad tail). Reset board and try again.\nGot %08X, expected %08X\n\n", tail, 0xEF00);
                    return 1;
                }    
                
                uint64_t end = millis();
                
                if (ret_code) {
                    printf("Failed. Error %hhx: ", ret_code);
                    for (int i = 7; i >= 0; i--) {
                        printf ("%c",((ret_code >> i) & 0x1) + '0');
                        if (i == 4) printf(" ");
                    }
        
                    printf("\n\n");
                    return 1;
                }
              
                printf(" completed successfully in %d ms.\n\n", end-start);
                break;

            } else {
                command(fd, CMD_BOOT);
                break;
            }
        }
    }
    
    if (!no_terminal)
        monitor(fd);
    
    close(fd);
    
    return 0;
}

uint8_t has_data(int fd) {
	struct pollfd poll_str;
    poll_str.fd = fd; /* this is STDIN */
    poll_str.events = POLLIN;
    
    return poll(&poll_str, 1, 0);
}

void monitor(int fd) {
    // ensure nodelay is on
    int flags = fcntl(fd, F_GETFL);
    flags |= O_NDELAY;
    fcntl(fd,F_SETFL,flags);
    
    // turn off line buffering and local echo
    struct termios oldt;
    tcgetattr( STDIN_FILENO, &oldt );
    oldt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr( STDIN_FILENO, TCSANOW, &oldt );
    
    printf("###### Serial Terminal ##########################################\n");
    
    char ch;
    while (true) {
        if (has_data(0))
            serputc(fd,getchar());
        
        while (1 == read(fd, &ch, 1)) {
            if (isprint(ch) || ch == '\n') {
                putchar(ch);
            } else {
               // printf("ʘ");
               printf ("[0x%02X]", ch&0xFF);
               // fprintf(stderr, "%d\n",ch);
            }
        }
        
        fflush(stdout);
        msleep(10);
    }
}

int serputc(int fd, char c) {
    if (write(fd, &c, 1) != 1) {
        printf("Serial write failed");
        exit(1);
    }
    return 1;
}

// empty the entire input queue
// so we should be in sync
void serflush(int fd) {
    char ch;
    msleep(100);
    while (has_data(fd))
    	read(fd, &ch, 1);
}


int sergetc(int fd) {
    char ch;
    int ret;
    while ((ret = read(fd, &ch, 1)) == 0) msleep(10);
    if (ret == -1) {
        printf("\n *** Serial read failed %d ***\n",ret);
        exit(1);
    }
    return ch;
}

void serprintf(int fd, const char *fmt, ... ){
    char buf[256];
    va_list args;
    va_start (args, fmt );
    int l = vsnprintf(buf, 256, fmt, args);
    va_end (args);
    
    if (write(fd, &buf,l) != l) {
        printf("Failed to write entire buffer\n");
        exit (1);
    }
}
