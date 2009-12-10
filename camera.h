
#ifndef CAMERA_H
#define CAMERA_H

#include <inttypes.h>

// camera functions
void camera_init(void);
void camera_takephoto(const char * fname);
void camera_txbyte(char c);
char camera_response(void);
void camera_rcv_cmd(unsigned char * cmdbuf);
void camera_rcv(unsigned char * buf, int n);
void camera_snd_cmd(char cmd, char b3, char b4, char b5, char b6);

#endif
