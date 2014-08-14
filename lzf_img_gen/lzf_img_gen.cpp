#include <stdio.h>
#include <stdlib.h>
#include "lzf.h"
#include <string.h>

#define ROL(num, bits) (((num) << (bits)) | ((num) >> (32 - (bits))))

uint32_t qcrc_update (uint32_t inp, uint8_t v) {
	return ROL(inp ^ v, 1);
}

uint32_t bswap(uint32_t num) {
    return ((num>>24)&0xff) | // move byte 3 to byte 0
                    ((num<<8)&0xff0000) | // move byte 1 to byte 2
                    ((num>>8)&0xff00) | // move byte 2 to byte 1
                    ((num<<24)&0xff000000);
}

int main(int argc, char ** argv) {

    char * filename = "md5_mt_demo.s68.srb";
    char * outfile;
    
    if (argc < 2) {
        printf("Missing arguments. Usage: lzf_img_gen infile [outfile]\n\n");
        return 1;
    }
    
    filename = argv[1];
    outfile = argv[2];    
    
    FILE *in = fopen(filename,"rb");
    FILE *out = 0;

    if (outfile && !(out = fopen(outfile,"wb"))) {
        printf("Failed to open %s\n",outfile);
        return 1;
    }
    
    if (!in) {
        printf("Failed to open %s\n",filename);
        return 1;
    }
    
    // get file size
    fseek (in, 0, SEEK_END);   // non-portable
    int size = ftell (in);
    rewind(in);
    
    printf("File: %s\n",filename);
    fprintf(stderr, "Size: %d\n", size);
    
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
   
    int ret = lzf_compress(in_dat, size, cmp_dat, size);
    
    if (!ret) {
        printf("Compression failed: incompressible data.\n");
        return 1;
    } 
    
    printf("Compressed size: %d\n", ret);

    if (out) {
        int compressed_sz = bswap(ret);
        fwrite(&compressed_sz, 4, 1, out);
        fwrite(cmp_dat, ret, 1, out);
        fclose(out);
        printf("Written to %s\n",outfile);
    } else printf("(not saving output...)\n");
    

    return 0;
}
