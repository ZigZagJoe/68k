#include <stdio.h>
#include <stdlib.h>
#include "lzf.h"
#include "lzfx.h"

int main(int argc, char ** argv) {

    char * filename = "md5_mt_demo.s68.srb";

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
    
    /*
    /*  Buffer-to buffer compression.

    Supply pre-allocated input and output buffers via ibuf and obuf, and
    their size in bytes via ilen and olen.  Buffers may not overlap.

    On success, the function returns a non-negative value and the argument
    olen contains the compressed size in bytes.  On failure, a negative
    value is returned and olen is not modified.
*/

    unsigned int out_sz = size;
    int ret =  lzf_compress(in_dat, size, cmp_dat, out_sz);
    
    int compressed_sz = ret;
    
    printf("Compressed size: %d\n", ret);
   // printf("Size: %d\n",out_sz);
   
   FILE *outfile;
   if (outfile = fopen("out.bin","wb")) {
    fwrite (cmp_dat, compressed_sz, 1, outfile);
    fclose(outfile);
    }
    
    out_sz = size;
    ret = lzfx_decompress(cmp_dat, compressed_sz, dec_dat, &out_sz);
    
    printf("Return code: %d\n", ret);
    printf("Size: %d\n",out_sz);
    
    return 0;
}
