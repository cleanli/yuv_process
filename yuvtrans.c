#include <stdio.h>
#include <string.h>
#include <errno.h>
#define THRD 128
#define IMG_W 5376
#define IMG_H 3024
#define UVSIZE (IMG_W * IMG_H / 2)


int main()
{
       size_t ret = 0;
       unsigned char c;
       printf("hello world.\n");
       FILE *fpi = fopen("C:\\files\\ffmpeg\\xcb.yuv", "rb");
       FILE *fpo = fopen("C:\\files\\ffmpeg\\xcb_o.yuv", "wb");
       if(fpi == NULL || fpo == NULL){
               //if(fpi == NULL){
               printf("open fail\n");
               return -1;
       }
       for(unsigned int i=0;i<UVSIZE*2;i++)
               //for(unsigned int i=0;i<UVSIZE;i++)
       {
               ret = fread(&c, 1, 1, fpi);
               if(ret != 1){
                       printf("ret %d i %x err %s\n", ret, i, strerror(errno));
                       return -1;
               }
               if(c > THRD)c=255;
               if(c <=THRD)c=0;
               //printf("c = %02x\n", c);
               fwrite(&c, 1, 1, fpo);
       }

       c=0x80;
       for(unsigned int i=0;i<UVSIZE;i++)
               //for(unsigned int i=0;i<UVSIZE;i++)
       {
               fwrite(&c, 1, 1, fpo);
       }

       fclose(fpi);
       fclose(fpo);
}

