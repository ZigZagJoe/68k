#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv) {

 if (argc < 3) {
   printf("Missing arguments. Expected: srec2bin infile outfile\n");
   exit(1);
 }
 
 FILE *inFile;
 FILE *outFile;
 
 if (!(inFile = fopen(argv[1],"rb"))) {
  printf("Failed to open infile %s\n",argv[1]);
  exit(1);
 }
 
 if (!(outFile = fopen(argv[2],"wb"))) {
  printf("Failed to open outfile %s\n",argv[2]);
  exit(1);
 }
 
 char buffer[128];
 
 while (fgets(buffer,128,inFile)) {
  if (buffer[0] != 'S') {
  	printf("Error: line not starting with S.\n%s\nAborting.\n",buffer);
  	break;
  }
  
  if (buffer[1] != '1' && buffer[1] != '2' && buffer[1] != '3') 
  	continue; // not a data record
  	
  unsigned int skip = 6;
  
  if (buffer[1] == '2')
   skip++;
   
  if (buffer[1] == '3')
   skip+=2;
    
  int i;
  int dos = 0;
  
  for (i = 0; i < 128; i++) {
   if (buffer[i] == '\r') dos=1;
   if (buffer[i] == '\n') break;
  }
  
  buffer[i-(2 + dos)] = 0;
  
  printf("%s\n",&buffer[skip]);
 }
 
 fclose(inFile);
 fclose(outFile);
 
 return 0;
}
