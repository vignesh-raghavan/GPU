#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include "bmp.h"
#include "aes.h"

int main(int argc, char* argv[])
{
	int nThreads;
	double time1;

	if(argc > 1)
	{
		nThreads = atoi(argv[1]);
	}
	else
	{
		nThreads = 1;
	}

	printf("Hello from bmp.c\n");

	/* INPUT BITMAP FILE  */
	FILE* inptr = fopen("tv.bmp", "r");
	if(!inptr)
	{
		printf("Input Image Not Found!!\n");
		return -1;
	}

	BITMAPFILEHEADER bf;
	fread(&bf, sizeof(BITMAPFILEHEADER), 1, inptr);
	printf("header size = %d\n", sizeof(BITMAPFILEHEADER));

	BITMAPINFOHEADER bi;
	fread(&bi, sizeof(BITMAPINFOHEADER), 1, inptr);
	printf("headerinfo size = %d\n", sizeof(BITMAPINFOHEADER));

	int padding = (4 - (bi.biWidth * sizeof(RGBTRIPLE)) % 4) % 4;
	int pixel_size = (abs(bi.biHeight)*bi.biWidth)/2;
	unsigned int* pixel;
	unsigned int p;
	int i, j, k;

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
			
			//p = (unsigned int) ((triple.rgbtRed) << 16); //Red
			//p = p | (unsigned int) ((triple.rgbtGreen) << 8); //Green
			//p = p | (unsigned int) ((triple.rgbtBlue)); //Blue
			
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
	generate_keys(key1);
	
   omp_set_num_threads(nThreads);
	time1 = omp_get_wtime();
   #pragma omp parallel for
	for(i = 0; i < pixel_size; i += 4)
	{
		sw_encrypt((pixel+i), (encrypted+i));
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
	return 0;
}
