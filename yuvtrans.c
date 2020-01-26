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

unsigned char abdiff(unsigned char x, unsigned char y);
unsigned char getmax(unsigned char x, unsigned char y);
void help_message()
{
        printf("yuvprocess v0.1\nUsage: yuv_process W H inputfile THR(128) THRL print\n");
        printf("Example:\n./yuvtrans.exe 1729 2448 test.yuv\n");
        printf("./yuvtrans.exe 1729 2448 test.yuv 110\n");
        printf("./yuvtrans.exe 1729 2448 test.yuv 110 0\n");
        printf("./yuvtrans.exe 1729 2448 test.yuv 30 30 print\n");
        printf("With ffmpeg:\n");
        printf("./ffmpeg.exe -i test.jpg -pix_fmt yuvj420p test.yuv\n");
        printf("./yuvtrans.exe 1729 2448 test.yuv 107\n");
        printf("./ffmpeg.exe -s 1729x2448 -pix_fmt yuvj420p -i test_out.yuv -frames 1 -f image2 -y test_out.jpeg\n");
}
int main(int argc, char *argv[])
{
    int rc = 0, tmp;
    size_t ret = 0;
    unsigned char thr=THRD, thrl=0;
    int width = IMG_W, height = IMG_H;
    int algo_flag = 0;
    char* inputfilename = "C:\\files\\ffmpeg\\xcb.yuv";
    char outputfilename[MAX_BYTES_FILENAME];
    char* filename_end = "_out.yuv";
    memset(outputfilename, 0, MAX_BYTES_FILENAME);
    printf("Build @ %s %s\n", __DATE__, __TIME__);

    fflush(stdout);
    int ch;
    while((ch = getopt(argc,argv,"s:i:t:p"))!= -1)
    {
        putchar(ch);
        printf("\n");
        fflush(stdout);
        switch(ch)
        {
            case 's':
                printf("option s:'%s'\n",optarg);
                fflush(stdout);
                sscanf(optarg, "%dx%d", &width, &height);
                printf("w=%d h=%d\n", width, height);
                fflush(stdout);
                break;
            case 'i':
                printf("option i:'%s'\n",optarg);
                inputfilename = optarg;
                printf("input file:%s\n",inputfilename);
                break;
            case 't':
                printf("option t:'%s'\n",optarg);
                tmp = atoi(optarg);;
                if(tmp>0 && tmp<256){
                    thr = tmp;
                }
                break;
            case 'l':
                printf("option l:'%s'\n",optarg);
                tmp = atoi(optarg);;
                if(tmp>0 && tmp<256 && tmp< thr){
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
            default:
                printf("other option :%c\n",ch);
        }
        printf("optopt +%c\n",optopt);
        fflush(stdout);
    }
    fflush(stdout);

    if(argc <= 6 || !width || !height){
        help_message();
        if(argc == 1)return 0;
    }
    if(!thrl){
        thrl = thr;
    }
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
    printf("yuv_process %d %d %s %d %s print=%d %d\n",
            width, height, inputfilename, thr, outputfilename, algo_flag, thrl);
    FILE *fpi = fopen(inputfilename, "rb");
    if(fpi == NULL){
        printf("open input fail\n");
        rc = -1;
        goto quit;
    }
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
            ret = fread(next_input_line, width, 1, fpi);
            if(ret != 1){
                printf("fread ret %d i %x err %s\n", ret, i, strerror(errno));
                rc = -1;
                goto quit;
            }
        }
        memcpy(input_line, next_input_line, width);
        if(i < height -1){
            ret = fread(next_input_line, width, 1, fpi);
            if(ret != 1){
                printf("fread ret %d i %x err %s\n", ret, i, strerror(errno));
                rc = -1;
                goto quit;
            }
        }
        for(int j=0;j<width;j++){
            if(!algo_flag){
                if(input_line[j] >= thr)output_line[j]=255;
                else if(input_line[j] <=thrl)output_line[j]=0;
                else{
                    output_line[j] = (unsigned int)256*(input_line[j]-thrl)/(thr-thrl);
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
                    output_line[j]=0;
                }
                else{//vertical check
                    if(i == height - 1) ddt = abdiff(input_line[j],pre_input_line[j]);
                    else if(i == 0) ddt = abdiff(next_input_line[j],input_line[j]);
                    else{
                        ddt = getmax(abdiff(pre_input_line[j],input_line[j]), abdiff(next_input_line[j],input_line[j]));
                    }
                    if(ddt > thr){
                        output_line[j]=0;
                    }
                    else{
                        output_line[j]=255;
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
                        output_line[j]=0;
                }
                else{
                    output_line[j]=255;
                }
#endif
                if(abdiff(input_line[j], input_line[j+1]) > thr
                        || abdiff(input_line[j], pre_input_line[j]) > thr
                        || abdiff(input_line[j], pre_input_line[j-1]) > thr
                        || abdiff(input_line[j], pre_input_line[j+1]) > thr){
                        output_line[j]=0;
                }
                else{
                    output_line[j]=255;
                }
            }
        }
        fwrite(output_line, width, 1, fpo);
    }

    memset(output_line, 0x80, (width+1)/2);
    for(unsigned int i=0;i<height;i++)
    {
        fwrite(output_line, (width+1)/2, 1, fpo);
    }
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
