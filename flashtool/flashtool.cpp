#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>

typedef unsigned char byte;

// main functions
int fulldump(int fd, char * filename);
int program_chip(int fd, char * filename, bool erase_only);
int hexdump(int fd, uint32_t addr_start, uint32_t dump_cnt);

void serprintf(int fd, const char *fmt, ... );
int sergetc(int fd);
int serputc(int fd, char c);
void serflush(int fd);

// find the next block of 0xFF
int find_next_block(byte * data, int addr, int size, int maxchunk);

uint16_t crc16_update(uint16_t crc, uint8_t a);

enum {
    READY = 'A', // a
    OKAY,        // b
    UNKNOWN_CMD, // c
    BAD_INPUT,   // d
    CRC_ERROR,   // e
    WRITE_ERROR, // f
    TIMEOUT,     // g
    ERASE_FAIL   // h
};

int main (int argc, char ** argv) {
    // vars n shit bitches
    char * filename = 0;
    int fd;

    char port_def[] ="/dev/cu.usbmodem80981";
    
    char * port = (char*)&port_def;
    bool do_program = false;
    bool do_erase = false;
    bool do_fulldump = false;
    
    int addr_start = 0;
    int dump_cnt = 512;
    
    // parse arguments
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
                    case 'p': do_program = true; break;
                    case 'e': do_erase = true; break;
                    case 'd': do_fulldump = true; break;
                    case 's':
                        if (++ac == argc) {
                            printf("missing argument\n");
                            return 1;
                        }
                        
                        arg=argv[ac];
                        
                        if (arg[0] == '0' && arg[1] == 'x') {
                            if (sscanf(arg,"0x%x",&addr_start) == 0) {
                                printf("bad hex\n");
                                return 1;
                            }
                        } else
                            addr_start=atoi(arg);
                        
                        break;
                    case 'c':
                        if (++ac == argc) {
                            printf("missing argument\n");
                            return 1;
                        }
                        
                        arg=argv[ac];
                        
                        if (arg[0] == '0' && arg[1] == 'x') {
                            if (sscanf(arg,"0x%x",&dump_cnt) == 0) {
                                printf("bad hex\n");
                                return 1;
                            }
                        } else
                            dump_cnt=atoi(arg);
                        
                        break;
                }
            } else
                filename = arg;
        }
    }
    
    if (do_program && filename == 0) {
        printf("Missing filename to program\n");
        return 1;
    }

    printf("Open serial... ");
    fd = open(port,O_RDWR | O_NDELAY);
    printf("OK\n");
   
    if (fd <= 0) {
        printf("Failed to open %s\n");
        return 1;
    }
    
    // must send first to be able to read
    serputc(fd,'H');
    
    // remove any pending characters
    serflush(fd);
    
    // unset nodelay
    int flags = fcntl(fd, F_GETFL);
    flags &= ~O_NDELAY;
    fcntl(fd,F_SETFL,flags);
    
    printf("Synchronizing... ");
    
    serputc(fd,'H');
    
    // ensure we are in a known state
    if (sergetc(fd) != 'T' || sergetc(fd) != OKAY|| sergetc(fd) != READY) {
        printf("Sync error\n");
        return 0;
    }
    
    printf("OK\n");
    
    int code = 0;
    
    // do whatever man
    if (do_program || do_erase) {
        code = program_chip(fd, filename, do_erase);
    } else if (do_fulldump) {
        code = fulldump(fd, filename);
    } else
        code = hexdump(fd, addr_start, dump_cnt);
        
    close(fd);
    return code;
}

int hexdump(int fd, uint32_t addr_start, uint32_t dump_cnt) {
    printf("\n   Dump of %d bytes starting at 0x%06X\n",dump_cnt,addr_start);
    serprintf(fd, "R%06X%06X",addr_start, dump_cnt);
    
    int c;
    
    while ((c = sergetc(fd)) > 4)
        putchar(c);
    
    if (c == 4) {
        // remove OKAY and READY chars
        sergetc(fd);
        sergetc(fd);
    } else if (c < 0) {
        printf("Error while reading\n");
        return 1;
    }
    
    return 0;
}

int fulldump(int fd, char * filename) {
    int c; // temp char
    
    if (!filename) {
        printf("No filename specified");
        return 1;
    }
    
    FILE *in = fopen(filename,"wb");
    if (!in) {
        printf("Failed to open %s\n",filename);
        return 1;
    }

    serputc(fd,'A');
    
    if ((c = sergetc(fd)) != OKAY) {
        printf("Failed to begin readout (%c)\n",c);
        return 1;
    }
    
    printf("Readout in progress: ");
    
    for (uint32_t addr = 0; addr < 0x80000; addr++) {
        if ((addr % 16) == 0) {
            printf("%6.2f %%",((float)addr * 100) / 0x80000);
            for (int i = 0 ; i < 8; i++) putchar(8);
        }
        
        fputc(sergetc(fd),in);
    }
    
    fclose(in);
    
    if ((c = sergetc(fd)) != OKAY) {
        printf("failure (%c)\n",c);
        return 1;
    }
    
    printf("\nOK.   \n");
    
    if ((c = sergetc(fd)) != READY) {
        printf("Warning: Out of sync\n");
        return 0;
    }
    
    return 0;
}


int program_chip(int fd, char * filename, bool erase_only) {
    if (!filename) {
        printf("No filename specified");
        return 1;
    }
    
    int c; // temp char
    
    printf("Chip erase... ");
    
    serputc(fd,'E');
    
    if ((c = sergetc(fd)) != OKAY) {
        printf("failure (%c)\n",c);
        return 1;
    } else
        printf("OK\n");
    
    if ((c = sergetc(fd)) != READY) {
        printf("Out of sync\n");
        return 1;
    }
    
    if (erase_only) return 0;
    
    FILE *in = fopen(filename,"rb");
    if (!in) {
        printf("Failed to open %s\n",filename);
        return 1;
    }
    
    fseek (in, 0, SEEK_END);   // non-portable
    int size = ftell (in);
    rewind(in);
    
    fprintf(stderr, "Image size: %d\n", size);
    
    byte *data = (byte*)malloc(size);
    if (!data) {
        printf("Failed to allocate memory\n");
        return 1;
    }
    
    if (fread(data, 1, size, in) != size) {
        printf("Did not read entire file\n");
        return 1;
    }

    printf("\nAddress  Size  CRC\n");
    
    int addr = 0;
    
    while(1) {
        while (addr<size && data[addr] == 0xFF) addr++;
        if (addr == size) // EOF
            break;
        
        int lb = find_next_block(data,addr+1, size, 1024);
        int sz =  lb-addr;
        
        printf("%6X  %4X ",addr,sz);
        
        uint16_t crc = 0;
        
        // header
        serprintf(fd, "D%06X%04X",addr,sz);
        
        // emit bytes
        while (addr != lb) {
            serprintf(fd,"%02X", data[addr]);
            crc = crc16_update(crc, data[addr]);
            addr++;
        }
        
        // checksum
        serprintf(fd, "%04X", crc);
        printf("  %4X    ", crc);
        
        if ((c = sergetc(fd)) != OKAY) {
            printf("FAIL (%c - %d)\n",c,c);
            return 1;
        } else
            printf("OK\n");
        
        if ((c = sergetc(fd)) != READY) {
            printf("Out of sync\n");
            return 1;
        }
        //addr = lb;
    }
    
    printf("\nProgramming completed!\n");
    fclose(in);
    
    return 0;
}

// find next block of 0xFF data
int find_next_block(byte * data, int addr, int size, int maxchunk) {
    int chz = 1;
    while (addr < size) {
        if (data[addr] == 0xFF) {
            int noskip = 0;
            // ensure this is not just a single 0xFF in a bunch of other data
            for (int i = 0; i < 8 && (addr+i) < size; i++) {
                if (data[addr+i] != 0xFF) {
                    noskip = 1;
                    break;
                }
            }
            
            if (!noskip)
                break;
        }
        
        addr++;
        chz++;
        
        if (chz == maxchunk)
            break;
    }
    
    return addr;
}

uint16_t crc16_update(uint16_t crc, uint8_t a) {
    int i;
    
    crc ^= a;
    for (i = 0; i < 8; ++i)
    {
        if (crc & 1)
            crc = (crc >> 1) ^ 0xA001;
        else
            crc = (crc >> 1);
    }
    
    return crc;
}

int serputc(int fd, char c) {
    if (write(fd, &c, 1) != 1) {
        printf("Serial write failed");
        exit(1);
    }
    return 1;
}

// empty the entire input queue (while nodelay is active)
// so we should be in sync
void serflush(int fd) {
    char ch;
    sleep(1);
    while (read(fd, &ch, 1) > 0);
}


int sergetc(int fd) {
    char ch;
    if (1 != read(fd, &ch, 1)) {
        printf("Serial read failed\n");
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