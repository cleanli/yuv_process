#ifndef J_H
#define J_H
int GetJPEGWidthHeight(const char* path, unsigned int *punWidth, unsigned int *punHeight);
int get_buffer_size(int width, int height);
int read_jpeg_file(char*inputbuf, int *pw, int *ph, const char* filename);
void write_jpeg_file(char*outputbuf, int w, int h, int q, const char* filename);
#endif
