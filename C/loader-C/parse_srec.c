/* S-record parsing */

#include <flash.h>
#include <stdint.h>

#include <loader.h>

#define BREAK_IF_ERROR()    if (errno) break

// global state vars
uint8_t *srec;          // pointer to memory containing s record
uint32_t pos;           // current position in srec
uint32_t srec_sz;       // size of srec, including trailing null
uint8_t write_armed;    // boolean if writes are enabled or not

// info vars
uint16_t rec_cnt = 0;   // number of records
uint32_t entry_point;   // entry point, specified in S7-S9 records, 0 if none
uint32_t program_sz;    // number of data bytes written
uint8_t wr_flags;       // write flags (see loader.h)
uint8_t errno;          // error flags register
uint8_t checksum;       // checksum char

uint8_t erased_sectors[SECTOR_COUNT];
                        // array containing sectors that were erased

// read a character from srec
uint8_t readch() {
    if (pos >= srec_sz) {
        errno |= EARLY_EOF;
        dbgprintf("EOF encountered in readch\n");
        return 0;
    }
    
    char ch = srec[pos++];

    // if not a binary srec, throw an exception if character is not in a valid range
    if (!(wr_flags & BINARY_SREC) && ((ch < '0' || ch > 'z') && ch !='\n' && ch != '\r')) {
        dbgprintf("Invalid character encountered\n");
        errno |= INVALID_CHAR;
        return 0;
    }
    
    return ch;
}

// read a nibble (one hex char)
uint8_t read_nibble() {
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

// read a single byte
uint8_t read_byte() {
    uint8_t b;
    
    if (wr_flags & BINARY_SREC) {   // binary mode; just read a character
        b = readch();
    } else
        b = (read_nibble() << 4) | read_nibble(); // read two hex chars to assemble a byte
        
    checksum += b;
    return b;
}

// read an address of variable length
uint32_t readAddr(uint8_t len) {
    uint32_t i = 0;
    while (len-- > 0)
        i = (i << 8) | read_byte();
    return i;
}

// execute a write to the byte specified, erasing a sector if it has not been already erased
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

// write a byte to memory address
void ram_write(uint32_t addr, uint8_t byte) {

#ifdef WR_DEBUG   
    dbgprintf("%08X rmWrite %02X\n",addr,byte);
#endif
    
    if (!write_armed) 
        return;
     
    MEM(addr) = byte;
}

// perform a write (and do a lot of sanity checks)
uint8_t write(uint32_t addr, uint8_t byte) {

    // first: do a lot of sanity checks on the write
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
        
        // okay, seems valid: pass it to flash_write
        flash_write(addr, byte);
    } else  // execute RAM write
        ram_write(addr, byte);
    
    // verify that the byte was successfully written
    if (write_armed && MEM(addr) != byte) {
        dbgprintf("Write to %X of value %X failed.\n",addr,byte&0xFF);
        errno |= FAILED_WRITE;
        return 1;
    }
     
    return 0;
}

// parse a s-record, either test writing or writing for real (based on armed)
uint8_t parseSREC(uint8_t * buffer, uint32_t buffer_len, uint8_t fl, uint8_t armed) {
    
    // local variables 
    char Sc; // char to hold 'S'
    uint8_t typ, len, file_crc, loc_crc;
    uint32_t address;
    uint16_t data_rec_cnt = 0;
    
    // initialize global state variables
    srec_sz = buffer_len;
    wr_flags = fl;
    srec = buffer;
    write_armed = armed;

    rec_cnt = 0;
    pos = 0;
    errno = 0;
    entry_point = 0;
    program_sz = 0;
    
    // clear sector count array
    for (uint8_t i = 0; i < SECTOR_COUNT; i++)
        erased_sectors[i] = 0;    
        
    // check for trailing null
    if (srec[srec_sz-1] != 0) {
        errno |= FORMAT_ERROR;
        dbgprintf("Record is missing trailing null\n", srec[srec_sz]);
    }
 
    // number of address bytes for each record type
    const uint8_t addr_len[] = {2,2,3,4,0,2,3,4,3,2};
    
    while(!errno) {
        rec_cnt++;
        
        // begin of record
        Sc = readch();  // if not a S, bad news...
        
        if (Sc != 'S') {
            errno |= FORMAT_ERROR;
            dbgprintf("Record started with %c, not S.\n", Sc);
            break;
        }
        
        typ = readch() - '0'; // record type

        BREAK_IF_ERROR();
        
        if (typ > 9) {
            errno |= FORMAT_ERROR;
            dbgprintf("Record #%d is of invalid type\n", rec_cnt);
            break;
        }
        
        checksum = 0;  // reset checksum
        
        // number of data bytes (record byte count, - checksum, - addr bytes)
        len = read_byte() - addr_len[typ] - 1;
       
        // read address field
        address = readAddr(addr_len[typ]);
        
        BREAK_IF_ERROR();
        
        switch (typ) {
            case 3: // data records (varying address size)
            case 2:
            case 1:
                data_rec_cnt++;
            case 0: // metadata record
                for (int i = 0; i < len && !errno; i++) {
                    uint8_t byt = read_byte();
                    
                    if (typ == 0)
                        continue;
                
                    // perform write
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
        
        BREAK_IF_ERROR();
        
        if (len != 0 && (typ >= 4)) {
             errno |= FORMAT_ERROR;
             dbgprintf("Record #%d has data, but type (%d) should not contain data!\n", rec_cnt,typ);
             break;
        }
        
        loc_crc = ~checksum; // one's complement
        file_crc = read_byte();   // read checksum from file
        
        if (loc_crc != file_crc) {
            errno |= CRC_ERROR;
            dbgprintf("CRC error in record #%d: got %d, calculated %d\n",rec_cnt, file_crc,loc_crc);
            break;
        }
        
        while (readch() != '\n' && !errno); // garbage at end of line...
            
        BREAK_IF_ERROR();
        
        if (srec[pos] == 0) // we're done
            break;        
    }

    return errno;
}
