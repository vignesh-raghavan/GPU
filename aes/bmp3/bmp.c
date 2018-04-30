#include <stdio.h>
#include <stdlib.h>
#include "bmp.h"
#include <omp.h>

typedef unsigned int u32;
unsigned int tbox0[256], tbox1[256], tbox2[256], tbox3[256], tbox4[256];

// foreward sbox
unsigned char sbox[256] =   {
//0     1    2      3     4    5     6     7      8    9     A      B    C     D     E     F
0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76, //0
0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0, //1
0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15, //2
0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75, //3
0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84, //4
0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf, //5
0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8, //6
0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2, //7
0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73, //8
0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb, //9
0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79, //A
0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08, //B
0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a, //C
0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e, //D
0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf, //E
0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16 }; //F

unsigned int rcon[10] = {
	0x01000000,
	0x02000000,
	0x04000000,
	0x08000000,
	0x10000000,
	0x20000000,
	0x40000000,
	0x80000000,
	0x1b000000,
	0x36000000
};

unsigned int KEY[4] = {0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f};

#define GETU32(pt) (((u32) pt[0]) << 24) + (((u32) pt[1]) << 16) + (((u32) pt[2]) << 8) + (((u32) pt[3]))

int main(int argc, char* argv[])
{
	int nThreads;
  	double time1;
	unsigned int i,j,k;
  	if(argc > 1)
  	{
		nThreads = atoi(argv[1]);
  	}
  	else
  	{
		nThreads = 1;
  	}
	printf("START\n");

	/* INPUT BITMAP FILE  */
	FILE* inptr = fopen("tv.bmp", "r");
	if(!inptr)
  	{
		printf("Input Image Not Found!!\n");
		return -1;
  	}

	BITMAPFILEHEADER bf;
  	fread(&bf, sizeof(BITMAPFILEHEADER), 1, inptr);
  	printf("header size = %ld\n", sizeof(BITMAPFILEHEADER));

  	BITMAPINFOHEADER bi;
  	fread(&bi, sizeof(BITMAPINFOHEADER), 1, inptr);
  	printf("headerinfo size = %ld\n", sizeof(BITMAPINFOHEADER));

  	int padding = (4 - (bi.biWidth * sizeof(RGBTRIPLE)) % 4) % 4;
  	int pixel_size = (abs(bi.biHeight)*bi.biWidth)/2;
  	unsigned int* pixel;
  	unsigned int p;

  	printf("width = %d, height = %d\n", bi.biWidth, bi.biHeight);
	
  	pixel = (unsigned int*) malloc(pixel_size * sizeof(unsigned int));
  	k = 0;
  	for(i = 0; i < abs(bi.biHeight); i++)
  	{
		j = 0;
		for(j = 0; j < bi.biWidth; j += 2)
		{
			RGBTRIPLE triple;
			fread(&triple, sizeof(RGBTRIPLE), 1, inptr);

			p = 0;
			p = p | (unsigned int) ((triple.rgbtRed & (0xF8)) << 8); //Red
			p = p | (unsigned int) ((triple.rgbtGreen & (0xFC)) << 3); //Green
			p = p | (unsigned int) ((triple.rgbtBlue & (0xF8)) >> 3); //Blue
		
			p = p << 16;
			fread(&triple, sizeof(RGBTRIPLE), 1, inptr);
			
			p = p | (unsigned int) ((triple.rgbtRed & (0xF8)) << 8); //Red
			p = p | (unsigned int) ((triple.rgbtGreen & (0xFC)) << 3); //Green
			p = p | (unsigned int) ((triple.rgbtBlue & (0xF8)) >> 3); //Blue

			pixel[k] = p;
			//printf("%0x\n", pixel[k]);
			k++;
		}

		fseek(inptr, padding, SEEK_CUR);
  	}

  	fclose(inptr);
 	// AES-128 encrpytion of the Image Pixels
	unsigned int* encrypted = (unsigned int*) malloc(pixel_size * sizeof(unsigned int));
	unsigned int* rk = (unsigned int*)malloc(44*sizeof(unsigned int));
	
	unsigned int temp;

	unsigned int temp1, temp2, temp3;
	unsigned char value;
   signed char temp4;
   for(i = 0; i < 256; i++)
   {
 	   temp1 = (unsigned int) sbox[i];
 	   temp4 = (signed char) sbox[i];
 	   temp4 = ((temp4 >> 7) & 0x1b);
 	   value = ((sbox[i] << 1) ^ temp4);
 	   temp2 = (unsigned int) value;
 	   //temp2 = (unsigned int)galois_mul2(temp1);
 	   temp3 = (unsigned int)(temp2 ^ temp1);
 	   tbox0[i] = (temp2 << 24) + (temp1 << 16) + (temp1 << 8) + (temp3);
 	   tbox1[i] = (temp3 << 24) + (temp2 << 16) + (temp1 << 8) + (temp1);
 	   tbox2[i] = (temp1 << 24) + (temp3 << 16) + (temp2 << 8) + (temp1);
 	   tbox3[i] = (temp1 << 24) + (temp1 << 16) + (temp3 << 8) + (temp2);
 	   tbox4[i] = (temp1 << 24) + (temp1 << 16) + (temp1 << 8) + (temp1);
   }
 
   printf("0x%08x\n", tbox0[0]);
   printf("0x%08x\n", tbox1[0]);
   printf("0x%08x\n", tbox2[0]);
   printf("0x%08x\n", tbox3[0]);
   printf("0x%08x\n", tbox4[0]);
 
   // Key Scheduling is Pre-computed.
   rk[0] = KEY[0];
   rk[1] = KEY[1];
   rk[2] = KEY[2];
   rk[3] = KEY[3];
	printf("%08x %08x %08x %08x\n", rk[0], rk[1], rk[2], rk[3]);
   for(i = 0; i < 10; i++)
   {
 	   temp = rk[(4*i)+3];
 	   rk[(4*i)+4] = rk[(4*i)+0] ^ 
 	        (tbox4[(temp >> 16) & 0xff] & 0xff000000) ^
 	        (tbox4[(temp >>  8) & 0xff] & 0x00ff0000) ^
 	        (tbox4[(temp      ) & 0xff] & 0x0000ff00) ^
 	        (tbox4[(temp >> 24)       ] & 0x000000ff) ^ rcon[i];
 	   rk[(4*i)+5] = rk[(4*i)+1] ^ rk[(4*i)+4];
 	   rk[(4*i)+6] = rk[(4*i)+2] ^ rk[(4*i)+5];
 	   rk[(4*i)+7] = rk[(4*i)+3] ^ rk[(4*i)+6];
	
		printf("%08x %08x %08x %08x\n", rk[(4*i)+4], rk[(4*i)+5], rk[(4*i)+6], rk[(4*i)+7]);
   }

	omp_set_num_threads(nThreads);
	time1 = omp_get_wtime();
	#pragma omp parallel for
	for(i = 0; i <pixel_size; i+=4)
	{
		unsigned int s0, s1, s2, s3, t0, t1, t2, t3; 
		
		// First Addroundkey
		//s0 = *((pixel + i    ));
		//s1 = *((pixel + i + 1));
		//s2 = *((pixel + i + 2));
		//s3 = *((pixel + i + 3));
		//printf("0x%08x %08x %08x %08x\n", s0, s1, s2, s3);
		//printf("0x%08x %08x %08x %08x\n", rk[0], rk[1], rk[2], rk[3]);
		
		s0 = *((pixel + i    )) ^ rk[0];
		s1 = *((pixel + i + 1)) ^ rk[1];
		s2 = *((pixel + i + 2)) ^ rk[2];
		s3 = *((pixel + i + 3)) ^ rk[3];
		//printf("0x%08x %08x %08x %08x\n", s0, s1, s2, s3);
	
		// Round 1
		t0 = tbox0[s0 >> 24] ^ tbox1[(s1 >> 16) & 0xff] ^ tbox2[(s2 >>  8) & 0xff] ^ tbox3[s3 & 0xff] ^ rk[ 4];
		t1 = tbox0[s1 >> 24] ^ tbox1[(s2 >> 16) & 0xff] ^ tbox2[(s3 >>  8) & 0xff] ^ tbox3[s0 & 0xff] ^ rk[ 5];
	  	t2 = tbox0[s2 >> 24] ^ tbox1[(s3 >> 16) & 0xff] ^ tbox2[(s0 >>  8) & 0xff] ^ tbox3[s1 & 0xff] ^ rk[ 6];
  		t3 = tbox0[s3 >> 24] ^ tbox1[(s0 >> 16) & 0xff] ^ tbox2[(s1 >>  8) & 0xff] ^ tbox3[s2 & 0xff] ^ rk[ 7];
		//printf("0x%08x %08x %08x %08x\n", t0, t1, t2, t3);
		
		// Round 2
		s0 = tbox0[t0 >> 24] ^ tbox1[(t1 >> 16) & 0xff] ^ tbox2[(t2 >>  8) & 0xff] ^ tbox3[t3 & 0xff] ^ rk[ 8];
		s1 = tbox0[t1 >> 24] ^ tbox1[(t2 >> 16) & 0xff] ^ tbox2[(t3 >>  8) & 0xff] ^ tbox3[t0 & 0xff] ^ rk[ 9];
	  	s2 = tbox0[t2 >> 24] ^ tbox1[(t3 >> 16) & 0xff] ^ tbox2[(t0 >>  8) & 0xff] ^ tbox3[t1 & 0xff] ^ rk[10];
  		s3 = tbox0[t3 >> 24] ^ tbox1[(t0 >> 16) & 0xff] ^ tbox2[(t1 >>  8) & 0xff] ^ tbox3[t2 & 0xff] ^ rk[11];
		//printf("0x%08x %08x %08x %08x\n", s0, s1, s2, s3);

		// Round 3
		t0 = tbox0[s0 >> 24] ^ tbox1[(s1 >> 16) & 0xff] ^ tbox2[(s2 >>  8) & 0xff] ^ tbox3[s3 & 0xff] ^ rk[12];
		t1 = tbox0[s1 >> 24] ^ tbox1[(s2 >> 16) & 0xff] ^ tbox2[(s3 >>  8) & 0xff] ^ tbox3[s0 & 0xff] ^ rk[13];
	  	t2 = tbox0[s2 >> 24] ^ tbox1[(s3 >> 16) & 0xff] ^ tbox2[(s0 >>  8) & 0xff] ^ tbox3[s1 & 0xff] ^ rk[14];
  		t3 = tbox0[s3 >> 24] ^ tbox1[(s0 >> 16) & 0xff] ^ tbox2[(s1 >>  8) & 0xff] ^ tbox3[s2 & 0xff] ^ rk[15];
		//printf("0x%08x %08x %08x %08x\n", t0, t1, t2, t3);
		
		// Round 4
		s0 = tbox0[t0 >> 24] ^ tbox1[(t1 >> 16) & 0xff] ^ tbox2[(t2 >>  8) & 0xff] ^ tbox3[t3 & 0xff] ^ rk[16];
		s1 = tbox0[t1 >> 24] ^ tbox1[(t2 >> 16) & 0xff] ^ tbox2[(t3 >>  8) & 0xff] ^ tbox3[t0 & 0xff] ^ rk[17];
	  	s2 = tbox0[t2 >> 24] ^ tbox1[(t3 >> 16) & 0xff] ^ tbox2[(t0 >>  8) & 0xff] ^ tbox3[t1 & 0xff] ^ rk[18];
  		s3 = tbox0[t3 >> 24] ^ tbox1[(t0 >> 16) & 0xff] ^ tbox2[(t1 >>  8) & 0xff] ^ tbox3[t2 & 0xff] ^ rk[19];
		//printf("0x%08x %08x %08x %08x\n", s0, s1, s2, s3);

		// Round 5
		t0 = tbox0[s0 >> 24] ^ tbox1[(s1 >> 16) & 0xff] ^ tbox2[(s2 >>  8) & 0xff] ^ tbox3[s3 & 0xff] ^ rk[20];
		t1 = tbox0[s1 >> 24] ^ tbox1[(s2 >> 16) & 0xff] ^ tbox2[(s3 >>  8) & 0xff] ^ tbox3[s0 & 0xff] ^ rk[21];
	  	t2 = tbox0[s2 >> 24] ^ tbox1[(s3 >> 16) & 0xff] ^ tbox2[(s0 >>  8) & 0xff] ^ tbox3[s1 & 0xff] ^ rk[22];
  		t3 = tbox0[s3 >> 24] ^ tbox1[(s0 >> 16) & 0xff] ^ tbox2[(s1 >>  8) & 0xff] ^ tbox3[s2 & 0xff] ^ rk[23];
		//printf("0x%08x %08x %08x %08x\n", t0, t1, t2, t3);
		
		// Round 6
		s0 = tbox0[t0 >> 24] ^ tbox1[(t1 >> 16) & 0xff] ^ tbox2[(t2 >>  8) & 0xff] ^ tbox3[t3 & 0xff] ^ rk[24];
		s1 = tbox0[t1 >> 24] ^ tbox1[(t2 >> 16) & 0xff] ^ tbox2[(t3 >>  8) & 0xff] ^ tbox3[t0 & 0xff] ^ rk[25];
	  	s2 = tbox0[t2 >> 24] ^ tbox1[(t3 >> 16) & 0xff] ^ tbox2[(t0 >>  8) & 0xff] ^ tbox3[t1 & 0xff] ^ rk[26];
  		s3 = tbox0[t3 >> 24] ^ tbox1[(t0 >> 16) & 0xff] ^ tbox2[(t1 >>  8) & 0xff] ^ tbox3[t2 & 0xff] ^ rk[27];
		//printf("0x%08x %08x %08x %08x\n", s0, s1, s2, s3);

		// Round 7
		t0 = tbox0[s0 >> 24] ^ tbox1[(s1 >> 16) & 0xff] ^ tbox2[(s2 >>  8) & 0xff] ^ tbox3[s3 & 0xff] ^ rk[28];
		t1 = tbox0[s1 >> 24] ^ tbox1[(s2 >> 16) & 0xff] ^ tbox2[(s3 >>  8) & 0xff] ^ tbox3[s0 & 0xff] ^ rk[29];
	  	t2 = tbox0[s2 >> 24] ^ tbox1[(s3 >> 16) & 0xff] ^ tbox2[(s0 >>  8) & 0xff] ^ tbox3[s1 & 0xff] ^ rk[30];
  		t3 = tbox0[s3 >> 24] ^ tbox1[(s0 >> 16) & 0xff] ^ tbox2[(s1 >>  8) & 0xff] ^ tbox3[s2 & 0xff] ^ rk[31];
		//printf("0x%08x %08x %08x %08x\n", t0, t1, t2, t3);
		
		// Round 8
		s0 = tbox0[t0 >> 24] ^ tbox1[(t1 >> 16) & 0xff] ^ tbox2[(t2 >>  8) & 0xff] ^ tbox3[t3 & 0xff] ^ rk[32];
		s1 = tbox0[t1 >> 24] ^ tbox1[(t2 >> 16) & 0xff] ^ tbox2[(t3 >>  8) & 0xff] ^ tbox3[t0 & 0xff] ^ rk[33];
	  	s2 = tbox0[t2 >> 24] ^ tbox1[(t3 >> 16) & 0xff] ^ tbox2[(t0 >>  8) & 0xff] ^ tbox3[t1 & 0xff] ^ rk[34];
  		s3 = tbox0[t3 >> 24] ^ tbox1[(t0 >> 16) & 0xff] ^ tbox2[(t1 >>  8) & 0xff] ^ tbox3[t2 & 0xff] ^ rk[35];
		//printf("0x%08x %08x %08x %08x\n", s0, s1, s2, s3);

		// Round 9
		t0 = tbox0[s0 >> 24] ^ tbox1[(s1 >> 16) & 0xff] ^ tbox2[(s2 >>  8) & 0xff] ^ tbox3[s3 & 0xff] ^ rk[36];
		t1 = tbox0[s1 >> 24] ^ tbox1[(s2 >> 16) & 0xff] ^ tbox2[(s3 >>  8) & 0xff] ^ tbox3[s0 & 0xff] ^ rk[37];
	  	t2 = tbox0[s2 >> 24] ^ tbox1[(s3 >> 16) & 0xff] ^ tbox2[(s0 >>  8) & 0xff] ^ tbox3[s1 & 0xff] ^ rk[38];
  		t3 = tbox0[s3 >> 24] ^ tbox1[(s0 >> 16) & 0xff] ^ tbox2[(s1 >>  8) & 0xff] ^ tbox3[s2 & 0xff] ^ rk[39];
		//printf("0x%08x %08x %08x %08x\n", t0, t1, t2, t3);

		// Round 10
		s0 = (tbox4[t0 >> 24] & 0xff000000) ^ (tbox4[(t1 >> 16) & 0xff] & 0xff0000) ^ (tbox4[(t2 >>  8) & 0xff] & 0xff00) ^ (tbox4[t3 & 0xff] & 0xff) ^ rk[40];
		s1 = (tbox4[t1 >> 24] & 0xff000000) ^ (tbox4[(t2 >> 16) & 0xff] & 0xff0000) ^ (tbox4[(t3 >>  8) & 0xff] & 0xff00) ^ (tbox4[t0 & 0xff] & 0xff) ^ rk[41];
	  	s2 = (tbox4[t2 >> 24] & 0xff000000) ^ (tbox4[(t3 >> 16) & 0xff] & 0xff0000) ^ (tbox4[(t0 >>  8) & 0xff] & 0xff00) ^ (tbox4[t1 & 0xff] & 0xff) ^ rk[42];
  		s3 = (tbox4[t3 >> 24] & 0xff000000) ^ (tbox4[(t0 >> 16) & 0xff] & 0xff0000) ^ (tbox4[(t1 >>  8) & 0xff] & 0xff00) ^ (tbox4[t2 & 0xff] & 0xff) ^ rk[43];
		//printf("0x%08x %08x %08x %08x\n", s0, s1, s2, s3);
		
		*((encrypted + i    ))  = s0;	
		*((encrypted + i + 1))  = s1;
		*((encrypted + i + 2))  = s2;
		*((encrypted + i + 3))  = s3;
	}
	time1 = omp_get_wtime() - time1;

	/* OUTPUT BITMAP FILE  */
  	FILE* outptr = fopen("my.bmp", "w");
 	if(!outptr)
  	{
 		printf("Stale Output Image Handle\n");
		return -1;
  	}

  	fwrite(&bf, sizeof(BITMAPFILEHEADER), 1, outptr);
  	fwrite(&bi, sizeof(BITMAPINFOHEADER), 1, outptr);

  	k = 0;
  	for(i = 0; i < abs(bi.biHeight); i++)
  	{
		j = 0;
		for(j = 0; j < bi.biWidth; j += 2)
		{
			RGBTRIPLE triple;
			//p = pixel[k];
			p = encrypted[k];
			triple.rgbtRed   = ((p >> 8) & 0xF8);
			triple.rgbtGreen = ((p >> 3) & 0xFC);
			triple.rgbtBlue  = ((p << 3) & 0xF8);
			fwrite(&triple, sizeof(RGBTRIPLE), 1, outptr);
			
			p = p >> 16;
			triple.rgbtRed   = ((p >> 8) & 0xF8);
			triple.rgbtGreen = ((p >> 3) & 0xFC);
			triple.rgbtBlue  = ((p << 3) & 0xF8);
			fwrite(&triple, sizeof(RGBTRIPLE), 1, outptr);

			k++;
		}
		fseek(outptr, padding, SEEK_CUR);
  	}

 	printf("Number of Threads %d\n", nThreads); 
  	printf("Elapsed time for encrpytion %fs\n", time1);

  	free(pixel);
  	free(encrypted);
  	fclose(outptr);
	//////////////////////////
	
	printf("END\n");
	return 0;
}
