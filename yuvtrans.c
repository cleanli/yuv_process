#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#define THRD 128
#define IMG_W 5376
#define IMG_H 3024
#define UVSIZE (IMG_W * IMG_H / 2)
#define MAX_BYTES_FILENAME 128

int main(int argc, char *argv[])
{
       size_t ret = 0;
       unsigned char thr=THRD;
       int width = IMG_W, height = IMG_H;
       char* inputfilename = "C:\\files\\ffmpeg\\xcb.yuv";
       char outputfilename[MAX_BYTES_FILENAME];
       memset(outputfilename, 0, MAX_BYTES_FILENAME);
       //printf("argc %d\n", argc);
       if(argc <= 5){
               printf("yuvprocess v0.1\nUsage: yuv_process W H inputfile THR(128)\n");
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
               inputfilename = argv[3];
	       printf("filename = %s\n", inputfilename);
       }
       if(argc >= 5){
               thr = atoi(argv[4]);
	       printf("THR = %d\n", thr);
       }
       //printf("last is %s\n", inputfilename+strlen(inputfilename)-4);
       sprintf(outputfilename, "%s_out.yuv", inputfilename);
       if(strlen(inputfilename)>4 && !strcmp(".yuv", inputfilename+strlen(inputfilename)-4)){
               sprintf(outputfilename+strlen(inputfilename)-4, "%s", "_out.yuv");
       }
       unsigned char* input_line = (char*) malloc(2*width);
       if(!input_line){
               printf("can't malloc memory!\n");
	       return -1;
       }
       unsigned char* output_line = input_line + width;
       printf("start process...\n");
       printf("yuv_process %d %d %s %d %s\n", width, height, inputfilename, thr, outputfilename);
       FILE *fpi = fopen(inputfilename, "rb");
       FILE *fpo = fopen(outputfilename, "wb");
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
       printf("done\n");

       free(input_line);
       fclose(fpi);
       fclose(fpo);
}

