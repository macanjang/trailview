#include <avr/io.h>
#include "camera.h"
#include "lcd.h"
#include "fat32.h"

#define CAMERA_CHECKCMD(cmd, buf) (buf[0] == 0xaa && buf[1] == 0x)
#define CAMERA_SYNC 0x0d
#define CAMERA_ACK 0x0e
#define CAMERA_INITIAL 0x01
#define CAMERA_PKGSIZE 0x06
#define CAMERA_SNAPSHOT 0x05
#define CAMERA_GETPIC 0x04

void init_camera(void)
{
	UBRR1H = 0;
	UBRR1L = 34; // 19.2k BAUD FOR 8MHZ SYSTEM CLOCK
	UCSR1C = (3<<UCSZ10);  // 8 BIT NO PARITY 1 STOP
	UCSR1B = (1<<RXEN1)|(1<<TXEN1); // ENABLE TX AND RX ALSO 8 BIT

	// initialization sequence
	char cmdbuf[6];

	int i = 75;
	do {
		camera_snd_cmd(CAMERA_SYNC, 0, 0, 0, 0);
	} while (!camera_response() && --i);

	// no response
	if (!i) {
		lcd_printf("camera error:\ninit timeout");
		return;
	}

	// get ack
	camera_rcv_cmd(cmdbuf);

	// bad response
	if (!CAMERA_CHECKCMD(CAMERA_ACK, cmdbuf)) {
		lcd_printf("camera error:\nbad init ack");
		return;
	}

	// get sync, send ack
	camera_rcv_cmd(cmdbuf);

	if (!CAMERA_CHECKCMD(CAMERA_SYNC, cmdbuf)) {
		lcd_printf("camera error:\nmissing sync");
		return;
	}

	camera_snd_cmd(CAMERA_ACK, CAMERA_SYNC, 0, 0, 0);

	// set picture settings
	camera_snd_cmd(CAMERA_INITIAL, 0, 0x07, 0x03, 0x07); // JPEG, 640 x 480
	camera_rcv_cmd(cmdbuf);

	camera_snd_cmd(CAMERA_PKGSIZE, 0x08, 0x00, 0x02, 0x00); // pkg size 512 bytes
	camera_rcv_cmd(cmdbuf);
}

void camera_takephoto(const char * fname)
{
	struct fatwrite_t fwrite;
	unsigned char pbuf[512];

	// take photo
	camera_snd_cmd(CAMERA_SNAPSHOT, 0, 0, 0, 0);
	camera_rcv_cmd(cmdbuf);

	// get snapshot and size
	camera_snd_cmd(CAMERA_GETPIC, 0x01, 0, 0, 0);
	camera_rcv_cmd(cmdbuf);
	camera_rcv_cmd(cmdbuf);

	uint32_t psize = cmdbuf[3] | (cmdbuf[4]<<8) | (cmdbuf[5]<<16);
	int packets = psize/(512-6);

	// create file	
	del(fname);
	touch(fname);
	write_start(fname, &fwrite);

	// receive packets
	unsigned int i, j, tmpsize;
	for (i=0; i<packets; i++) {
		camera_snd_cmd(CAMERA_ACK, 0, 0, i&0xff, (i>>8)&0xff);
		
		for (j=0; j<256; j++) {
			pbuf[j] = camera_rxbyte();
		}

		tmpsize = (unsigned int)pbuf[2] | (((unsigned int)pbuf[3])<<8);
		write_add(&fwrite, pbuf+4, tmpsize);
	}
	
	camera_snd_cmd(CAMERA_ACK, 0, 0, 0xf0, 0xf0);

	write_end(&fwrite);
}

void camera_txbyte(char c)
{
	while ((UCSR1A&(1<<UDRE1)) == 0); // wait until empty
	UDR0 = c;
}

char camera_rxbyte(void)
{
	while ((UCSR1A&(1<<RXC1)) == 0);  // wait for char
	return = UDR1;
}

char camera_response(void)
{
	if (UCSR1A&(1<<RXC1)) return 1;
	else return 0;
}

void camera_rcv_cmd(char * cmdbuf)
{
	char i;
	for (i=0; i<6; i++)
		cmdbuf[i] = camera_rxbyte();
}

void camera_snd_cmd(char b2, char b3, char b4, char b5, char b6)
{
	camera_txbyte(0xaa);
	camera_txbyte(b2);
	camera_txbyte(b3);
	camera_txbyte(b4);
	camera_txbyte(b5);
	camera_txbyte(b6);
}

