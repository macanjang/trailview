
#ifndef CAMERA_H
#define CAMERA_H

#include <inttypes.h>

// camera functions
void camera_init(void);
void camera_takephoto(const char * fname);
void camera_txbyte(char c);
inline char camera_rxbyte(void);
char camera_response(void);
void camera_rcv_cmd(char * cmdbuf);
void camera_snd_cmd(char b2, char b3, char b4, char b5, char b6);
void camera_snd_sync(void);
void camera_rcv_ack(void);

#endif
