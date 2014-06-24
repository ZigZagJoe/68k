#include <flash.h>
#include <stdint.h>

//void printf(char * fmt, ...);

#include <loader.h>

// global vars
uint8_t *srec;
uint32_t pos;
uint32_t srec_sz;

uint32_t entry_point;
uint32_t program_sz;
uint8_t wr_flags;
uint8_t errno;
uint8_t erased_sectors[SECTOR_COUNT];
uint8_t write_armed;
uint8_t checksum;

#define RETURN_IF_ERROR()    if (errno) return errno

uint8_t readch() {
    if (pos >= srec_sz) {
        errno |= EARLY_EOF;
        dbgprintf("EOF encountered in readch\n");
        return 0;
    }
    
    char ch = srec[pos++];
    
    if (!(wr_flags & BINARY_SREC) && ((ch < '0' || ch > 'z') && ch !='\n' && ch != '\r')) {
        dbgprintf("Invalid character encountered\n");
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
        
    dbgprintf("Expected valid hex char, got %c.\n",ch);
    errno |= BAD_HEX_CHAR;
    return 0;        
}

uint8_t getb() {
    uint8_t b;
    
    if (wr_flags & BINARY_SREC) {
        b = readch();
    } else
        b = (getn() << 4) | getn();
        
    checksum += b;
    return b;
}

uint32_t readAddr(uint8_t len) {
    uint32_t i = 0;
    while (len-- > 0)
        i = (i << 8) | getb();
    return i;
}

void flash_write(uint32_t addr, uint8_t byte) {
    uint8_t sector = ADDR_TO_SECTOR(addr);
    
#ifdef WR_DEBUG   
    dbgprintf("%08X flWrite %02hhX (sect %hhd)\n", addr, byte, sector);
#endif

    if (!erased_sectors[sector]) {
    
#ifdef WR_DEBUG   
         dbgprintf("Flash erase sector %hhd\n", sector);
#endif
         erased_sectors[sector] = 1;
          
         if (write_armed)
            flash_erase_sector(sector);
    }
    
    if (!write_armed) 
        return;
    
    flash_write_byte(addr, byte);
}

void ram_write(uint32_t addr, uint8_t byte) {

#ifdef WR_DEBUG   
    dbgprintf("%08X rmWrite %02X\n",addr,byte);
#endif
    
    if (!write_armed) 
        return;
     
    MEM(addr) = byte;
}

uint8_t write(uint32_t addr, uint8_t byte) {
    if (addr < LOWEST_VALID_ADDR) {
        errno |= INVALID_WRITE;
        dbgprintf("Attempted to write to system address space. Bad address: 0x%X\n", addr);
        return 1;
    } else if (addr >= IO_START) {
        errno |= INVALID_WRITE;
        dbgprintf("Attempted to write to IO address space (or higher). Bad address: 0x%X\n", addr);
        return 1;
    } else if (addr >= FLASH_START && addr < IO_START) {
        if (!(wr_flags & ALLOW_FLASH)) {
            errno |= INVALID_WRITE;
            dbgprintf("Attempted to write to flash without allow_flash flag set.\n");
            return 1;
        }
        
        if (addr < LOADER_END && !(wr_flags & ALLOW_LOADER)) {
            errno |= INVALID_WRITE;
            dbgprintf("Attempted to write to loader region without allow_loader flag set.\n");
            return 1;
        }
        
        flash_write(addr, byte);
    } else 
        ram_write(addr, byte);
    
    if (write_armed && MEM(addr) != byte) {
        dbgprintf("Write to %X of value %X failed.\n",addr,byte&0xFF);
        errno |= FAILED_WRITE;
        return 1;
    }
     
    return 0;
}

uint8_t parseSREC(uint8_t * buffer, uint32_t buffer_len, uint8_t fl, uint8_t armed) {
    srec_sz = buffer_len;
    wr_flags = fl;
    srec = buffer;
    
    pos = 0;
    errno = 0;
    entry_point = 0;
    program_sz = 0;
    
    write_armed = armed;
       
    char Sc; // char to hold 'S'
    uint8_t typ, len, file_crc, loc_crc;
    uint32_t address;
    
    uint16_t data_rec_cnt = 0;
    uint16_t rec_cnt = 0;

    // clear sector count array
    for (uint8_t i = 0; i < SECTOR_COUNT; i++)
        erased_sectors[i] = 0;    
    
    if (srec[srec_sz-1] != 0) {
        errno |= FORMAT_ERROR;
        dbgprintf("Record is missing trailing null\n", srec[srec_sz]);
        return errno;
    }
 
    // number of address bytes for each record type
    const uint8_t addr_len[] = {2,2,3,4,0,2,3,4,3,2};
    
    while(1) {
        rec_cnt++;
        
        Sc = readch();  // if not a S, bad news
        
        if (Sc != 'S') {
            errno |= FORMAT_ERROR;
            dbgprintf("Record started with %c, not S.\n", Sc);
            break;
        }
        
        typ = readch() - '0'; // record type

        RETURN_IF_ERROR();
        
        if (typ > 9) {
            errno |= FORMAT_ERROR;
            dbgprintf("Record of invalid type\n");
            break;
        }
        
        checksum = 0;              // reset checksum
        len = getb() - addr_len[typ] - 1;
       
        address = readAddr(addr_len[typ]);
        
        RETURN_IF_ERROR();
        
        switch (typ) {
            case 3: // data records (varying address size)
            case 2:
            case 1:
                data_rec_cnt++;
            case 0: // metadata record
                for (int i = 0; i < len && !errno; i++) {
                    uint8_t byt = getb();
                    if (typ == 0)
                        continue;
                
                    write(address, byt);
                    address++; 
                    program_sz++;
                }
                break;
            case 5: // record count
            case 6:
                if (address != data_rec_cnt) {
                    errno |= FORMAT_ERROR;
                    dbgprintf("Data record count missmatch (read %d, actual %d)\n",address,data_rec_cnt);
                    return errno;
                }
                break;
            case 7: // entry point records (varying address size)
            case 8:
            case 9:
                entry_point = address;
                //dbgprintf("Set entry point to 0x%x\n",entry_point);
                break;
        }
        
        RETURN_IF_ERROR();
        
        if (len != 0 && (typ >= 4)) {
             errno |= FORMAT_ERROR;
             dbgprintf("Record has data in record that should not contain data!\n");
             return errno;
        }
        
        loc_crc = ~checksum;
        file_crc = getb();
        
        if (loc_crc != file_crc) {
            errno |= CRC_ERROR;
            dbgprintf("CRC error in record on line %d: got %d, calculated %d\n",rec_cnt, file_crc,loc_crc);
            return errno;
        }
        
        while (readch() != '\n' && !errno); // garbage at end of line...
            
        RETURN_IF_ERROR();
        
        if (srec[pos] == 0) 
            break;        
    }

    return errno;
}

uint8_t handle_srec(uint8_t * start, uint32_t len, uint8_t fl) { 
    //mem_dump(start, len);
     
    dbgprintf("start=0x%x, len=0x%x, wr_flags=0x%x\n", start, len, wr_flags);
   
    // do sanity check parse run
    uint8_t ret = parseSREC(start,len,fl,0);
    
    if (ret) 
        return ret;
    
    // if no errors, do programming
    ret = parseSREC(start,len,fl,1);
    
    if (ret) 
        return ret | PROG_FAILURE;
    
    return 0;
}

/*
void _putc( char ch) {
    while(!(TSR & 0x80));
    UDR = ch;

}

void _puts(char *str) {
    while (*str != 0)
        _putc(*str++);
}

void mem_dump(uint8_t *addr, uint32_t cnt) {
    int c = 0;
    char ascii[17];
    ascii[16] = 0;
    
    _putc('\n');
     char *hex_chars = "0123456789ABCDEF";
	
    printf("%X   ", addr);
    for (uint32_t i = 0; i < cnt; i++) {
        uint8_t b = *addr;
        
        ascii[c] = (b > 31 & b < 127) ? b : '.';
        
		_putc(hex_chars[b>>4]);
		_putc(hex_chars[b&0xF]);
		_putc(' ');
		
        addr++;
        
        if (++c == 16) {
            _putc(' '); _putc(' ');
            _puts(ascii);
            _putc('\n');
            if ((i+1) < cnt)
                printf("%X   ", addr);
            c = 0;
        }
    }
    
    if (c < 15) _putc('\n');
}*/
