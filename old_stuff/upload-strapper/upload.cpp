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
#include <time.h>
#include <string.h>

int sergetc(int fd);
int serputc(int fd, char c);
void serflush(int fd);
void monitor(int fd);
uint8_t has_data(int fd);

#define msleep(x) usleep((x) * 1000)

#define RESET 0xCF
#define BOOT 0xCB

#define IOSSDATALAT    _IOW('T', 0, unsigned long)
#define IOSSIOSPEED    _IOW('T', 2, speed_t)

char port_def[] ="/dev/cu.usbserial-M68KV1BB";
const int pin_rts = TIOCM_RTS;

void command(int fd, uint8_t instr) {
    
    ioctl(fd, TIOCMBIS, &pin_rts); // assert RTS
    usleep(100*1000);
    serputc(fd, instr);
    usleep(100*1000);
    ioctl(fd, TIOCMBIC, &pin_rts); // deassert RTS
    usleep(100*1000);

}

int main (int argc, char ** argv) {
    // vars n shit bitches
    char * filename = 0;
    char * port = (char*)&port_def;
    
    bool no_terminal = false;
    bool verbose = false;
    bool term_only = false;
    bool program_reset = false;
    
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

                }
            } else
                filename = arg;
        }
    }
    
    int fd, flags;

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
    
    speed_t speed = 28000; // Set 38400 baud
    
    ioctl(fd, IOSSIOSPEED, &speed);
    ioctl(fd, TIOCMBIC, &pin_rts); // deassert RTS
    
    if (term_only) {
        monitor(fd);
        return 0;
    }
    
    if (program_reset) {
    	printf("Resetting program...\n");
    	
    	command(fd, RESET);
        serflush(fd);
        command(fd, BOOT);
        
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
    
    uint8_t *data = (uint8_t*)malloc(size);
    uint8_t *readback = (uint8_t*)malloc(size);
    
    if (!data) {
        printf("Failed to allocate memory\n");
        return 1;
    }
    
    if (fread(data, 1, size, in) != size) {
        printf("Did not read entire file\n");
        return 1;
    }
    
    fclose(in);
    
    ////////////////////////////////
    
    printf("Sending reboot command...\n");
    
    bool programmedOK = false;

    while (!programmedOK) {
        programmedOK = true;
        
        // reset board
        command(fd, RESET);
        serflush(fd);
        //sergetc(fd); // first junk char from uart

        printf("Uploading...\n");
        
        char ch;
        int rbi = 0;
        int i;
        int wrSz = 16;
        
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
                msleep(110);
                //sleep because the ftdi driver is terrible
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
        
        clock_t start = clock();
        
        while (1) {
            if (has_data(fd))
            	if(read(fd, &ch, 1) == 1) {
					readback[rbi] = ch;
					start = clock();
					rbi++;
            	} else {
                	printf("Serial read error.\n");
                	return 1;
                }
            
            if ((((float)(clock() - start)) / CLOCKS_PER_SEC) > 0.2F) 
            	break;
        }
        
        if (size/10 > rbi) {
            printf("Only %d bytes read. Manually reset board and try again.\n\n", rbi);
            close(fd);
            return 1;
        }
        
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
            // reset board
            command(fd, RESET);
            
            // tell board to reset position
            
            printf("Verification FAILED with %d errors. Retrying.\n\n",errors);
        } else {
            printf("Upload verified.\nBooting program.\n\n");
            command(fd, BOOT);
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
        printf("Serial read failed %d\n",ret);
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
