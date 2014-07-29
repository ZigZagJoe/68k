#include <stdio.h>
#include <stdlib.h>
#include "lzf.h"
#include "lzfx.h"
#include <string.h>

#define ROL(num, bits) (((num) << (bits)) | ((num) >> (32 - (bits))))

uint32_t qcrc_update (uint32_t inp, uint8_t v) {
	return ROL(inp ^ v, 1);
}

int main(int argc, char ** argv) {

    char * filename = "md5_mt_demo.s68.srb";

    if (argc > 1)
        filename = argv[1];
        
     FILE *in = fopen(filename,"rb");
    
    if (!in) {
        printf("Failed to open %s\n",filename);
        return 1;
    }
    
    // get file size
    fseek (in, 0, SEEK_END);   // non-portable
    int size = ftell (in);
    rewind(in);
    
    fprintf(stderr, "Input size: %d\n", size);
    
    // allocate buffers
    uint8_t *in_dat = (uint8_t*)malloc(size);
    uint8_t *cmp_dat = (uint8_t*)malloc(size);
    uint8_t *dec_dat = (uint8_t*)malloc(size);
    
    if (!in_dat || !cmp_dat || !dec_dat) {
        printf("Failed to allocate memory\n");
        return 1;
    }
    
    if (fread(in_dat, 1, size, in) != size) {
        printf("Did not read entire file\n");
        return 1;
    }
    
    fclose(in);
    
    uint32_t crc;
     
    crc = 0xDEADBEEF;
    for (int i  = 0; i < size; i++)
        crc = qcrc_update(crc, in_dat[i]);
      
    printf("Input CRC: %08X\n",crc); 
    
    unsigned int out_sz = size;
    int ret = lzf_compress(in_dat, size, cmp_dat, out_sz);
    
    if (!ret) {
        printf("Compression failed: incompressible data.\n");
        return 1;
    }
    
    int compressed_sz = ret;
    
    printf("Compressed size: %d\n", ret);
   // printf("Size: %d\n",out_sz);
   
    FILE *outfile;
    if (outfile = fopen("out.bin","wb")) {
        fwrite (cmp_dat, compressed_sz, 1, outfile);
        fclose(outfile);
    }
    
    printf("\nDecompress test:\n");
    out_sz = size;
    ret = lzfx_decompress(cmp_dat, compressed_sz, dec_dat, &out_sz);
    
    printf("Return code: %d\n", ret);
    printf("Size: %d\n",out_sz);
    
    crc = 0xDEADBEEF;
    for (int i  = 0; i < out_sz; i++)
        crc = qcrc_update(crc, dec_dat[i]);
      
    printf("Decompress CRC: %08X\n",crc); 
    
    if (size > 450*1024) {
        printf("File size too large for memory test.\n");
        return 1;
    }
    
    uint8_t * memory = (uint8_t*)malloc(512*1024);
    
    if (!memory) {
        printf("Failed to allocate memory array.");
        return 1;
    }
    
    memset(memory, 0xFE, 512*1024);
    
    uint32_t load_addr = (0x80000 - 4096 - compressed_sz) & ~1;
    uint32_t dec_addr = (0x80000 - 4096 - size) & ~1 - 8096; // 8096: max range of backref
    
    uint8_t * ld_ptr = memory+load_addr;
    uint8_t * dec_ptr = memory+dec_addr;
    
    printf("\nLoad compressed to %08X in memory\n",load_addr);
    memcpy(ld_ptr,cmp_dat,compressed_sz);

    printf("Decompress to %08X in memory\n",dec_addr);
    
    out_sz = size;
    ret = lzfx_decompress(memory + load_addr, compressed_sz, dec_ptr, &out_sz);
    
    printf("Return code: %d\n", ret);
    printf("Size: %d\n",out_sz);
    
    crc = 0xDEADBEEF;
    for (int i  = 0; i < out_sz; i++)
        crc = qcrc_update(crc, dec_ptr[i]);
      
    printf("CRC: %08X\n",crc); 
    
    return 0;
}
