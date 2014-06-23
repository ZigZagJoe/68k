#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>


// flag bits
#define ALLOW_FLASH   (1)
#define ALLOW_LOADER  (2)

// errno bits
#define BAD_HEX_CHAR  (1)
#define INVALID_WRITE (2)
#define FAILED_WRITE  (4)
#define FORMAT_ERROR  (8)
#define EARLY_EOF    (16)
#define INVALID_CHAR (32)
#define CRC_ERROR    (64)

// important addresses
#define FLASH_START 0x80000
#define IO_START    0xC0000
#define LOADER_END  0x80FFF

#define SECTOR_COUNT 64

// resolve address to flash sector
#define ADDR_TO_SECTOR(X) ((((uint32_t)X) & 0x3FFFF) >> 12)

#define RETURN_IF_ERROR()  if (errno) return errno

uint8_t * ptr;
uint32_t pos;
uint32_t sz;
uint8_t flags;
uint8_t errno;
uint8_t erased_sectors[SECTOR_COUNT];
uint8_t write_armed;
uint8_t chk;
uint32_t entry;

uint8_t MEMORY[0x100000];

uint8_t readch() {
    if (pos >= sz) {
        errno |= EARLY_EOF;
        return 0;
    }
    
    char ch = ptr[pos++];
    
    if ((ch < '0' || ch > 'z') && ch !='\n' && ch != '\r') {
        errno |= INVALID_CHAR;
        return 0;
    }
    
    return ch;
}

uint8_t getn() {
    char ch = readch();
    
    if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;
    if (ch >= '0' && ch <= '9')
        return ch - '0';
        
    errno |= BAD_HEX_CHAR;
    return 0;        
}

uint8_t getb() {
    uint8_t b = (getn() << 4) | getn();
    chk += b;
    return b;
}

uint32_t readAddr(uint8_t sz) {
    uint32_t i = 0;
    while (sz-- > 0)
        i = (i << 8) | getb();
    return i;
}

void flash_erase_sector(uint8_t sector) {
   
}

void do_flash_write(uint32_t addr, uint8_t byte) {
    uint8_t sector = ADDR_TO_SECTOR(addr);
    
#ifdef WR_DEBUG   
    printf("%X flWrite %X (sect %x)\n", addr, byte, sector);
#endif

    if (!erased_sectors[sector]) {
         printf("Flash erase sector %x\n", sector);
         erased_sectors[sector] = 1;
          
         if (write_armed)
            flash_erase_sector(sector);
    }
    
    if (!write_armed) 
        return;
    
    MEMORY[addr] = byte;
}

void do_ram_write(uint32_t addr, uint8_t byte) {

#ifdef WR_DEBUG   
    printf("%X rmWrite %X\n",addr,byte);
#endif
    
    if (!write_armed) 
        return;
     
    MEMORY[addr] = byte;
}

uint8_t write(uint32_t addr, uint8_t byte) {
    if (addr >= IO_START) {
        errno |= INVALID_WRITE;
        return 1;
    } else if (addr >= FLASH_START && addr < IO_START) {
        if (!(flags & ALLOW_FLASH)) {
            errno |= INVALID_WRITE;
            return 1;
        }
        
        if (addr < LOADER_END && !(flags & ALLOW_LOADER)) {
            errno |= INVALID_WRITE;
            return 1;
        }
        
        do_flash_write(addr, byte);
    } else 
        do_ram_write(addr, byte);
    
    if (write_armed && MEMORY[addr] != byte) {
        errno |= FAILED_WRITE;
        return 1;
    }
     
    return 0;
}

uint8_t parseSREC(bool armed) {
    pos = 0;
    errno = 0;
    entry = 0;
    
    write_armed = armed;
       
    char Sc; // char to hold 'S'
    uint8_t typ, len, file_crc, loc_crc;
    uint32_t address;
    
    uint16_t data_rec_cnt = 0;
    uint16_t rec_cnt = 0;

    // clear sector count array
    for (uint8_t i = 0; i < SECTOR_COUNT; i++)
        erased_sectors[i] = 0;    
    
    if (ptr[sz-1] != 0) {
        errno |= FORMAT_ERROR;
        printf("Record is missing trailing null (%hhd)\n", ptr[sz]);
        return errno;
    }
 
    // number of address bytes for each record type
    const uint8_t addr_len[] = {2,2,3,4,0,2,3,4,3,2};
    
    while(true) {
        rec_cnt++;
        
        Sc = readch();  // if not a S, bad news
        
        if (Sc != 'S') {
            errno |= FORMAT_ERROR;
            printf("Record did not start with S (%c)\n", Sc);
            break;
        }
        
        typ = readch() - '0'; // record type
        
#ifdef DEBUG
        printf ("Get record of type %x\n",typ);
#endif
        
        RETURN_IF_ERROR();
        
        if (typ > 9) {
            errno |= FORMAT_ERROR;
            printf("Record has type > 9 (%d)", typ);
            break;
        }
        
        chk = 0;              // reset checksum
        len = getb() - addr_len[typ] - 1;
        
#ifdef DEBUG
        printf("Record length: %x\n",len);
#endif
       
        address = readAddr(addr_len[typ]);
        
        RETURN_IF_ERROR();
        
        switch (typ) {
            case 3:
            case 2:
            case 1:
                data_rec_cnt++;
            case 0:
                for (int i = 0; i < len && !errno; i++) {
                    uint8_t byt = getb();
                    if (typ == 0) // don't do anything with the data, which should not be present
                   continue;
                
                    write(address, byt);
                    address++; 
                }
                break;
            case 5:
            case 6:
                if (address != data_rec_cnt) {
                    errno |= FORMAT_ERROR;
                    printf("Data record count missmatch (read %d, actual %d)\n",address,data_rec_cnt);
                }
                break;
            case 7:
            case 8:
            case 9:
                if (entry)
                    printf("Overwriting previous entry point!\n");
                    
                entry = address;
                printf("Set entry point to 0x%x\n",entry);
                break;
        }
        
        RETURN_IF_ERROR();
        
        if (len != 0 && (typ >= 4)) {
             errno |= FORMAT_ERROR;
             printf("Record has data in record that should not contain data!\n");
             return errno;
        }
        
        loc_crc = ~chk;
        file_crc = getb();
        
        if (loc_crc != file_crc) {
            errno |= CRC_ERROR;
            printf("CRC error in record on line %d: got %d, calculated %d\n",rec_cnt, file_crc,loc_crc);
            return errno;
        }
        
        while (readch() != '\n' && !errno); // garbage at end of line...
            
        RETURN_IF_ERROR();
        
        if (ptr[pos] == 0) 
            break;        
    }
    
    printf("Parsed %d records\n",data_rec_cnt);
//#ifdef DEBUG
    if (errno) {
        printf("Complete with errors.\n");
    } else {
        printf("Complete.\n");
    }
//#endif

    return errno;
}

#define TEST_WRITE(X, Y, T)   printf("#  %30s: %s\n", X, (write(Y,0xAB) != T)?"FAILED":"Passed")

int main (int argc, char ** argv) {
    if (argc == 1) {
        printf("Argument required...\n");
        return 1;
    }
    
    FILE *in = fopen(argv[1],"rb");
    if (!in) {
        printf("Failed to open %s\n",argv[1]);
        return 1;
    }
    
    fseek (in, 0, SEEK_END);   // non-portable
    sz = ftell (in);
    rewind(in);
    
    fprintf(stderr, "File size: %d\n", sz);
    ptr = (uint8_t*)malloc(sz+1);
    
    if (fread(ptr, 1, sz, in) != sz) {
        printf("Did not read entire file\n");
        return 1;
    }
    
    fclose(in);
     
     sz++;
    ptr[sz-1] = 0;
    printf("%hhd\n",ptr[sz]);
     
    flags = 0;
    
    for (int i = 0; i < 0x100000; i++)
        MEMORY[i] = 0;
    
    printf("Parsing: ");
    uint8_t ret = parseSREC(false);
    
    if (ret) {
        printf("Error %x: ", ret);
        for (int i = 7; i >= 0; i--) {
            printf ("%c",((ret >> i) & 0x1) + '0');
            if (i == 4) printf(" ");
        }
        
        printf("\n");
        printf("Programming aborted.\n"); 
        return 1;
    } else 
        printf("OKAY\n");
      
    printf("Programming: ");
    ret = parseSREC(true);
    
    if (ret) {
        printf("Error %x: ", ret);
        for (int i = 7; i >= 0; i--) {
            printf ("%c",((ret >> i) & 0x1) + '0');
            if (i == 4) printf(" ");
        }
        
        printf("\n");
    
    } else 
        printf("OKAY\n");
        
    FILE *f = fopen("out.bin","wt");
    fwrite(MEMORY, 1, 0x100000, f);
    fclose(f);

   /* write_armed = false;
    printf("### Writes with ARMED=false\n");
    
    TEST_WRITE("RAM write",0x123, 0);
    TEST_WRITE("IO write",IO_START, 1);
    TEST_WRITE("Flash write, no flags",FLASH_START+23331, 1);
    flags = ALLOW_FLASH;
    TEST_WRITE("Flash write, valid flags",FLASH_START+23331, 0);
    TEST_WRITE("Loader write, al_fl only",FLASH_START+32, 1);
    flags = ALLOW_LOADER;
    TEST_WRITE("Loader write, al_ld only",FLASH_START+32, 1);
    flags = ALLOW_FLASH | ALLOW_LOADER;
    TEST_WRITE("Loader write, al_ld | al_fl",FLASH_START+32, 0);
    TEST_WRITE("Flash write w/ same sector",FLASH_START+32, 0);
        
    flags = 0;
    write_armed = true;
    printf("### Writes with ARMED=true\n");
    
    TEST_WRITE("RAM write",0x123, 0);
    TEST_WRITE("IO write",IO_START, 1);
    TEST_WRITE("Flash write, no flags",FLASH_START+23331, 1);
    flags = ALLOW_FLASH;
    TEST_WRITE("Flash write, valid flags",FLASH_START+23331, 0);
    TEST_WRITE("Loader write, al_fl only",FLASH_START+32, 1);
    flags = ALLOW_LOADER;
    TEST_WRITE("Loader write, al_ld only",FLASH_START+32, 1);
    flags = ALLOW_FLASH | ALLOW_LOADER;
    TEST_WRITE("Loader write, al_ld | al_fl",FLASH_START+32, 0);
    TEST_WRITE("Flash write w/ same sector",FLASH_START+32, 0);*/
  
    
    //printf("%X\n", readAddr(4));
    
}

