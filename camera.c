#include <avr/io.h>
#include "camera.h"
#include "lcd.h"
#include "fat32.h"
#include "camera.h"

#define F_CPU 8E6
#include <util/delay.h>
#include <avr/interrupt.h>

// camera message/command types
#define CAMERA_SYNC 0x0d
#define CAMERA_ACK 0x0e
#define CAMERA_INITIAL 0x01
#define CAMERA_PKGSIZE 0x06
#define CAMERA_SNAPSHOT 0x05
#define CAMERA_GETPIC 0x04
#define CAMERA_DATA 0x0a

// interrupt populated circular buffer
#define CAMERA_DATAREADY() (camera_readpos != camera_writepos)
#define CAMERA_READBYTE() (camera_buf[camera_readpos++])
volatile unsigned char camera_readpos, camera_writepos;
volatile char camera_buf[256];

// camera initialization routine
void camera_init(void)
{
	UBRR1H = 0;
	UBRR1L = 34; // BAUD FOR 8MHZ SYSTEM CLOCK
	UCSR1C = (3<<UCSZ10);  // 8 BIT NO PARITY 1 STOP
	UCSR1B = (1<<RXCIE1)|(1<<RXEN1)|(1<<TXEN1); // ENABLE TX AND RX ALSO 8 BIT and INTERRUPT

	// setup interrupts
	sei();

	// initialization sequence
	unsigned char cmdbuf[6];
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
		lcd_printf("camera: synced!\n");
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

// takes a photo and saves it as fname in the current directory
void camera_takephoto(const char * fname)
{
	struct fatwrite_t fwrite;
	unsigned char pbuf[128];
	unsigned char cmdbuf[6];
	uint32_t psize;
	unsigned int packets = 0, load_bar = 0;

		
	lcd_printf("camera:\nTaking Photo");
	
	// clear buffer
	camera_readpos = camera_writepos;

	// take photo
	camera_snd_cmd(CAMERA_SNAPSHOT, 0, 0, 0, 0);
	camera_rcv_cmd(cmdbuf);
	_delay_ms(100);

	// get snapshot and size
	camera_snd_cmd(CAMERA_GETPIC, 0x01, 0, 0, 0);
	camera_rcv_cmd(cmdbuf);
	camera_rcv_cmd(cmdbuf);

	psize = cmdbuf[3] | (((uint32_t)cmdbuf[4])<<8) | (((uint32_t)cmdbuf[5])<<16);
	packets = psize/(128-6);
	load_bar = packets/16;
	
	// create file
	del(fname);
	touch(fname);
	write_start(fname, &fwrite);
	
	lcd_printf("Saving: %dkB\n", (unsigned int)(psize/1024));
	lcd_go_line(1);

	// receive packets
	unsigned int i, packet_size, packet_id;
	
	for (i=0; i<packets; i++) {
		_delay_ms(10);
		
		// draw progress bar
		if (!(i%load_bar)) lcd_wdata('=');
		//lcd_go_line(1);
		//lcd_print_int(i);

		// request next packet
		camera_snd_cmd(CAMERA_ACK, 0, 0, i&0xff, (i>>8)&0xff);
		camera_rcv(pbuf, 128);

		// get packet id and size from packet
		packet_id = (unsigned int)pbuf[0] | (((unsigned int)pbuf[1])<<8);
		packet_size = (unsigned int)pbuf[2] | (((unsigned int)pbuf[3])<<8);
		
		// error out if we lose a packet
		if (packet_id != i) {
			lcd_printf("camera error:\npacket %d!=%d", packet_id, i);
			while (1) ;
		}
		
		// write to file
		write_add(&fwrite, (char *)(pbuf+4), packet_size);
	}
	
	lcd_printf("camera: saved\n");
	
	// finish
	camera_snd_cmd(CAMERA_ACK, 0, 0, 0xf0, 0xf0);
	write_end(&fwrite);
}

// send a single byte to the camera
void camera_txbyte(char c)
{
	while ((UCSR1A&(1<<UDRE1)) == 0); // wait until empty
	UDR1 = c;
}

// check if there is an unread response from the camera
char camera_response(void)
{
	if (camera_writepos - camera_readpos >= 6) return 1;
	return 0;
}

// receive a packet of data from the camera
void camera_rcv(unsigned char * buf, int n)
{
	int i;
	for (i=0; i<n; i++) {
		while (!CAMERA_DATAREADY()) ;
		buf[i] = CAMERA_READBYTE();
	}
}

// receive a command from the camera
void camera_rcv_cmd(unsigned char * cmdbuf)
{
	lcd_go_line(1);
	int i;
	for (i=0; i<6; i++) {
		while (!CAMERA_DATAREADY()) ;
		cmdbuf[i] = CAMERA_READBYTE();
		//lcd_print_hex(cmdbuf[i]);
	}
}

// send a command to the camera
void camera_snd_cmd(char cmd, char b3, char b4, char b5, char b6)
{
	camera_txbyte(0xaa);
	camera_txbyte(cmd);
	camera_txbyte(b3);
	camera_txbyte(b4);
	camera_txbyte(b5);
	camera_txbyte(b6);
}

// interrupt service routine that listens to the camera
ISR(USART1_RX_vect)
{
	camera_buf[camera_writepos++] = UDR1;
}

