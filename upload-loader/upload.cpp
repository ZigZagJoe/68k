#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <sys/poll.h>
#include <ctype.h>
#include <sys/time.h>
#include <string.h>

#include <loader.h>

#define IOSSDATALAT    _IOW('T', 0, unsigned long)
#define IOSSIOSPEED    _IOW('T', 2, speed_t)

#define ROL(num, bits) (((num) << (bits)) | ((num) >> (32 - (bits))))
#define msleep(x) usleep((x) * 1000)


// default port
char port_def[] ="/dev/cu.usbserial-M68KV1BB";

const int pin_rts = TIOCM_RTS;

// serial functions
uint8_t readb();
uint16_t readw();
uint32_t readl();

void putl(uint32_t i);
void putwd(uint32_t i);

int sergetc(int fd);
int serputc(int fd, char c);
void serflush(int fd);
uint8_t has_data(int fd);

void command(int fd, uint8_t instr);

// execute a flag set command
bool SET_FLAG(const char * name, uint8_t value, uint32_t magic) ;

// large functions
int perform_dump(uint32_t addr, uint32_t len);
void monitor(int fd);

// general functions
uint64_t millis();
uint32_t crc_update (uint32_t inp, uint8_t v);
void hex_dump(uint8_t *array, uint32_t cnt, uint32_t baseaddr) ;

// s-rec info variables
extern uint32_t program_sz;
extern uint32_t entry_point;
extern uint8_t erased_sectors[SECTOR_COUNT];

// simulated system memory
uint8_t MEMORY[0x100000];

// file descriptor... getting lazy...
int fd;
char * filename = 0;
bool can_timeout = true;
/// MAIN CODE

void usage() { 
    printf(
        "\nUsage: upload-loader [options] file\n"
        "\n"
        "-t             only act as terminal emulator\n"
        "-n             do not enter terminal emulator mode after load\n"
        "-v             verbose verification error log\n"
        "-s             file to load is a motorola s-record\n"
        "-f             allow s-record to write flash\n"
        "-l             allow s-record to write loader (must also specify -f)\n"
        "-b             boot from entry point specified in s-record\n"
        "-x             s-record is in binary format from srec2srb\n"
        "-c             enable qcrc validation (implicit in -s)\n"
        "-p port        specify serial port (full path to device)\n"
        "-d addr[:len]  dump specified region of memory, in hex or dec.\n"
        "\n"
    ); 
}

uint32_t parse_num(char * c) {
    uint32_t ret;
    
    if (c[0] == '$') {
        c++;
        if (!sscanf(c," %x",&ret)) {
            printf("Argument error parsing %s\n", c);
            exit(1);
        }
        return ret;
    }

    if (c[0] == '0' && c[1] == 'x') {
        c+=2;
        if (!sscanf(c," %x",&ret)) {
            printf("Argument error parsing %s\n", c);
            exit(1);
        }
        return ret;
    }

    if (!sscanf(c," %u",&ret)) {
            printf("Argument error parsing %s\n", c);
            exit(1);
    }
    return ret;
}


int main (int argc, char ** argv) {
    if (argc == 1) {
        usage();
        return 1;
    }
    
    // vars n shit bitches
    char * port = (char*)&port_def;
    
    // program flags
    bool no_terminal = false;
    bool verbose = false;
    bool term_only = false;
    bool use_qcrc = false;
    bool do_dump = false;
    
    // flags
    bool boot_srec = false;
    bool flash_wr = false;
    bool loader_wr = false;
    bool set_addr = false;
    bool bin_srec = false;
    
    bool srec = false;
    
    // should allow setting of this
    int BAUD_RATE = 28000;
    
    uint32_t addr = 0;
    uint32_t len = 256;
        
    if (argc > 1) {
        char * arg;
        int dash = 0;
        
        char *dumparg = 0;
        
        for (int ac = 1; ac < argc; ac++) {
            arg=argv[ac];
            dash = 0;
            while (*arg == '-') { arg++; dash = 1; }
            
            if (dash) {
                switch (*arg) {
                    case 'h': usage(); return 1;
                    case 't': term_only = true; break;
                    case 'n': no_terminal = true; break;
                    case 'v': verbose = true; break;
                    case 's': srec = true; break;
                    case 'l': loader_wr = true; break;
                    case 'f': flash_wr = true; break;
                    case 'b': boot_srec = true; break;
                    case 'x': bin_srec = true; break;
                    case 'c': use_qcrc = true; break;
                    case 'd': 
                                do_dump = true; 
                                if (ac + 1 == argc) {
                                    printf("Missing argument to -d\n");
                                    return 1;
                                }
                                dumparg = argv[++ac];
                                break;
                       
                    case 'p': 
                              if (ac + 1 == argc) {
                                    printf("Missing argument to -p\n");
                                    return 1;
                              }
                              
                              port = argv[++ac];
                              printf("Port set to %s\n", port);
                              break;  
                }
            } else
                filename = arg;
        }
        
        if (dumparg != 0) {
            char *addr_s = dumparg;
            char *len_s = 0;
            
            for (int i = 0; i < 100; i++) {
                char ch = dumparg[i];
                if (!(ch >= '0' && ch <= '9') && !(ch >= 'a' && ch <= 'f') && !(ch >= 'A' && ch <= 'F') && ch != 'x' && ch != '$') {
                    if (ch == 0) 
                        break;
                        
                    len_s = addr_s + i + 1;
                    dumparg[i] = 0; 
                }
            }
                
            addr = parse_num(addr_s);
            
            if (len_s)
                len = parse_num (len_s);
            
                
            if (len == 0 || len > 768 * 1024) {
                printf("Bad length argument: value of %d is invalid.\n",len);
                return 1;
            }
            
            if (addr == 0 || addr > 768 * 1024) {
                printf("Bad address argument: value of %d is invalid.\n",addr);
                return 1;
            }
        }
    }
    
    printf("Open port %s ... ", port);
    fd = open(port,O_RDWR);
    
    if (fd <= 0) {
        printf("FAILED!\n");
        return 1;
    }
    
    printf("OK\n");
    
    // remove any pending characters
    serflush(fd);
    
    unsigned long mics = 1UL; // set latency to 1 microsecond
    ioctl(fd, IOSSDATALAT, &mics);
    
    speed_t speed = BAUD_RATE; // Set baud
    
    // since the FTDI driver is shit, hard rate the data rate 
    // to the max possible by baud rate (in chunks of 512)
    int BAUD_DELAY = (int)(1000.0F / (BAUD_RATE / 9) * 512);
    
    ioctl(fd, IOSSIOSPEED, &speed);
    ioctl(fd, TIOCMBIC, &pin_rts); // deassert RTS
    
    if (term_only) {
        monitor(fd);
        return 0;
    }
  
    if (do_dump) {  
        return perform_dump(addr, len);
    }
    
    if (filename == 0) {
        printf("Missing filename.\n");
        return 1;
    }
    
    FILE *in = fopen(filename,"rb");
    
    if (!in) {
        printf("Failed to open %s\n",filename);
        return 1;
    }
    
    // get file size
    fseek (in, 0, SEEK_END);   // non-portable
    int size = ftell (in);
    rewind(in);
    
    fprintf(stderr, "File size: %d\n", size);
    
    // allocate buffers
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

    // set flags as relevant
    uint8_t flags = (flash_wr ? ALLOW_FLASH : 0) +
                    (loader_wr ? ALLOW_LOADER : 0) +
                    (bin_srec ? BINARY_SREC : 0);
    
    // clear simulated memory
    memset(MEMORY, 0, 0x100000);
         
    if (srec) {
        // add trailing null
        data[size++] = 0;
        
        printf("Validating S-record... ");
        uint8_t ret = parseSREC(data, size+1, flags, true);
        
        if (ret) {
            printf("\nFailed to validate S-record. Ret: ");
            for (int i = 7; i >= 0; i--) {
                printf ("%c",((ret >> i) & 0x1) + '0');
                if (i == 4) printf(" ");
            }
        
            printf("\n");

            if (ret & BAD_HEX_CHAR || ret & FORMAT_ERROR)
                printf("\nVerify that the file is not corrupt.\n");
                
            printf("\nProgramming aborted.\n"); 
            return 1;
        } else 
            printf("OK.\n");
        
        if (boot_srec && entry_point == 0) {
            printf("Error: -b flag present, but S-record does not specify an entry point. Aborting!\n");
            return 1;
        }
        
        printf("Payload size: %d\n",program_sz);
        use_qcrc  = true;
        set_addr = true;
        
        // a loader write will be executed: do a sanity check
        if (loader_wr && erased_sectors[0]) {
            printf("\n*** Loader sector WILL be cleared. ***\n");
            
            uint32_t sp = 0, pc = 0;
            uint32_t a = 0x80000;
            
            // read stack pointer from simulated memory, big endian
            sp |= MEMORY[a++];
            sp <<= 8;
            sp |= MEMORY[a++];
            sp <<= 8;
            sp |= MEMORY[a++];
            sp <<= 8;
            sp |= MEMORY[a++];
            
            // read program counter from simulated memory, big endian
            pc |= MEMORY[a++];
            pc <<= 8;
            pc |= MEMORY[a++];
            pc <<= 8;
            pc |= MEMORY[a++];
            pc <<= 8;
            pc |= MEMORY[a++];
            
            printf("SP: 0x%06X  PC: 0x%06X\n",sp,pc);
            
            if (sp > FLASH_START) {
                printf("\nERROR: SP is not located in RAM. Aborting!\n");
                return 1;
            }
            
            if (pc < FLASH_START || pc >= IO_START) {
                printf("\nERROR: PC is not located in FLASH. Aborting!\n");
                return 1;
            }
 
            if (!erased_sectors[ADDR_TO_SECTOR(pc)]) {
                printf("\nERROR: PC is located in a sector that was not written to. Aborting!\n");
                return 1;
            }
     
            if (loader_wr && flash_wr) {
                printf("\n###################################################################\n");
                printf("# WARNING: Loader write and Flash write have both been enabled.   #\n");
                printf("# At least one write to the bootloader sector will be executed.   #\n");
                printf("# The stack pointer and program counter values appear to be sane. #\n");
                printf("#                                                                 #\n");
                printf("# Please confirm that you want to update the bootloader sector    #\n");
                printf("# by pressing SHIFT+1, then enter. Press any other key to abort.  #\n");
                printf("###################################################################\n\n> ");
                
                if (getchar() != '!') {
                    printf("\nAborting!\n");
                    return 1;
                }
        
                printf("\nContinuing with programming.\n");
            }
        }
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
        
        if (set_addr) {
            uint32_t load_addr = (0x80000 - 4096 - size) & ~1; // aligned load address with just enough room to load entire lower RAM
            
            printf("Setting write address to 0x%x... ", load_addr);
            fflush(stdout);
            
            command(fd, CMD_SET_ADDR);
            
            uint32_t intro = readl();
            
            if (intro != ADDR_MAGIC) {
                printf("Sync error setting write address: got %x\n", intro);
                return 1;
            }
            putl(load_addr);
            uint32_t addr_rb = readl();
            uint16_t tail = readw();
            
            if (tail != ADDR_TAIL_MAGIC) {
                printf("Sync error getting addrSet tail: got %x\n", tail);
                return 1;
            }
            
            if (addr_rb != load_addr) {
                printf("Address verification failed (got %x, expected %x)\nRetrying.", addr_rb, load_addr);
                continue;
            }
            
            printf("OK!\n");
        }

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
                // sleep because the ftdi driver is terrible, so we hard rate limit
            }
            
			while (has_data(fd)) 
				if (read(fd, &ch, 1) == 1) {
					readback[rbi++] = ch;
				} else {
					printf("Serial read error.\n");
					return 1;
				}
                
        }
        
        // done!
        printf("%6.2f %%\n\n",((float)100));
        
        uint64_t start = millis();
        
        // read additional bytes while they are available, wait up to 250ms for data 
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
            printf("Only %d bytes read. Reset board manually and try again.\n\n", rbi);
            close(fd);
            return 1;
        }
        
        // validate CRC, if enabled
        if (use_qcrc) {
            command(fd, CMD_QCRC);
        
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
        }
        
        // validate readback
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
        
        // okay, errors? otherwise boot.
        if (errors) {
            printf("Verification FAILED with %d errors. Retrying.\n\n",errors);
        } else {
            printf("Upload echo verified.\n\n");
            
            if (srec) {

                // apply relevant flags
                
                if (boot_srec && !SET_FLAG("run from SREC", CMD_SET_BOOT, BOOT_MAGIC))
                    return 1;
                    
                if (flash_wr   && !SET_FLAG("flash write", CMD_SET_FLWR, FLWR_MAGIC))
                    return 1;
                    
                if (loader_wr && !SET_FLAG("loader write", CMD_SET_LDWR, LDWR_MAGIC))
                    return 1;
                   
                if (bin_srec && !SET_FLAG("binary srec", CMD_SET_BINSR, BINSREC_MAGIC)) 
                    return 1;
                    
                printf("Programming SREC");
                fflush(stdout);
                
                // execute command
                command(fd, CMD_SREC);
                
                uint32_t ok = readl();
                
                if (ok != 0xD0E881CC) {
                    printf("\nSync error executing srec (bad greeting). Reset board and try again.\nGot %08X, expected %08X\n\n", ok, 0xD0E881CC);
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
                
                // this can take a while, so disable serial timeouts
                can_timeout = false;
                uint32_t c0de = readw();
                can_timeout = true;
                
                uint64_t end = millis();
                
                if (c0de != 0xC0DE) {
                    printf("\nSync error executing srec (bad c0de). Reset board and try again.\nGot %04X, expected %04X\n\n", c0de, 0xC0DE);
                    return 1;
                }
                
                uint8_t ret_code = readb();
                uint16_t tail_magic = readw();
                
                if (tail_magic != 0xEF00) {
                    printf("\nSync error executing srec (bad tail). Reset board and try again.\nGot %04X, expected %04X\n\n", tail_magic, 0xEF00);
                    return 1;
                }    
                
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
                
                if (!boot_srec) 
                    no_terminal = true;
                
                break;
            } else {
                printf("Booting program.\n\n");
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


int perform_dump(uint32_t addr, uint32_t len) {
    FILE *out = fopen(filename,"w");
    
    if (!out && filename || (!filename && len >= 5000)) {
        printf("Failed to open %s.\n",filename);
    }

    printf("Performing dump of %d bytes starting at 0x%06X\n",len, addr);
    printf("Resetting board...\n");
    command(fd, CMD_RESET);
    serflush(fd);
    printf("Sending dump command... \n");
    command(fd, CMD_DUMP);
    
    uint32_t greeting = readw();
    
    if (greeting != DUMP_GREET_MAGIC) {
        printf("Sync error in greeting. Got 0x%x\n", greeting);
        return 1;
    }
    
    putl(addr);
    putl(len);
    
    uint32_t addr_rb, len_rb;
    len_rb = readl();
    addr_rb = readl();
    
    if (addr_rb != addr || len_rb != len) {
        putl(0);
        printf("Bad readback of length/address! Dump aborted...\n");
        printf("Address: read 0x%06X, expected 0x%06X\n",addr_rb, addr);
        printf("Length:  read 0x%06X, expected 0x%06X\n",len_rb, len);
        return 1;
    }
    
    putwd(DUMP_START_MAGIC);
    
    int crc = 0xDEADC0DE;
    char ch;
    
    uint64_t start = millis();
    uint64_t lastByte = start;
    uint32_t sz = 0;
    
    uint8_t *dump = (uint8_t*)malloc(len);
    if (!dump) {
        printf("Failed to allocate memory\n");
        return 1;
    }

    printf("\n");
    
    while (sz < len) {
        if (has_data(fd)) {
            if(read(fd, &ch, 1) == 1) {
                
                if (out) fputc(ch, out);
                
                crc = crc_update(crc, ch);
                
                lastByte = millis();
                dump[sz++] = ch;
                
                if (sz % 32 == 0) {
                    printf("%6.2f %%\x8\x8\x8\x8\x8\x8\x8\x8",((float)sz * 100) / len);
                    fflush(stdout);
                }
            } else {
                printf("Serial read error.\n");
                return 1;
            }
        } else usleep(10);
        
        if ((millis() - lastByte) > 250) {
            printf("Error: timeout in dump (no data for 250ms)\n");
            return 1;
        }
    }
             
    uint64_t end = millis();
    
    printf("%6.2f %%\n\n",((float)100));					 
         
    uint32_t rem_crc = readl();
    uint16_t tail = readw();
    
    if (tail != DUMP_TAIL_MAGIC) {
        printf("Sync error in tail. Got 0x%x\n", tail);
        return 1;
    }
    
    printf("Read %d bytes in %d ms\n",sz, end-start);
    // (%3.2f bytes/s) ((float)sz) / (((float)(end-start))/ 1000)
    
    if (crc != rem_crc) {
        printf("ERROR: CRC MISSMATCH!\n");
        printf("Local CRC:  0x%08X\n",crc);
        printf("Remote CRC: 0x%08X\n",rem_crc);
        return 1;
    }

    printf("CRC validated as 0x%08X\n",crc);
    printf("Dump completed successfully.\n");
    
    if (len < 5000)
        hex_dump(dump, len, addr);
    
    if (out) fclose(out);
    return 0;
}

void hex_dump(uint8_t *array, uint32_t cnt, uint32_t baseaddr) {
    int c = 0;
    char ascii[17];
    ascii[16] = 0;
    
    printf("\n%8X   ", baseaddr);
   
    for (uint32_t i = 0; i < cnt; i++) {
        uint8_t b = array[i];
        
        ascii[c] = (b > 31 & b < 127) ? b : '.';
        
        printf("%02X ",b);

        if (++c == 16) {
            printf("  %s\n",ascii);
            if ((i+1) < cnt)
                printf("%8X   ", baseaddr+i);
            c = 0;
        }
    }
    
    if (c && c < 15) {
        while (c++ < 16)
            printf("   ");
        printf("  %s\n",ascii);
    } else printf("\n");
}

// perform a flag-set command (returns a magic value)
bool SET_FLAG(const char * name, uint8_t value, uint32_t magic) {
    printf("Setting %s... ",name);
    fflush(stdout);

    command(fd, value);
    uint32_t ok = readl();

    if (ok != magic) {
        printf("Sync error setting %s. Reset board and try again.\nGot %08X, expected %08X\n\n", name, ok, magic);
        return 0;
    }

    printf("OK.\n");
    return 1;
}

uint8_t has_data(int fd) {
	struct pollfd poll_str;
    poll_str.fd = fd;
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
               printf ("[ʘx%02X]", ch&0xFF);
               // fprintf(stderr, "%d\n",ch);
            }
        }
        
        fflush(stdout);
        msleep(10);
    }
}

int serputc(int fd, char c) {
   // printf("xmit %02hhx\n",c);
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
    uint64_t start = millis();
    uint64_t t;
    
    while(!has_data(fd)) {
        t = millis();
        if (can_timeout && (t-start) > 2000) {
            printf("Timeout occurred in sergetc(). Ensure bootloader supports command.\n");
            exit(1);
        }
        usleep(10);
    }
        
    char ch;
    int ret = read(fd, &ch, 1);
    if (ret == -1) {
        printf("\n *** Serial read failed %d ***\n",ret);
        exit(1);
    }
    
   // printf("rec  %02hhx\n",ch);
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


uint32_t crc_update (uint32_t inp, uint8_t v) {
	return ROL(inp ^ v, 1);
}

void command(int fd, uint8_t instr) {
    ioctl(fd, TIOCMBIS, &pin_rts); // assert RTS
    usleep(50*1000);
    
    if (instr == CMD_RESET) { // send multiple to ensure we enter bootloader mode
        for (int i = 0; i < 8; i++) {
            serputc(fd, instr);
            usleep(25*1000);
        }
    } else {
        serputc(fd, instr);
        usleep(50*1000);
    }
    
    ioctl(fd, TIOCMBIC, &pin_rts); // deassert RTS
}

void putl(uint32_t i) {
    serputc(fd, (i >> 24) & 0xFF);
    serputc(fd, (i >> 16) & 0xFF);
    serputc(fd, (i >>  8) & 0xFF);
    serputc(fd, (i      ) & 0xFF);
}

void putwd(uint32_t i) {
    serputc(fd, (i >>  8) & 0xFF);
    serputc(fd, (i      ) & 0xFF);
}

uint8_t readb() {
    return sergetc(fd);
}

uint16_t readw() {
    return (((uint16_t)readb()) << 8) | readb();
}

uint32_t readl() {
    return (((uint32_t)readw()) << 16) | readw();
}

uint64_t millis() {
    struct timeval tv;

    gettimeofday(&tv, NULL);
    
    return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}
