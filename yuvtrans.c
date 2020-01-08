#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#define THRD 128
#define IMG_W 5376
#define IMG_H 3024
#define UVSIZE (IMG_W * IMG_H / 2)


int main(int argc, char *argv[])
{
       size_t ret = 0;
       unsigned char thr=THRD;
       int width = IMG_W, height = IMG_H;
       printf("argc %d\n", argc);
       if(argc <= 6){
               printf("Usage: yuv_process W H THR inputfile outputfile\n");
       }
       if(argc >= 2){
               width = atoi(argv[1]);
	       printf("W = %d\n", width);
       }
       if(argc >= 3){
               height = atoi(argv[2]);
	       printf("H = %d\n", height);
       }
       if(argc >= 4){
               thr = atoi(argv[3]);
	       printf("THR = %d\n", thr);
       }
       unsigned char* input_line = (char*) malloc(2*width);
       if(!input_line){
               printf("can't malloc memory!\n");
	       return -1;
       }
       unsigned char* output_line = input_line + width;
       printf("hello world.\n");
       FILE *fpi = fopen("C:\\files\\ffmpeg\\xcb.yuv", "rb");
       FILE *fpo = fopen("C:\\files\\ffmpeg\\xcb_o.yuv", "wb");
       if(fpi == NULL || fpo == NULL){
               printf("open fail\n");
               return -1;
       }
       for(unsigned int i=0;i<height;i++)
       {
               ret = fread(input_line, width, 1, fpi);
               if(ret != 1){
                       printf("fread ret %d i %x err %s\n", ret, i, strerror(errno));
                       return -1;
               }
	       for(int j=0;j<width;j++){
	               //printf("i put j %x i[j] %x\n", j, input_line[j]);
                       if(input_line[j] > thr)output_line[j]=255;
                       if(input_line[j] <=thr)output_line[j]=0;
	       }
               fwrite(output_line, width, 1, fpo);
       }

       memset(output_line, 0x80, width/2);
       for(unsigned int i=0;i<height;i++)
       {
               fwrite(output_line, width/2, 1, fpo);
       }

       free(input_line);
       fclose(fpi);
       fclose(fpo);
}

