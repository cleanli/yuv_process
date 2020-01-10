#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#define THRD 128
#define IMG_W 5376
#define IMG_H 3024
#define UVSIZE (IMG_W * IMG_H / 2)
#define MAX_BYTES_FILENAME 128

unsigned char abdiff(unsigned char x, unsigned char y);
int main(int argc, char *argv[])
{
       size_t ret = 0;
       unsigned char thr=THRD;
       int width = IMG_W, height = IMG_H;
       int algo_flag = 0;
       char* inputfilename = "C:\\files\\ffmpeg\\xcb.yuv";
       char outputfilename[MAX_BYTES_FILENAME];
       char* filename_end = "_out.yuv";
       memset(outputfilename, 0, MAX_BYTES_FILENAME);
       //printf("argc %d\n", argc);
       if(argc <= 6){
               printf("yuvprocess v0.1\nUsage: yuv_process W H inputfile THR(128) print\n");
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
       if(argc >= 6){
               if(!strcmp("print",argv[5])){
	           algo_flag = 1;
                   filename_end = "_outP.yuv";
	       }
	       printf("print = %d\n", algo_flag);
       }
       //printf("last is %s\n", inputfilename+strlen(inputfilename)-4);
       sprintf(outputfilename, "%s%s", inputfilename, filename_end);
       if(strlen(inputfilename)>4 && !strcmp(".yuv", inputfilename+strlen(inputfilename)-4)){
               sprintf(outputfilename+strlen(inputfilename)-4, "%s", filename_end);
       }
       unsigned char* input_line = (char*) malloc(3*width);
       if(!input_line){
               printf("can't malloc memory!\n");
	       return -1;
       }
       unsigned char* output_line = input_line + width;
       unsigned char* pre_input_line = input_line + width*2;
       memset(input_line, 0, width);
       printf("start process...\n");
       printf("yuv_process %d %d %s %d %s print=%d\n",
           width, height, inputfilename, thr, outputfilename, algo_flag);
       FILE *fpi = fopen(inputfilename, "rb");
       FILE *fpo = fopen(outputfilename, "wb");
       if(fpi == NULL || fpo == NULL){
               printf("open fail\n");
               return -1;
       }
       for(unsigned int i=0;i<height;i++)
       {
               memcpy(pre_input_line, input_line, width);
               ret = fread(input_line, width, 1, fpi);
               if(ret != 1){
                       printf("fread ret %d i %x err %s\n", ret, i, strerror(errno));
                       return -1;
               }
	       for(int j=0;j<width;j++){
	           if(!algo_flag){
                       if(input_line[j] > thr)output_line[j]=255;
                       if(input_line[j] <=thr)output_line[j]=0;
		   }
		   else{
                       unsigned char ddt;
		       if(j == width -1) ddt = 0;
		       else ddt = abdiff(input_line[j+1],input_line[j]);
                       if(ddt > thr)output_line[j]=0;
		       else if(abdiff(input_line[j], pre_input_line[j]) > thr) output_line[j] = 0;
		       else output_line[j]=255;
		   }
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

unsigned char abdiff(unsigned char x, unsigned char y)
{
        if(x > y)return x-y;
	else return y-x;
}
