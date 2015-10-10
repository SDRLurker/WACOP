#include<stdio.h>
#define FILE_ERR(fp) if(!fp){ printf("file open error\n"); return 2; }

typedef long			LONG;
typedef unsigned char		BYTE;
typedef unsigned short		WORD;
typedef unsigned long		DWORD;

typedef struct tagBITMAPFILEHEADER
{
	unsigned short	bfType;
	unsigned long	bfSize;
	unsigned short	bfReserved1;
	unsigned short	bfReserved2;
	unsigned long	bfOffBits;
} __attribute__ ((packed)) BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER
{
	DWORD	biSize;
	LONG	biWidth;
	LONG	biHeight;
	WORD	biPlanes;
	WORD	biBitCount;
	DWORD	biCompression;
	DWORD	biSizeImage;
	LONG	biXPelsPerMeter;
	LONG	biYPelsPerMeter;
	DWORD	biClrUsed;
	DWORD	biClrImportant;
} BITMAPINFOHEADER;

int loadBitmap24(const char *bmps)
// 0 : Error, 1 : Success
// Only load the Bitmap of 240 x 320...
{
	FILE *lcd,*bmp;			// the pointer for TFT LCD
	unsigned short putpixel;	// a pixel for TFT LCD
	BYTE r,g,b;
	int px,py;			// The length of Image...
	int i,j;
	
	BITMAPFILEHEADER bf;
	BITMAPINFOHEADER bi;
	int NMG;
	BYTE *image, temp[4];

	printf("%s\n",bmps);
	bmp = fopen(bmps,"r");
	FILE_ERR(bmp);
	fread(&bf, 14, 1, bmp);
	fread(&bi, sizeof(BITMAPINFOHEADER), 1, bmp);
	printf("%d %d",sizeof(BITMAPFILEHEADER), sizeof(BITMAPINFOHEADER));

	if(bi.biBitCount!=24){
		printf("This function don't apply for %d bits",bi.biBitCount);
		return 0;
	}
	px = bi.biWidth;
	py = bi.biHeight;
	NMG = ((bi.biBitCount>>3)*px%4==0)?0:4-((bi.biBitCount>>3)
		* px%4);
	image = (BYTE*)malloc(px*py*(bi.biBitCount>>3));
	for(i=0;i<bi.biHeight;i++){
		fread(&image[(px*3)*(py-i-1)],px*3,1,bmp);
		fread(&temp,NMG,1,bmp);
	}
	fclose(bmp);
	
	lcd = fopen("/dev/fb0","w");
	FILE_ERR(lcd);
	
	for(i=0;i<py;i++){
		for(j=0;j<px;j++){
			b = image[px*i*3+j*3];
			g = image[px*i*3+j*3+1];
			r = image[px*i*3+j*3+2];
			r /= 8; g/= 4; b/=8;
			putpixel = (r<<11)|(g<<5)|b;
			fwrite(&putpixel,2,1,lcd);
		}
	}

	fclose(lcd);
	free(image);
	return 1;
}
