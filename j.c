#include <stdio.h>
#include <tchar.h>

#define MAKEUS(a, b)    ((unsigned short) ( ((unsigned short)(a))<<8 | ((unsigned short)(b)) ))
#define MAKEUI(a,b,c,d) ((unsigned int) ( ((unsigned int)(a)) << 24 | ((unsigned int)(b)) << 16 | ((unsigned int)(c)) << 8 | ((unsigned int)(d)) ))

#define M_DATA  0x00
#define M_SOF0  0xc0
#define M_DHT   0xc4
#define M_SOI   0xd8
#define M_EOI   0xd9
#define M_SOS   0xda
#define M_DQT   0xdb
#define M_DNL   0xdc
#define M_DRI   0xdd
#define M_APP0  0xe0
#define M_APPF  0xef
#define M_COM   0xfe

#include <stdlib.h>
#include <string.h>


int GetJPEGWidthHeight(const char* path, unsigned int *punWidth, unsigned int *punHeight)
{
    int Finished = 0;
    unsigned char id, ucHigh, ucLow;
    FILE *pfRead;

    *punWidth = 0;
    *punHeight = 0;

    if ((pfRead = fopen(path, "rb")) == 0)
    {
        //printf("[GetJPEGWidthHeight]:can't open file:%s\n", path);
        return -1;
    }

    while (!Finished)
    {
        if (!fread(&id, sizeof(char), 1, pfRead) || id != 0xff || !fread(&id, sizeof(char), 1, pfRead))
        {
            Finished = -2;
            break;
        }

        if (id >= M_APP0 && id <= M_APPF)
        {
            fread(&ucHigh, sizeof(char), 1, pfRead);
            fread(&ucLow, sizeof(char), 1, pfRead);
            fseek(pfRead, (long)(MAKEUS(ucHigh, ucLow) - 2), SEEK_CUR);
            continue;
        }

        switch (id)
        {
        case M_SOI:
            break;

        case M_COM:
        case M_DQT:
        case M_DHT:
        case M_DNL:
        case M_DRI:
            fread(&ucHigh, sizeof(char), 1, pfRead);
            fread(&ucLow, sizeof(char), 1, pfRead);
            fseek(pfRead, (long)(MAKEUS(ucHigh, ucLow) - 2), SEEK_CUR);
            break;

        case M_SOF0:
            fseek(pfRead, 3L, SEEK_CUR);
            fread(&ucHigh, sizeof(char), 1, pfRead);
            fread(&ucLow, sizeof(char), 1, pfRead);
            *punHeight = (unsigned int)MAKEUS(ucHigh, ucLow);
            fread(&ucHigh, sizeof(char), 1, pfRead);
            fread(&ucLow, sizeof(char), 1, pfRead);
            *punWidth = (unsigned int)MAKEUS(ucHigh, ucLow);
            return 0;

        case M_SOS:
        case M_EOI:
        case M_DATA:
            Finished = -1;
            break;

        default:
            fread(&ucHigh, sizeof(char), 1, pfRead);
            fread(&ucLow, sizeof(char), 1, pfRead);
            //printf("[GetJPEGWidthHeight]:unknown id: 0x%x ;  length=%hd\n", id, MAKEUS(ucHigh, ucLow));
            if (fseek(pfRead, (long)(MAKEUS(ucHigh, ucLow) - 2), SEEK_CUR) != 0)
                Finished = -2;
            break;
        }
    }

#if 0
    if (Finished == -1)
        printf("[GetJPEGWidthHeight]:can't find SOF0!\n");
    else if (Finished == -2)
        printf("[GetJPEGWidthHeight]:jpeg format error!\n");
#endif
    return -1;
}
