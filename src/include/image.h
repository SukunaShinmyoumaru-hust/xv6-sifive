#ifndef IMAGE_H
#define IMAGE_H
#include"fat32.h"
#include"buf.h"

void image_init(struct dirent* img);
void image_read(struct buf *b,struct dirent* img);
void image_write(struct buf *b,struct dirent* img);

#endif
