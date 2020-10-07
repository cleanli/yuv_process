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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include "jpeglib.h"

static char* g_buffer=0;		/* Output row buffer */
static char* g_sub_buffer=0;
static JSAMPLE * write_image_buffer = 0;	/* Points to large array of R,G,B-order data */
static int g_image_height;	/* Number of rows in image */
static int g_image_width;		/* Number of columns in image */
static int g_sub_image_height;	/* Number of rows in image */
static int g_sub_image_width;		/* Number of columns in image */
static int g_sub_buffer_size = 0;
static int g_buffer_size = 0;
static int g_divided = 0;
static int g_parts = 1;
static int g_quality = 90;
/*
 * ERROR HANDLING:
 *
 * The JPEG library's standard error handler (jerror.c) is divided into
 * several "methods" which you can override individually.  This lets you
 * adjust the behavior without duplicating a lot of code, which you might
 * have to update with each future release.
 *
 * Our example here shows how to override the "error_exit" method so that
 * control is returned to the library's caller when a fatal error occurs,
 * rather than calling exit() as the standard error_exit method does.
 *
 * We use C's setjmp/longjmp facility to return control.  This means that the
 * routine which calls the JPEG library must first execute a setjmp() call to
 * establish the return point.  We want the replacement error_exit to do a
 * longjmp().  But we need to make the setjmp buffer accessible to the
 * error_exit routine.  To do this, we make a private extension of the
 * standard JPEG error handler object.  (If we were using C++, we'd say we
 * were making a subclass of the regular error handler.)
 *
 * Here's the extended error handler struct:
 */

struct my_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

/*
 * Here's the routine that will replace the standard error_exit method:
 */

METHODDEF(void)
my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}

#define JPEG_CROP_RET_OK 0
#define JPEG_CROP_RET_ERR (-1)
#define COMPONENTS_PER_POINT 3
#define TO_EVEN(X) (((X)+1)/2*2)
int buffer_init(int width, int height)
{
  if(g_image_height == height && g_image_width == width){
    return JPEG_CROP_RET_OK;
  }
  g_image_height = height;
  g_image_width = width;
  g_buffer_size = g_image_width * g_image_height;
  g_buffer_size += TO_EVEN(g_image_width) * TO_EVEN(g_image_height)/2;
  if(g_buffer){
    free(g_buffer);
  }
  if(width == 0 || height == 0){
    g_buffer = 0;
    return JPEG_CROP_RET_OK;
  }
  g_buffer =(char*) malloc(g_image_width * g_image_height * COMPONENTS_PER_POINT);
  if(!g_buffer){
	  printf("error, malloc failed\n");
	  return JPEG_CROP_RET_ERR;
  }
  printf("g_buffer %lx alloced\n", (long)g_buffer);
  return JPEG_CROP_RET_OK;
}

void buffer_uninit()
{
  if(g_buffer){
    free(g_buffer); 
    g_buffer = 0;
  } 
}

int read_jpeg_file(const char* filename)
{
  /* This struct contains the JPEG decompression parameters and pointers to
   * working space (which is allocated as needed by the JPEG library).
   */
  struct jpeg_decompress_struct cinfo;
  /* We use our private extension JPEG error handler.
   * Note that this struct must live as long as the main JPEG parameter
   * struct, to avoid dangling-pointer problems.
   */
  struct my_error_mgr jerr;
  /* More stuff */
  FILE * infile;		/* source file */
  JSAMPARRAY buffer;		/* Output row buffer */
  int row_stride;		/* physical row width in output buffer */
  char* dst_buffer;

  /* In this example we want to open the input file before doing anything else,
   * so that the setjmp() error recovery below can assume the file is open.
   * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
   * requires it in order to read binary files.
   */

  if ((infile = fopen(filename, "rb")) == NULL) {
    fprintf(stderr, "can't open %s\n", filename);
    return 0;
  }
  else{
      printf("open %s successfully\n", filename);
  }

  /* Step 1: allocate and initialize JPEG decompression object */

  cinfo.out_color_space = JCS_YCbCr;
  /* We set up the normal JPEG error routines, then override error_exit. */
  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = my_error_exit;
  /* Establish the setjmp return context for my_error_exit to use. */
  if (setjmp(jerr.setjmp_buffer)) {
    /* If we get here, the JPEG code has signaled an error.
     * We need to clean up the JPEG object, close the input file, and return.
     */
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);
    return 0;
  }
  /* Now we can initialize the JPEG decompression object. */
  jpeg_create_decompress(&cinfo);

  /* Step 2: specify data source (eg, a file) */

  jpeg_stdio_src(&cinfo, infile);

  /* Step 3: read file parameters with jpeg_read_header() */

  (void) jpeg_read_header(&cinfo, TRUE);
  /* We can ignore the return value from jpeg_read_header since
   *   (a) suspension is not possible with the stdio data source, and
   *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
   * See libjpeg.doc for more info.
   */

  /* Step 4: set parameters for decompression */

  /* In this example, we don't need to change any of the defaults set by
   * jpeg_read_header(), so we do nothing here.
   */

  /* Step 5: Start decompressor */

  (void) jpeg_start_decompress(&cinfo);
  /* We can ignore the return value since suspension is not possible
   * with the stdio data source.
   */

  /* We may need to do some setup of our own at this point before reading
   * the data.  After jpeg_start_decompress() we have the correct scaled
   * output image dimensions available, as well as the output colormap
   * if we asked for color quantization.
   * In this example, we need to make an output work buffer of the right size.
   */ 
  /* JSAMPLEs per row in output buffer */
  buffer_init(cinfo.output_width, cinfo.output_height);
  printf("read wxh = %dx%d\n", g_image_width, g_image_height);
  row_stride = cinfo.output_width * cinfo.output_components;
  printf("row_stride is %d\n", row_stride);
  dst_buffer = g_buffer;
  /* Make a one-row-high sample array that will go away when done with image */
  buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

  printf("code run %d line\n", __LINE__);
  /* Step 6: while (scan lines remain to be read) */
  /*           jpeg_read_scanlines(...); */

  /* Here we use the library's state variable cinfo.output_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   */
  while (cinfo.output_scanline < cinfo.output_height) {
    /* jpeg_read_scanlines expects an array of pointers to scanlines.
     * Here the array is only one element long, but you could ask for
     * more than one scanline at a time if that's more convenient.
     */
    (void) jpeg_read_scanlines(&cinfo, buffer, 1);
    //printf("scan %d\n", cinfo.output_scanline);
    /* Assume put_scanline_someplace wants a pointer and sample count. */
    //memcpy(dst_buffer, (char*)(*buffer), row_stride);
	int index = 0;
    char* p_in_buffer = (char*)(*buffer);
	for (int i = 0; i < g_image_width; i ++){
		dst_buffer[index++] = *p_in_buffer;
        //output from jpeg decompress is YUV444, Y,U,V,Y,U,V...
		p_in_buffer += 3;
	}
    dst_buffer += g_image_width;
/*
	while (cinfo.next_scanline < cinfo.image_height) {
		int index = 0;
		for (i = 0; i < width; i += 2){
			yuvbuf[index++] = *pY;
			yuvbuf[index++] = *pU;
			yuvbuf[index++] = *pV;
			pY += 2;
			yuvbuf[index++] = *pY;
			yuvbuf[index++] = *pU;
			yuvbuf[index++] = *pV;
			pY += 2;
			pU += 4;
			pV += 4;
		}
		row_pointer[0] = yuvbuf;
		(void)jpeg_write_scanlines(&cinfo, row_pointer, 1);// single line compress
		j++;
	}
*/
  }
  memset(g_buffer+g_image_width*g_image_height, 0x80, g_buffer_size-g_image_width*g_image_height);
  printf("code run %d line\n", __LINE__);

  /* Step 7: Finish decompression */

  (void) jpeg_finish_decompress(&cinfo);
  /* We can ignore the return value since suspension is not possible
   * with the stdio data source.
   */

  /* Step 8: Release JPEG decompression object */

  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_decompress(&cinfo);

  /* After finish_decompress, we can close the input file.
   * Here we postpone it until after no more JPEG errors are possible,
   * so as to simplify the setjmp error logic above.  (Actually, I don't
   * think that jpeg_destroy can do an error exit, but why assume anything...)
   */
  fclose(infile);

  FILE* fpw;
  if ((fpw = fopen("testout.yuv", "wb")) == NULL) {
    fprintf(stderr, "can't open %s\n", "out");
  }
  else{
    fwrite(g_buffer, g_buffer_size, 1, fpw);
    fclose(fpw);
  }
  buffer_uninit();
  printf("code run %d line\n", __LINE__);
  /* At this point you may want to check to see whether any corrupt-data
   * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
   */

  /* And we're done! */
  return 1;
}

int GetJPEGWidthHeight(const char* path, unsigned int *punWidth, unsigned int *punHeight)
{
    int Finished = 0;
    unsigned char id, ucHigh, ucLow;
    FILE *pfRead;

    *punWidth = 0;
    *punHeight = 0;

    read_jpeg_file(path);
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
