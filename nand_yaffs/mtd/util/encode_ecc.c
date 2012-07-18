#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <getopt.h>
#include <errno.h>

#include <asm/types.h>

#define TenBit_Mask 0x3ff

unsigned char data_buf[518] = {
0x18, 0x00, 0x00, 0xb8, 0x01, 0x00, 0x00, 0x00, 0x04, 0x01, 0x00, 0xb8, 0x05, 0x00, 0x00, 0x00,
0x00, 0x01, 0x00, 0xb8, 0xb0, 0xe3, 0x04, 0x00, 0x04, 0x01, 0x00, 0xb8, 0x04, 0x00, 0x00, 0x00,
0x04, 0x01, 0x00, 0xb8, 0x06, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
0x04, 0x01, 0x00, 0xb8, 0x02, 0x00, 0x00, 0x00, 0x6c, 0x01, 0x00, 0xb8, 0x05, 0x00, 0x00, 0x00,
0x60, 0x00, 0x00, 0xb8, 0x00, 0x00, 0x7f, 0xff, 0x64, 0x01, 0x00, 0xb8, 0x2a, 0xbf, 0x5d, 0x02,
0x6c, 0x01, 0x00, 0xb8, 0x04, 0x00, 0x00, 0x00, 0x6c, 0x01, 0x00, 0xb8, 0x06, 0x00, 0x00, 0x00,
0x03, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x6c, 0x01, 0x00, 0xb8, 0x02, 0x00, 0x00, 0x00,
0x03, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x08, 0xa8, 0x01, 0xb8, 0x03, 0x00, 0x03, 0x01,
0x14, 0xa8, 0x01, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0xb8, 0x00, 0x00, 0x00, 0x00,
0x00, 0x06, 0x00, 0xb8, 0x00, 0x40, 0x02, 0x00, 0x04, 0x06, 0x00, 0xb8, 0x00, 0x02, 0x02, 0x00,
0x00, 0x06, 0x00, 0xb8, 0x01, 0x40, 0x02, 0x00, 0x81, 0x06, 0x00, 0xb8, 0x01, 0x00, 0x00, 0x00,
0x01, 0x00, 0x00, 0x00, 0x35, 0x06, 0x00, 0xb8, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
0x04, 0x06, 0x00, 0xb8, 0x01, 0x02, 0x02, 0x00, 0x00, 0x06, 0x00, 0xb8, 0x00, 0x40, 0x02, 0x00,
0x00, 0x06, 0x00, 0xb8, 0x01, 0x40, 0x02, 0x00, 0x18, 0x01, 0x00, 0xb8, 0x0a, 0x72, 0x73, 0x00,
0x24, 0x01, 0x00, 0xb8, 0x04, 0x00, 0x00, 0x00, 0x24, 0x01, 0x00, 0xb8, 0x06, 0x00, 0x00, 0x00,
0x03, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x24, 0x01, 0x00, 0xb8, 0x02, 0x00, 0x00, 0x00,

0x30, 0x01, 0x00, 0xb8, 0x70, 0xa6, 0x00, 0x01, 0x34, 0x01, 0x00, 0xb8, 0x04, 0x00, 0x00, 0x00,
0x34, 0x01, 0x00, 0xb8, 0x06, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00,
0x34, 0x01, 0x00, 0xb8, 0x02, 0x00, 0x00, 0x00, 0x28, 0x01, 0x00, 0xb8, 0x14, 0x00, 0x00, 0x00,
0x2c, 0x01, 0x00, 0xb8, 0x03, 0x00, 0x00, 0x00, 0x2c, 0x01, 0x00, 0xb8, 0x01, 0x00, 0x00, 0x00,
0x08, 0x01, 0x00, 0xb8, 0xb0, 0xbb, 0x04, 0x00, 0x0c, 0x01, 0x00, 0xb8, 0x04, 0x00, 0x00, 0x00,
0x0c, 0x01, 0x00, 0xb8, 0x06, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00,
0x0c, 0x01, 0x00, 0xb8, 0x02, 0x00, 0x00, 0x00, 0x10, 0x01, 0x00, 0xb8, 0x70, 0x8b, 0x04, 0x00,
0x14, 0x01, 0x00, 0xb8, 0x04, 0x00, 0x00, 0x00, 0x14, 0x01, 0x00, 0xb8, 0x06, 0x00, 0x00, 0x00,
0x03, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x14, 0x01, 0x00, 0xb8, 0x02, 0x00, 0x00, 0x00,
0x54, 0x01, 0x00, 0xb8, 0x44, 0xe5, 0xb8, 0x4b, 0x60, 0x01, 0x00, 0xb8, 0x04, 0x00, 0x00, 0x00,
0x60, 0x01, 0x00, 0xb8, 0x06, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
0x64, 0x00, 0x00, 0xb8, 0x00, 0x00, 0x7f, 0xbf, 0x60, 0x01, 0x00, 0xb8, 0x02, 0x00, 0x00, 0x00,
0x03, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0xb8, 0x2d, 0xff, 0xff, 0x01,
0x10, 0x00, 0x00, 0xb8, 0xce, 0x01, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00,
0x0c, 0x00, 0x00, 0xb8, 0x01, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xb8, 0x00, 0x00, 0x00, 0x00,
0x03, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb8, 0xFF, 0xf1, 0xFF, 0x7f,

0x23, 0x00, 0x00, 0x00, 0xff, 0xff
};
	
unsigned short int GO7=0, GO6=0, GO5=0, GO4=0, GO3=0, GO2=0, GO1=0, GO0=0;
unsigned short int GO70=0, GO60=0, GO50=0, GO40=0, GO30=0, GO20=0, GO10=0, GO00=0;

unsigned short int inline ToBit( unsigned short int value)
{
	value = value & 0x0001;
	return value;
}


void encode_ecc_byte(unsigned short int input)
{
	unsigned short int xa540 , xa385 , xa398 , xa607 , xa407 , xa403 , xa567 , xa36;
	unsigned short int I;

	printf("before: input=%x, GO7=%x, GO6=%x, GO5=%x, GO4=%x, GO3=%x, GO2=%x, GO1=%x, GO0=%x\n",
		input, GO7, GO6, GO5, GO4, GO3, GO2, GO1, GO0);
	
	//printf("I=0x%x\n", I);
	I = input ^ GO7;
	I = I & TenBit_Mask;
	printf("I=0x%x\n", I);
	
	xa36 = ( ToBit(I>>1) ^ ToBit(I>>2) ^ ToBit(I>>4) ^ ToBit(I>>9) ) <<0 |
                ( ToBit(I>>2) ^ ToBit(I>>3) ^ ToBit(I>>5) ) <<1 |
                ( ToBit(I>>0) ^ ToBit(I>>3) ^ ToBit(I>>4) ^ ToBit(I>>6) ) << 2 |
                ( ToBit(I>>2) ^ ToBit(I>>5) ^ ToBit(I>>7) ^ ToBit(I>>9) ) << 3 |
                ( ToBit(I>>3) ^ ToBit(I>>6) ^ ToBit(I>>8) ) <<4 |
                ( ToBit(I>>4) ^ ToBit(I>>7) ^ ToBit(I>>9) ) <<5 |
                ( ToBit(I>>0) ^ ToBit(I>>5) ^ ToBit(I>>8) ) <<6 |
                ( ToBit(I>>1) ^ ToBit(I>>6) ^ ToBit(I>>9) ) <<7 |
                ( ToBit(I>>0) ^ ToBit(I>>2) ^ ToBit(I>>7) ) <<8 |
                ( ToBit(I>>0) ^ ToBit(I>>1) ^ ToBit(I>>3) ^ ToBit(I>>8) ) <<9;
	/*
	printf("1= %x\n", ToBit(I>>1) ^ ToBit(I>>2) ^ ToBit(I>>4) ^ ToBit(I>>9));
	printf("2= %x\n", ToBit(I>>2) ^ ToBit(I>>3) ^ ToBit(I>>5));
	printf("3= %x\n", ToBit(I>>0) ^ ToBit(I>>3) ^ ToBit(I>>4) ^ ToBit(I>>6) );
	printf("4= %x\n", ToBit(I>>2) ^ ToBit(I>>5) ^ ToBit(I>>7) ^ ToBit(I>>9) );
	printf("5= %x\n", ToBit(I>>3) ^ ToBit(I>>6) ^ ToBit(I>>8));
	printf("6= %x\n", ToBit(I>>4) ^ ToBit(I>>7) ^ ToBit(I>>9));
	printf("7= %x\n", ToBit(I>>0) ^ ToBit(I>>5) ^ ToBit(I>>8));
	printf("8= %x\n", ToBit(I>>1) ^ ToBit(I>>6) ^ ToBit(I>>9));
	printf("9= %x\n", ToBit(I>>0) ^ ToBit(I>>2) ^ ToBit(I>>7));
	printf("10= %x\n", ToBit(I>>0) ^ ToBit(I>>1) ^ ToBit(I>>3) ^ ToBit(I>>8));
	*/
	xa36 = xa36 & TenBit_Mask;
	//printf("xa36=0x%x\n", xa36);


	xa567 = ( ToBit(I>>0) ^ ToBit(I>>1) ^ ToBit(I>>4) ^ ToBit(I>>7) ^ ToBit(I>>8) ^ ToBit(I>>9) ) <<0 |
                ( ToBit(I>>0) ^ ToBit(I>>1) ^ ToBit(I>>2) ^ ToBit(I>>5) ^ ToBit(I>>8) ^ ToBit(I>>9) ) <<1 |
                ( ToBit(I>>1) ^ ToBit(I>>2) ^ ToBit(I>>3) ^ ToBit(I>>6) ^ ToBit(I>>9) ) <<2 |
                ( ToBit(I>>0) ^ ToBit(I>>1) ^ ToBit(I>>2) ^ ToBit(I>>3) ^ ToBit(I>>8) ^ ToBit(I>>9) ) <<3 |
                ( ToBit(I>>1) ^ ToBit(I>>2) ^ ToBit(I>>3) ^ ToBit(I>>4) ^ ToBit(I>>9) ) <<4 |
                ( ToBit(I>>2) ^ ToBit(I>>3) ^ ToBit(I>>4) ^ ToBit(I>>5) ) <<5 |
                ( ToBit(I>>0) ^ ToBit(I>>3) ^ ToBit(I>>4) ^ ToBit(I>>5) ^ ToBit(I>>6) ) <<6 |
                ( ToBit(I>>1) ^ ToBit(I>>4) ^ ToBit(I>>5) ^ ToBit(I>>6) ^ ToBit(I>>7) ) <<7 |
                ( ToBit(I>>2) ^ ToBit(I>>5) ^ ToBit(I>>6) ^ ToBit(I>>7) ^ ToBit(I>>8) ) <<8 |
                ( ToBit(I>>0) ^ ToBit(I>>3) ^ ToBit(I>>6) ^ ToBit(I>>7) ^ ToBit(I>>8) ^ ToBit(I>>9) ) <<9;
	xa567 = xa567 & TenBit_Mask;
	//printf("xa567=0x%x\n", xa567);

	xa403 = ( ToBit(I>>5) ^ ToBit(I>>6) ^ ToBit(I>>7) ^ ToBit(I>>9) ) <<0 |
                ( ToBit(I>>0) ^ ToBit(I>>6) ^ ToBit(I>>7) ^ ToBit(I>>8) ) <<1 |
                ( ToBit(I>>1) ^ ToBit(I>>7) ^ ToBit(I>>8) ^ ToBit(I>>9) ) <<2 |
                ( ToBit(I>>0) ^ ToBit(I>>2) ^ ToBit(I>>5) ^ ToBit(I>>6) ^ ToBit(I>>7) ^ ToBit(I>>8) ) <<3 |
                ( ToBit(I>>0) ^ ToBit(I>>1) ^ ToBit(I>>3) ^ ToBit(I>>6) ^ ToBit(I>>7) ^ ToBit(I>>8) ^ ToBit(I>>9) ) <<4 |
                ( ToBit(I>>0) ^ ToBit(I>>1) ^ ToBit(I>>2) ^ ToBit(I>>4) ^ ToBit(I>>7) ^ ToBit(I>>8) ^ ToBit(I>>9) ) <<5 |
                ( ToBit(I>>1) ^ ToBit(I>>2) ^ ToBit(I>>3) ^ ToBit(I>>5) ^ ToBit(I>>8) ^ ToBit(I>>9) ) <<6 |
                ( ToBit(I>>2) ^ ToBit(I>>3) ^ ToBit(I>>4) ^ ToBit(I>>6) ^ ToBit(I>>9) ) <<7 |
                ( ToBit(I>>3) ^ ToBit(I>>4) ^ ToBit(I>>5) ^ ToBit(I>>7) ) <<8 |
                ( ToBit(I>>4) ^ ToBit(I>>5) ^ ToBit(I>>6) ^ ToBit(I>>8) ) <<9;
	xa403 = xa403 & TenBit_Mask;
	//printf("xa403=0x%x\n", xa403);
	

	xa407 = ( ToBit(I>>1) ^ ToBit(I>>2) ^ ToBit(I>>3) ^ ToBit(I>>5) ^ ToBit(I>>8) ^ ToBit(I>>9) )<<0 | 
  				( ToBit(I>>2) ^ ToBit(I>>3) ^ ToBit(I>>4) ^ ToBit(I>>6) ^ ToBit(I>>9) ) <<1 | 
  				( ToBit(I>>3) ^ ToBit(I>>4) ^ ToBit(I>>5) ^ ToBit(I>>7) ) <<2 | 
  				( ToBit(I>>1) ^ ToBit(I>>2) ^ ToBit(I>>3) ^ ToBit(I>>4) ^ ToBit(I>>6) ^ ToBit(I>>9) ) <<3 | 
  				( ToBit(I>>2) ^ ToBit(I>>3) ^ ToBit(I>>4) ^ ToBit(I>>5) ^ ToBit(I>>7) ) <<4 | 
  				( ToBit(I>>0) ^ ToBit(I>>3) ^ ToBit(I>>4) ^ ToBit(I>>5) ^ ToBit(I>>6) ^ ToBit(I>>8) ) <<5 | 
  				( ToBit(I>>1) ^ ToBit(I>>4) ^ ToBit(I>>5) ^ ToBit(I>>6) ^ ToBit(I>>7) ^ ToBit(I>>9) ) <<6 | 
  				( ToBit(I>>0) ^ ToBit(I>>2) ^ ToBit(I>>5) ^ ToBit(I>>6) ^ ToBit(I>>7) ^ ToBit(I>>8) ) << 7| 
  				( ToBit(I>>0) ^ ToBit(I>>1) ^ ToBit(I>>3) ^ ToBit(I>>6) ^ ToBit(I>>7) ^ ToBit(I>>8) ^ ToBit(I>>9) ) <<8 | 
  				( ToBit(I>>0) ^ ToBit(I>>1) ^ ToBit(I>>2) ^ ToBit(I>>4) ^ ToBit(I>>7) ^ ToBit(I>>8) ^ ToBit(I>>9) ) <<9;
	xa407 = xa407 & TenBit_Mask;
	//printf("xa407=0x%x\n", xa407);


  	xa607 = ( ToBit(I>>0) ^ ToBit(I>>1) ^ ToBit(I>>3) ^ ToBit(I>>6) ^ ToBit(I>>9) ) <<0 | 
  				( ToBit(I>>0) ^ ToBit(I>>1) ^ ToBit(I>>2) ^ ToBit(I>>4) ^ ToBit(I>>7) ) <<1 | 
  				( ToBit(I>>0) ^ ToBit(I>>1) ^ ToBit(I>>2) ^ ToBit(I>>3) ^ ToBit(I>>5) ^ ToBit(I>>8) ) <<2 | 
  				( ToBit(I>>2) ^ ToBit(I>>4) ) <<3 | 
  				( ToBit(I>>0) ^ ToBit(I>>3) ^ ToBit(I>>5) ) <<4 | 
  				( ToBit(I>>1) ^ ToBit(I>>4) ^ ToBit(I>>6) ) <<5 | 
  				( ToBit(I>>2) ^ ToBit(I>>5) ^ ToBit(I>>7) ) << 6| 
  				( ToBit(I>>0) ^ ToBit(I>>3) ^ ToBit(I>>6) ^ ToBit(I>>8) ) <<7 | 
  				( ToBit(I>>1) ^ ToBit(I>>4) ^ ToBit(I>>7) ^ ToBit(I>>9) ) <<8 | 
  				( ToBit(I>>0) ^ ToBit(I>>2) ^ ToBit(I>>5) ^ ToBit(I>>8) ) <<9;
	xa607 = xa607 & TenBit_Mask;
	//printf("xa607=0x%x\n", xa607);
	
	xa398 =( ToBit(I>>0) ^ ToBit(I>>2) ^ ToBit(I>>4) ) <<0 | 
  				( ToBit(I>>0) ^ ToBit(I>>1) ^ ToBit(I>>3) ^ ToBit(I>>5) ) <<1 | 
  				( ToBit(I>>1) ^ ToBit(I>>2) ^ ToBit(I>>4) ^ ToBit(I>>6) ) <<2 | 
  				( ToBit(I>>3) ^ ToBit(I>>4) ^ ToBit(I>>5) ^ ToBit(I>>7) ) <<3 | 
  				( ToBit(I>>4) ^ ToBit(I>>5) ^ ToBit(I>>6) ^ ToBit(I>>8) ) <<4 | 
  				( ToBit(I>>5) ^ ToBit(I>>6) ^ ToBit(I>>7) ^ ToBit(I>>9) ) <<5 | 
  				( ToBit(I>>0) ^ ToBit(I>>6) ^ ToBit(I>>7) ^ ToBit(I>>8) ) <<6 | 
  				( ToBit(I>>1) ^ ToBit(I>>7) ^ ToBit(I>>8) ^ ToBit(I>>9) ) <<7 | 
  				( ToBit(I>>0) ^ ToBit(I>>2) ^ ToBit(I>>8) ^ ToBit(I>>9) ) <<8 | 
  				( ToBit(I>>1) ^ ToBit(I>>3) ^ ToBit(I>>9) ) <<9;
	xa398 = xa398 & TenBit_Mask;
	//printf("xa398=0x%x\n", xa398);
	
	xa385= ( ToBit(I>>0) ^ ToBit(I>>5) ^ ToBit(I>>6) ^ ToBit(I>>9) ) <<0 | 
				( ToBit(I>>0) ^ ToBit(I>>1) ^ ToBit(I>>6) ^ ToBit(I>>7) )<<1 | 
				( ToBit(I>>1) ^ ToBit(I>>2) ^ ToBit(I>>7) ^ ToBit(I>>8) ) <<2 | 
				( ToBit(I>>2) ^ ToBit(I>>3) ^ ToBit(I>>5) ^ ToBit(I>>6) ^ ToBit(I>>8)) <<3 | 
				( ToBit(I>>0) ^ ToBit(I>>3) ^ ToBit(I>>4) ^ ToBit(I>>6) ^ ToBit(I>>7) ^ ToBit(I>>9) ) <<4 | 
				( ToBit(I>>0) ^ ToBit(I>>1) ^ ToBit(I>>4) ^ ToBit(I>>5) ^ ToBit(I>>7) ^ ToBit(I>>8) ) <<5 | 
				( ToBit(I>>1) ^ ToBit(I>>2) ^ ToBit(I>>5) ^ ToBit(I>>6) ^ ToBit(I>>8) ^ ToBit(I>>9) ) <<6 | 
				( ToBit(I>>2) ^ ToBit(I>>3) ^ ToBit(I>>6) ^ ToBit(I>>7) ^ ToBit(I>>9) ) <<7 | 
				( ToBit(I>>3) ^ ToBit(I>>4) ^ ToBit(I>>7) ^ ToBit(I>>8) ) <<8 | 
				( ToBit(I>>4) ^ ToBit(I>>5) ^ ToBit(I>>8) ^ ToBit(I>>9) ) <<9;
	xa385 = xa385 & TenBit_Mask;
	//printf("xa385=0x%x\n", xa385);
	
	xa540 = ( ToBit(I>>2) ^ ToBit(I>>3) ^ ToBit(I>>4) ^ ToBit(I>>5) ^ ToBit(I>>6) ^ ToBit(I>>7) ^ ToBit(I>>8) )  <<0 | 
				( ToBit(I>>0) ^ ToBit(I>>3) ^ ToBit(I>>4) ^ ToBit(I>>5) ^ ToBit(I>>6) ^ ToBit(I>>7) ^ ToBit(I>>8) ^ ToBit(I>>9) ) <<1 | 
				( ToBit(I>>0) ^ ToBit(I>>1) ^ ToBit(I>>4) ^ ToBit(I>>5) ^ ToBit(I>>6) ^ ToBit(I>>7) ^ ToBit(I>>8) ^ ToBit(I>>9) ) <<2 | 
				( ToBit(I>>0) ^ ToBit(I>>1) ^ ToBit(I>>3) ^ ToBit(I>>4) ^ ToBit(I>>9) ) <<3 | 
				( ToBit(I>>0) ^ ToBit(I>>1) ^ ToBit(I>>2) ^ ToBit(I>>4) ^ ToBit(I>>5) ) <<4 | 
				( ToBit(I>>0) ^ ToBit(I>>1) ^ ToBit(I>>2) ^ ToBit(I>>3) ^ ToBit(I>>5) ^ ToBit(I>>6) ) <<5 | 
				( ToBit(I>>0) ^ ToBit(I>>1) ^ ToBit(I>>2) ^ ToBit(I>>3) ^ ToBit(I>>4) ^ ToBit(I>>6) ^ ToBit(I>>7) ) <<6 | 
				( ToBit(I>>0) ^ ToBit(I>>1) ^ ToBit(I>>2) ^ ToBit(I>>3) ^ ToBit(I>>4) ^ ToBit(I>>5) ^ ToBit(I>>7) ^ ToBit(I>>8) ) <<7 | 
				( ToBit(I>>0) ^ ToBit(I>>1) ^ ToBit(I>>2) ^ ToBit(I>>3) ^ ToBit(I>>4) ^ ToBit(I>>5) ^ ToBit(I>>6) ^ ToBit(I>>8) ^ ToBit(I>>9) )<<8 | 
				( ToBit(I>>1) ^ ToBit(I>>2) ^ ToBit(I>>3) ^ ToBit(I>>4) ^ ToBit(I>>5) ^ ToBit(I>>6) ^ ToBit(I>>7) ^ ToBit(I>>9) ) <<9;
	xa540 = xa540 & TenBit_Mask;
	//printf("xa540=0x%x\n", xa540);

	printf("xa36=0x%x, xa567=0x%x, xa403=0x%x, xa407=0x%x, xa607=0x%x, xa398=0x%x, xa385=0x%x, xa540=0x%x\n", 
		xa36, xa567, xa403, xa407, xa607, xa398, xa385, xa540);
	
	GO00 = xa36 ; 
	GO10 = xa567 ^ GO0 ;
	GO20 = xa403 ^ GO1 ;
	GO30 = xa407 ^ GO2 ;
	GO40 = xa607 ^ GO3 ;
	GO50 = xa398 ^ GO4 ;
	GO60 = xa385 ^ GO5 ;
	GO70 = xa540 ^ GO6 ;

	GO0 =GO00 ; 
	GO1 =GO10 ;
	GO2 =GO20 ;
	GO3 =GO30 ;
	GO4 =GO40 ;
	GO5 =GO50 ;
	GO6 =GO60 ;
	GO7 =GO70 ;
              
	printf("after: input=%x, GO7=%x, GO6=%x, GO5=%x, GO4=%x, GO3=%x, GO2=%x, GO1=%x, GO0=%x\n",
		input, GO7, GO6, GO5, GO4, GO3, GO2, GO1, GO0);
}


/*
 * Main program
 */
int main(int argc, char **argv)
{
	unsigned int ecc0, ecc1, ecc2, ecc3, ecc4, ecc5, ecc6, ecc7, ecc8, ecc9;
	int i;
#if 1
	for ( i=0; i< 518; i++)
		encode_ecc_byte(data_buf[i]);
#else
		encode_ecc_byte(0xA5);
#endif

	ecc0 = (GO7>>2)&0xff;
	ecc1 = ((GO7 & 0x03)<< 6) | ((GO6>>4)&0x3f );
	ecc2 = ((GO6 & 0x0f)<< 4) | ((GO5>>6)&0x0f );
	ecc3 = ((GO5 & 0x3f)<< 2) | ((GO4>>8)&0x03 );
	ecc4 = GO4 & 0xff;
	ecc5 = (GO3>>2)&0xff;
	ecc6 = ((GO3 & 0x03)<< 6) | ((GO2>>4)&0x3f );
	ecc7 = ((GO2 & 0x0f)<< 4) | ((GO1>>6)&0x0f );
	ecc8 = ((GO1 & 0x3f)<< 2) | ((GO0>>8)&0x03 );
	ecc9 = GO0 & 0xff;

	printf("ecc0=%x, ecc1=%x, ecc2=%x, ecc3=%x, ecc4=%x, ecc5=%x, ecc6=%x, ecc7=%x, ecc8=%x, ecc9=%x\n",
		ecc0, ecc1, ecc2, ecc3, ecc4, ecc5, ecc6, ecc7, ecc8, ecc9);
	//Answer: 4c, 06, 07, d5, ab, 19, 91, 31, a1, de	
	return 0;
}
