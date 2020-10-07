#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#define THRD 128
#define IMG_W 5376
#define IMG_H 3024
#define UVSIZE (IMG_W * IMG_H / 2)
#define MAX_BYTES_FILENAME 128

#include "j.h"

char* input_buffer = NULL;
char* output_buffer = NULL;
int input_bufsize = 0;
int output_bufsize = 0;

unsigned char abdiff(unsigned char x, unsigned char y);
unsigned char getmax(unsigned char x, unsigned char y);
void help_message()
{
        printf("yuvprocess v0.3\nUsage: yuv_process -s WxH -i inputfile -r THR(128) -l THRL (-p) -c uDdDlDrD\n");
        printf("Example:\n./yuvtrans.exe -s 1729x2448 -i test.yuv\n");
        printf("./yuvtrans.exe -s 1729x2448 -i test.yuv -r 110 -c u100d100l50r50\n");
        printf("./yuvtrans.exe -s 1729x2448 -i test.yuv -r 110 -l 0\n");
        printf("./yuvtrans.exe -s 3264x2448 -i IMG_20200126_233432.yuv -r 160 -l 100\n");
        printf("./yuvtrans.exe -s 1729x2448 -i test.yuv -r 30 -l 30 -p\n");
        printf("With ffmpeg:\n");
        printf("./ffmpeg.exe -i test.jpg -pix_fmt yuvj420p test.yuv\n");
        printf("./yuvtrans.exe -s 1729x2448 -i test.yuv -r 107\n");
        printf("./ffmpeg.exe -s 1729x2448 -pix_fmt yuvj420p -i test_out.yuv -frames 1 -f image2 -y test_out.jpeg\n");
        printf("./process_sh IMG_20200126_233432 110 40 u200d200l20r20\n");
        printf("\n\nGet size of jpeg:\nyuv_process -j inputfile\n");
}
int main(int argc, char *argv[])
{
    int rc = 0, tmp;
    int quiet = 0;
    int jpeginput = 0;
    size_t ret = 0;
    unsigned char thr=THRD, thrl=THRD;
    int cut_up = 0, cut_down = 0, cut_left = 0, cut_right = 0;
    int width = IMG_W, height = IMG_H;
    int outwidth=width, outheight=height;
    int algo_flag = 0;
    char* inputfilename = "C:\\files\\ffmpeg\\xcb.yuv";
    char outputfilename[MAX_BYTES_FILENAME];
    char* filename_end = "_out.yuv";
    char* ibp = NULL;
    char* obp = NULL;
    memset(outputfilename, 0, MAX_BYTES_FILENAME);
    int ch;
    while((ch = getopt(argc,argv,"s:i:r:l:pj:c:qJ"))!= -1)
    {
        //putchar(ch);
        //printf("\n");
        //fflush(stdout);
        switch(ch)
        {
            case 'J':
                jpeginput = 1;
                break;
            case 'q':
                quiet = 1;
                break;
            case 'c':
                if(!quiet){
                    printf("option c:'%s'\n",optarg);
                    fflush(stdout);
                }
                sscanf(optarg, "u%dd%dl%dr%d", &cut_up, &cut_down, &cut_left, &cut_right);
                if(!quiet){
                    printf("cut u=%d d=%d l=%d r=%d\n", cut_up, cut_down, cut_left, cut_right);
                    fflush(stdout);
                }
                break;
            case 's':
                printf("option s:'%s'\n",optarg);
                fflush(stdout);
                sscanf(optarg, "%dx%d", &width, &height);
                printf("w=%d h=%d\n", width, height);
                outwidth=width;
                outheight=height;
                fflush(stdout);
                break;
            case 'i':
                printf("option i:'%s'\n",optarg);
                inputfilename = optarg;
                printf("input file:%s\n",inputfilename);
                break;
            case 'r':
                printf("option r:'%s'\n",optarg);
                tmp = atoi(optarg);;
                if(tmp>0 && tmp<256){
                    thr = tmp;
                }
                break;
            case 'l':
                printf("option l:'%s'\n",optarg);
                tmp = atoi(optarg);;
                if(tmp>=0 && tmp< thr){
                    thrl = tmp;
                    printf("THRL = %d\n", thrl);
                }
                else{
                    printf("err thrl\n");
                }
                break;
            case 'p':
                algo_flag = 1;
                filename_end = "_outP.yuv";
                printf("print = %d\n", algo_flag);
                break;
            case 'j':
                inputfilename = optarg;
                if(0 == GetJPEGWidthHeight(optarg, &width, &height)){
                    printf("%dx%d\n", width-cut_left-cut_right, height-cut_up-cut_down);
                }
                else{
                    printf("err\n");
                }
                return 0;
                break;
            default:
                printf("other option :%c\n",ch);
        }
        if(!quiet){
            printf("optopt +%c\n",optopt);
        }
        fflush(stdout);
    }
    fflush(stdout);
    printf("Build @ %s %s\n", __DATE__, __TIME__);
    fflush(stdout);

    if(argc <= 6 || !width || !height){
        help_message();
        if(argc == 1)return 0;
    }
    outwidth=width-cut_left-cut_right;
    outheight=height-cut_up-cut_down;
    printf("THRL = %d\n", thr);
    //printf("last is %s\n", inputfilename+strlen(inputfilename)-4);
    sprintf(outputfilename, "%s%s", inputfilename, filename_end);
    if(strlen(inputfilename)>4 && !strcmp(".yuv", inputfilename+strlen(inputfilename)-4)){
        sprintf(outputfilename+strlen(inputfilename)-4, "%s", filename_end);
    }
    unsigned char* mem = (char*) malloc(4*(width+2));
    if(!mem){
        printf("can't malloc memory!\n");
        rc = -1;
        goto quit;
    }
    unsigned char* input_line = mem + 1;
    unsigned char* output_line = mem + (width + 2) + 1;
    unsigned char* pre_input_line = mem + (width + 2)*2 + 1;
    unsigned char* next_input_line = mem + (width + 2)*3 + 1;
    memset(mem, 0x80, (width + 2) * 4);
    printf("start process...\n");
    printf("yuv_process %dx%d %s %d %s print=%d %d\n",
            width, height, inputfilename, thr, outputfilename, algo_flag, thrl);
    FILE *fpi = fopen(inputfilename, "rb");
    if(fpi == NULL){
        printf("open input fail\n");
        rc = -1;
        goto quit;
    }
    input_bufsize = get_buffer_size(width, height);
    output_bufsize = get_buffer_size(outwidth, outheight);
    input_buffer = (char*)malloc(input_bufsize);
    output_buffer = (char*)malloc(output_bufsize);
    if(input_buffer == NULL || output_buffer == NULL){
        printf("alloc mem fail\n");
        rc = -1;
        goto quit;
    }
    ret = fread(input_buffer, 1, input_bufsize, fpi);
    if(ret != input_bufsize){
        printf("fread ret %d not meet input bufsize err %s\n", ret, strerror(errno));
        rc = -1;
        goto quit;
    }
    ibp = input_buffer;
    obp = output_buffer;
    FILE *fpo = fopen(outputfilename, "wb");
    if(fpi == NULL || fpo == NULL){
        printf("open output fail\n");
        fclose(fpi);
        rc = -1;
        goto quit;
    }
    for(unsigned int i=0;i<height;i++)
    {
        memcpy(pre_input_line, input_line, width);
        if(i == 0){
            /*
            ret = fread(next_input_line, width, 1, fpi);
            if(ret != 1){
                printf("fread ret %d i %x err %s\n", ret, i, strerror(errno));
                rc = -1;
                goto quit;
            }
            */
            memcpy(next_input_line, ibp, width);
            ibp += width;
        }
        memcpy(input_line, next_input_line, width);
        if(i < height){
            /*
            ret = fread(next_input_line, width, 1, fpi);
            if(ret != 1){
                printf("fread ret %d i %x err %s\n", ret, i, strerror(errno));
                rc = -1;
                goto quit;
            }
            */
            memcpy(next_input_line, ibp, width);
            ibp += width;
        }
        if(i < cut_up || i >= (height-cut_down)){
            continue;
        }
        for(int j=0;j<width;j++){
            if(j < cut_left || j >= (width-cut_right)){
                continue;
            }
            if(!algo_flag){
                if(input_line[j] >= thr)output_line[j-cut_left]=255;
                else if(input_line[j] <=thrl)output_line[j-cut_left]=0;
                else{
                    output_line[j-cut_left] = (unsigned int)256*(input_line[j]-thrl)/(thr-thrl);
                }
            }
            else{
#if 0
                unsigned char ddt;
                if(j == width -1) ddt = abdiff(input_line[j-1],input_line[j]);
                else if(j == 0) ddt = abdiff(input_line[j+1],input_line[j]);
                else{
                    ddt = getmax(abdiff(input_line[j-1],input_line[j]), abdiff(input_line[j+1],input_line[j]));
                }
                if(ddt > thr){
                    output_line[j-cut_left]=0;
                }
                else{//vertical check
                    if(i == height - 1) ddt = abdiff(input_line[j],pre_input_line[j]);
                    else if(i == 0) ddt = abdiff(next_input_line[j],input_line[j]);
                    else{
                        ddt = getmax(abdiff(pre_input_line[j],input_line[j]), abdiff(next_input_line[j],input_line[j]));
                    }
                    if(ddt > thr){
                        output_line[j-cut_left]=0;
                    }
                    else{
                        output_line[j-cut_left]=255;
                    }
                }
#endif
#if 0
                if(abdiff(input_line[j], input_line[j-1]) > thr
                        || abdiff(input_line[j], input_line[j+1]) > thr
                        || abdiff(input_line[j], pre_input_line[j]) > thr
                        || abdiff(input_line[j], next_input_line[j]) > thr
                        || abdiff(input_line[j], pre_input_line[j-1]) > thr
                        || abdiff(input_line[j], next_input_line[j-1]) > thr
                        || abdiff(input_line[j], pre_input_line[j+1]) > thr
                        || abdiff(input_line[j], next_input_line[j+1]) > thr){
                        output_line[j-cut_left]=0;
                }
                else{
                    output_line[j-cut_left]=255;
                }
#endif
                if(abdiff(input_line[j], input_line[j+1]) > thr
                        || abdiff(input_line[j], pre_input_line[j]) > thr
                        || abdiff(input_line[j], pre_input_line[j-1]) > thr
                        || abdiff(input_line[j], pre_input_line[j+1]) > thr){
                        output_line[j-cut_left]=0;
                }
                else{
                    output_line[j-cut_left]=255;
                }
            }
        }
        //fwrite(output_line, width-cut_right-cut_left, 1, fpo);
        memcpy(obp, output_line, width-cut_right-cut_left);
        obp += outwidth;
    }

    memset(output_line, 0x80, (width-cut_right-cut_left+1)/2);
    for(unsigned int i=cut_up;i<height-cut_down;i++)
    {
        //fwrite(output_line, (width-cut_right-cut_left+1)/2, 1, fpo);
        memcpy(obp, output_line, (width-cut_right-cut_left+1)/2);
        obp += (outwidth+1)/2;
    }
    fwrite(output_buffer, 1, output_bufsize, fpo);
    printf("write output size %d ret %d\n", output_bufsize, ret);
    printf("done\n");

quit:
    free(input_line);
    if(!fpi)fclose(fpi);
    if(!fpo)fclose(fpo);
    return ret;
}

unsigned char abdiff(unsigned char x, unsigned char y)
{
    if(x > y)return x-y;
    else return y-x;
}

unsigned char getmax(unsigned char x, unsigned char y)
{
    return (x > y)?x:y;
}
