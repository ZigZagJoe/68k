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

uint8_t parseSREC(uint8_t * start, uint32_t l, uint8_t fl) {  
    //printf("start=0x%x, len=0x%x, flags=0x%x\n", start, len, flags);
    sz = l;
    flags = fl;
    ptr = start;
   
    pos = 0;
    errno = 0;
    entry = 0;
    
    write_armed = false;
       
    char Sc; // char to hold 'S'
    uint8_t typ, len, file_crc, loc_crc;
    uint32_t address;
    
    uint16_t data_rec_cnt = 0;
    uint16_t rec_cnt = 0;

    // clear sector count array
    for (uint8_t i = 0; i < SECTOR_COUNT; i++)
        erased_sectors[i] = 0;    
    
    if (ptr[sz] != 0) {
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
        
        RETURN_IF_ERROR();
        
        if (typ > 9) {
            errno |= FORMAT_ERROR;
            printf("Record has type > 9 (%d)", typ);
            break;
        }
        
        chk = 0;              // reset checksum
        len = getb() - addr_len[typ] - 1;

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
                entry = address;
                printf("Entry point of 0x%x\n",entry);
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
    
    if (!errno) 
        printf("SREC validated: parsed %d records.\n", rec_cnt);

    return errno;
}
