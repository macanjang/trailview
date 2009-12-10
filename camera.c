#include <avr/io.h>
#include "camera.h"
#include "lcd.h"
#include "fat32.h"
#include "camera.h"

#define F_CPU 8E6
#include <util/delay.h>
#include <avr/interrupt.h>

#define CAMERA_SYNC 0x0d
#define CAMERA_ACK 0x0e
#define CAMERA_INITIAL 0x01
#define CAMERA_PKGSIZE 0x06
#define CAMERA_SNAPSHOT 0x05
#define CAMERA_GETPIC 0x04

#define CAMERA_DATAREADY() (camera_readpos != camera_writepos)
#define CAMERA_READBYTE() (camera_buf[camera_readpos++])
volatile unsigned char camera_readpos, camera_writepos;
volatile char camera_buf[256];

void camera_init(void)
{
	UBRR1H = 0;
	UBRR1L = 34; // BAUD FOR 8MHZ SYSTEM CLOCK
	UCSR1C = (3<<UCSZ10);  // 8 BIT NO PARITY 1 STOP
	UCSR1B = (1<<RXCIE1)|(1<<RXEN1)|(1<<TXEN1); // ENABLE TX AND RX ALSO 8 BIT and INTERRUPT

	// setup interrupts
	sei();

	// initialization sequence
	char cmdbuf[6];
	camera_readpos = 0;
	camera_writepos = 0;
	
	lcd_printf("camera: init\n");

	int i = 61;
	do {
		camera_snd_cmd(CAMERA_SYNC, 0, 0, 0, 0);
		_delay_ms(50);
	} while (!camera_response() && --i);

	// no response
	if (!i) {
		lcd_printf("camera error:\ninit timeout");
		while (1) ;
		return;
	}
	
	
	// get ack and sync, send back ack
	camera_rcv_cmd(cmdbuf);
	camera_rcv_cmd(cmdbuf);

	if (cmdbuf[1] != CAMERA_SYNC) {
		lcd_printf("camera error:\nmissing sync");
		while (1) ;
		return;
	} else {
		lcd_printf("camera synced!\n");
	}

	camera_snd_cmd(CAMERA_ACK, CAMERA_SYNC, 0, 0, 0);
	
	// set picture settings
	lcd_printf("camera: set");
	camera_snd_cmd(CAMERA_INITIAL, 0, 0x07, 0x03, 0x07); // JPEG, 640 x 480
	camera_rcv_cmd(cmdbuf);

	lcd_printf("camera: pkgsize");
	camera_snd_cmd(CAMERA_PKGSIZE, 0x08, 0x80, 0x00, 0x00); // pkg size 128 bytes
	camera_rcv_cmd(cmdbuf);
}

void camera_takephoto(const char * fname)
{
	struct fatwrite_t fwrite;
	unsigned char pbuf[512];
	char cmdbuf[6];

		
	lcd_printf("Taking Photo\n");
	
	// take photo
	camera_snd_cmd(CAMERA_SNAPSHOT, 0, 0, 0, 0);
	camera_rcv_cmd(cmdbuf);
	_delay_ms(500);
	
	lcd_printf("Getting Size\n");

	// get snapshot and size
	camera_snd_cmd(CAMERA_GETPIC, 0x01, 0, 0, 0);
	camera_rcv_cmd(cmdbuf);
	camera_rcv_cmd(cmdbuf);

	uint32_t psize = cmdbuf[3] | (((uint32_t)cmdbuf[4])<<8) | (((uint32_t)cmdbuf[5])<<16);
	int packets = psize/(128-6);

	lcd_printf("Packets:\n%d", packets);
	_delay_ms(1000);
	
	// create file	
	del(fname);
	touch(fname);
	write_start(fname, &fwrite);
	
	lcd_printf("Getting Packets");
	lcd_go_line(1);

	// receive packets
	unsigned int i, j, k, tmpsize;
	char c, flag = 0, ff_flag = 0;
	
	for (i=0; i<packets; i++) {
		_delay_ms(10);
		lcd_print_int(i);
		camera_snd_cmd(CAMERA_ACK, 0, 0, i&0xff, (i>>8)&0xff);
		
		//camera_rxbyte();
		//camera_rxbyte();

		//c = camera_rxbyte();
		//c1 = camera_rxbyte();
		c = 0;
		c1 = 0;

		tmpsize = (unsigned int)c1 | (((unsigned int)c)<<8);

		j=0;
		for (k=0; k<512-6; k++) {
			//c = camera_rxbyte();

			/*if (flag == 1) */pbuf[j++] = c;
		
			// only record between start and end jpeg markers
			/*if (ff_flag) {
				if (pbuf[j] == 0xd9) flag = 2;
				else if (pbuf[j] == 0xd8) {
					flag = 1;
					pbuf[j++] = 0xff;
					pbuf[j++] = 0xd8;
				}
				ff_flag = 0;
			}
			if (pbuf[j] == 0xff) ff_flag = 1;*/
		}

		//camera_rxbyte();
		//camera_rxbyte();
		
		write_add(&fwrite, (char *)pbuf, j);
	}
	
	lcd_printf("Finished");
	
	// finish
	camera_snd_cmd(CAMERA_ACK, 0, 0, 0xf0, 0xf0);
	write_end(&fwrite);
}

void camera_txbyte(char c)
{
	while ((UCSR1A&(1<<UDRE1)) == 0); // wait until empty
	UDR1 = c;
}

char camera_response(void)
{
	if (camera_writepos - camera_readpos >= 6) return 1;
	return 0;
}

void camera_rcv_cmd(char * cmdbuf)
{
	lcd_go_line(1);
	int i;
	for (i=0; i<6; i++) {
		while (!CAMERA_DATAREADY()) ;
		cmdbuf[i] = CAMERA_READBYTE();
		lcd_print_hex(cmdbuf[i]);
	}
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

ISR(USART1_RX_vect)
{
	camera_buf[camera_writepos++] = UDR1;
}

