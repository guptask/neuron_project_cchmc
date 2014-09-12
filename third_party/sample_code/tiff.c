#include <stdio.h>
#include <tiffio.h>
#include <stdlib.h>
#include <string.h>

int verbose = 0;

void tiff_write_raw (int width, int length, char* name, char* buffer, int buflen);
void tiff_write_raw (int width, int length, char* name, char* buffer, int buflen) {

  TIFF *image;
  // Open the TIFF file
  if((image = TIFFOpen(name, "w")) == NULL){
    printf("Could not open %s for writing\n", name);
    exit(1);
  }

  // We need to set some values for basic tags before we can add any data
  TIFFSetField(image, TIFFTAG_IMAGEWIDTH, width);
  TIFFSetField(image, TIFFTAG_IMAGELENGTH, length);
  TIFFSetField(image, TIFFTAG_BITSPERSAMPLE, 1);
  TIFFSetField(image, TIFFTAG_SAMPLESPERPIXEL, 1);
  TIFFSetField(image, TIFFTAG_ROWSPERSTRIP, length);

  TIFFSetField(image, TIFFTAG_COMPRESSION, COMPRESSION_CCITTFAX4);
  TIFFSetField(image, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISWHITE);
  TIFFSetField(image, TIFFTAG_FILLORDER, FILLORDER_MSB2LSB);
  TIFFSetField(image, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

  TIFFSetField(image, TIFFTAG_XRESOLUTION, 300.0);
  TIFFSetField(image, TIFFTAG_YRESOLUTION, 300.0);
  TIFFSetField(image, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
  
  // Write the information to the file

  printf ("Writing %u chars", buflen);
  TIFFWriteRawStrip(image, 0, buffer, buflen);

  // Close the file
  TIFFClose(image);
}

void tiff_write (int width, int length, char* name, char* buffer);
void tiff_write (int width, int length, char* name, char* buffer) {

  TIFF *image;
  // Open the TIFF file
  if((image = TIFFOpen(name, "w")) == NULL){
    printf("Could not open %s for writing\n", name);
    exit(1);
  }

  // We need to set some values for basic tags before we can add any data
  TIFFSetField(image, TIFFTAG_IMAGEWIDTH, width);
  TIFFSetField(image, TIFFTAG_IMAGELENGTH, length);
  TIFFSetField(image, TIFFTAG_BITSPERSAMPLE, 1);
  TIFFSetField(image, TIFFTAG_SAMPLESPERPIXEL, 1);
  TIFFSetField(image, TIFFTAG_ROWSPERSTRIP, length);

  TIFFSetField(image, TIFFTAG_COMPRESSION, COMPRESSION_CCITTFAX4);
  TIFFSetField(image, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISWHITE);
  TIFFSetField(image, TIFFTAG_FILLORDER, FILLORDER_MSB2LSB);
  TIFFSetField(image, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

  TIFFSetField(image, TIFFTAG_XRESOLUTION, 300.0);
  TIFFSetField(image, TIFFTAG_YRESOLUTION, 300.0);
  TIFFSetField(image, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
  
  // Write the information to the file

  //printf ("Writing %u chars", width*length);
  TIFFWriteEncodedStrip(image, 0, buffer, width*length);

  // Close the file
  TIFFClose(image);
}


void tiff_read (int *width, int *height, char* name, char* buffer); 
void tiff_read (int *width, int *height, char* name, char* buffer) {

  TIFF *image;
  uint16 photo, bps, spp, fillorder;
  tsize_t stripSize;
  unsigned long imageOffset, result;
  int stripMax, stripCount;
  char tempbyte;
  unsigned long bufferSize, count;
  int lineval[8192];
  int changes[20];
  int changecount;
  int above;

  // Open the TIFF image
  if((image = TIFFOpen(name, "r")) == NULL){
    fprintf(stderr, "Could not open incoming image\n");
    exit(1);
  }

  // Check that it is of a type that we support
  if((TIFFGetField(image, TIFFTAG_BITSPERSAMPLE, &bps) == 0) || (bps != 1)){
    fprintf(stderr, "Either undefined or unsupported number of bits per sample\n");
    exit(1);
  }

  if((TIFFGetField(image, TIFFTAG_SAMPLESPERPIXEL, &spp) == 0) || (spp != 1)){
    fprintf(stderr, "Either undefined or unsupported number of samples per pixel\n");
    exit(1);
  }

  // Read in the possibly multiple strips
  stripSize = TIFFStripSize (image);
  stripMax = TIFFNumberOfStrips (image);
  imageOffset = 0;
  
  bufferSize = TIFFNumberOfStrips (image) * stripSize;
  
  for (stripCount = 0; stripCount < stripMax; stripCount++){
    if((result = TIFFReadEncodedStrip (image, stripCount,
				      buffer + imageOffset,
				      stripSize)) == -1){
      fprintf(stderr, "Read error on input strip number %d\n", stripCount);
      exit(1);
    }

    imageOffset += result;
  }

  // Deal with photometric interpretations
  if(TIFFGetField(image, TIFFTAG_PHOTOMETRIC, &photo) == 0){
    fprintf(stderr, "Image has an undefined photometric interpretation\n");
    exit(1);
  }
  
  if(photo != PHOTOMETRIC_MINISWHITE){
    // Flip bits
    printf("Fixing the photometric interpretation\n");

    for(count = 0; count < bufferSize; count++)
      buffer[count] = ~buffer[count];
  }

  // Deal with fillorder
  if(TIFFGetField(image, TIFFTAG_FILLORDER, &fillorder) == 0){
    fprintf(stderr, "Image has an undefined fillorder\n");
    exit(1);
  }
  
  if(fillorder != FILLORDER_MSB2LSB){
    // We need to swap bits -- ABCDEFGH becomes HGFEDCBA
    printf("Fixing the fillorder\n");

    for(count = 0; count < bufferSize; count++){
      tempbyte = 0;
      if(buffer[count] & 128) tempbyte += 1;
      if(buffer[count] & 64) tempbyte += 2;
      if(buffer[count] & 32) tempbyte += 4;
      if(buffer[count] & 16) tempbyte += 8;
      if(buffer[count] & 8) tempbyte += 16;
      if(buffer[count] & 4) tempbyte += 32;
      if(buffer[count] & 2) tempbyte += 64;
      if(buffer[count] & 1) tempbyte += 128;
      buffer[count] = tempbyte;
    }
  }
     
   if(TIFFGetField(image, TIFFTAG_IMAGEWIDTH, width) == 0){
    fprintf(stderr, "Image does not define its width\n");
    exit(1);
  }
   if(TIFFGetField(image, TIFFTAG_IMAGELENGTH, height) == 0){
    fprintf(stderr, "Image does not define its heigth\n");
    exit(1);
  }


  TIFFClose(image);
}

int tiff_testswap (int width, int length, char *buffer);
int tiff_testswap (int width, int length, char *buffer) {

/*simple idea, test for the i's whenever going from black 
 * into white. If there was an i, we will have a long black stretch
 * preceded by a short white stretch, precedd by a short white dot
 */

	long w,l;
	long changes=0;
	long changesum=0;
	short black=0;
	long patterns = 0;
	long pattern[3];
	pattern[0]=pattern[1]=pattern[2]=0;
	
    void checkpattern () {
			//printf("P %d %d %d\n", pattern[(changes)%3], pattern[(changes-1)%3], pattern[(changes-2)%3]); 
		if (pattern[(changes)%3] > pattern[(changes-2)%3]*2
			&& pattern[(changes)%3] > pattern[(changes-1)%3]*2
			&& pattern[(changes)%3] < 100) patterns++;
	};
	
	for (w=0; w<width/8; w++) {
		for (l=0; l<length; l++) {
			//printf ("w%d l%d %d\n", w, l, buffer[l*width/8+w]);
			pattern[changes%3]++;
			if ((unsigned char)buffer[l*width/8+w]!=0 && black==0)
				{black=1;changes++;pattern[changes%3]=0;}
			if ((unsigned char)buffer[l*width/8+w]==0 && black==1)
				{black=0;checkpattern();changes++;pattern[changes%3]=0;}
		}
	}
	int perm = patterns*1000/changes; //i-atic pattern per thousand
	printf ("perm %d patterns %d changes %d\n", perm, patterns, changes);

	//ugly heuristics ...
	if (changes<1000) return -perm; //hard to say
	if (perm>18 && changes>1000) return perm;
	if (perm<=18 && changes>1000) return perm; 

	//if > 18 then it's good.

}

void tiff_slice(int width, int length,char *buffer,int from, int to, char *rbuffer);
void tiff_slice(int width, int length,char *buffer,int from, int to, char *rbuffer) {

	int w,l;	
	int addvalue = 0;
	int w8 = width/8;
	int w8rest = width%8;
	if (width/8 != width/8.0) {addvalue = from; printf ("Warning width %d not divisible by 8!\n", width);}
	for (l=0; l<(to-from+1); l++) {
		for (w=0;w<w8;w++) {
				//rbuffer[l*w8+w] = buffer[(l+from)*w8+w]; //worked for old fujitsu scans
				//rbuffer[l*w8+w] = buffer[(l+from)*w8+(w8rest*(l+from))/8+w];
				//rbuffer[l*w8+w] = buffer[(l+from)*w8+(w8rest*(from))/8+w];
				rbuffer[l*w8+w] = buffer[(l+from)*w8+addvalue+w]; //works for new c6265a scans
		}
	}
	return; 
}

void inspect_byte (char buffer);
void inspect_byte (char buffer) {
	int p,pow;
	pow = 1;
	printf("\nbyte(%d): ",(int)(unsigned char)buffer);
	for (p=0;p<8;p++) {
		printf("%d",(int)(unsigned char)(((unsigned char)buffer%(pow*2))/pow));
		pow = pow * 2;
	}
	printf("\n");
}

char tiff_shift_left(char lbuffer, char rbuffer, int shift);
char tiff_shift_left(char lbuffer, char rbuffer, int shift) {
	
	int pow = 1; int i;
	for (i=0; i<shift; i++) {
		pow = pow * 2;
	}
	int antipow = 256/pow;
	//left cell restructuring
	if (verbose) printf ("In shift: %d <- lbuffer (%d->%d) + rbuffer (%d->%d) pow %d.\n", ((unsigned char)lbuffer)/pow+(((unsigned char)rbuffer)%pow)*antipow, lbuffer, (unsigned char)lbuffer/pow, rbuffer, ((unsigned char)rbuffer%pow)*antipow, pow);
	lbuffer=(((unsigned char)lbuffer)%antipow)*pow+(((unsigned char)rbuffer)/antipow);
	return lbuffer;
}

void tiff_rotate_cell(char *buffer,char *rbuffer);
void tiff_rotate_cell(char *buffer,char *rbuffer) {

	int w,l,rpow,wpow,p;	
		//rpow is read bit position, wpow is write bit position
	wpow = 1;
	for (l=0; l<8; l++) {
		if (verbose) inspect_byte(buffer[l]);
		rpow = 128;
		for (p=0; p<8; p++) {
			//p=y*b+x, y=(p-x)/b, x=p-yb, q=xb+y
			//b=w8, y=l, x=w
			if (verbose) printf("%d <- %d (%d)\n", p, l,  (((((unsigned char)buffer[l]%(rpow*2))/rpow))*wpow));
			rbuffer[p] = rbuffer[p] | (((((unsigned char)buffer[l]%(rpow*2))/rpow))*wpow);
			rpow = rpow / 2;	
			}
	wpow = wpow * 2;
	}
	return; 
}

void tiff_rotate_cell_ccw(char *buffer,char *rbuffer);
void tiff_rotate_cell_ccw(char *buffer,char *rbuffer) {

	int w,l,rpow,wpow,p;	
		//rpow is read bit position, wpow is write bit position
	wpow = 1;
	for (l=0; l<8; l++) {
		if (verbose) inspect_byte(buffer[l]);
		rpow = 128;
		for (p=0; p<8; p++) {
			//p=y*b+x, y=(p-x)/b, x=p-yb, q=xb+y
			//b=w8, y=l, x=w
			if (verbose) printf("%d <- %d (%d)\n", p, l,  (((((unsigned char)buffer[l]%(rpow*2))/rpow))*wpow));
			rbuffer[7-p] = rbuffer[7-p] | (((((unsigned char)buffer[7-l]%(rpow*2))/rpow))*wpow);
			rpow = rpow / 2;	
			}
	wpow = wpow * 2;
	}
	return; 
}

void tiff_rotate_ccw(int width, int length,char *buffer,char *rbuffer);
void tiff_rotate_ccw(int width, int length,char *buffer,char *rbuffer) {

	int w,l,rpow,wpow,p;	
		//rpow is read bit position, wpow is write bit position
	int w8 = (width-1)/8+1;
	int l8 = (length-1)/8+1;
	int w8shift = (8-width%8)%8;
	char cellbuffer_in[8];
	char cellbuffer_out[8];
	int i;
	if (verbose) printf("Width %d Length %d Shift %d \n", width,length, w8shift);
	wpow = 1;
	for (l=0; l<l8; l++) {
		//progress in 8 line segments
		for (w=w8-1;w>=0;w--) {
			//read in 8 vertical bytes
			for (i=0;i<8;i++) {
				cellbuffer_in[i] = buffer[(l8-l-1)*8*w8+i*(w8)+w];
			}
			if (verbose) printf("Cell l %d w %d\n", l, w);
			memset(cellbuffer_out,0,8);
			tiff_rotate_cell_ccw(cellbuffer_in, cellbuffer_out);
			//write 8 horizontal bytes
			for (i=0;i<8;i++) {
				if (verbose) printf("Writing into %d (w %d i %d l %d).\n", (w8-w-1)*8*l8-w8shift*l8+l8*i+l8-l-1, w,i,l);
				int targetbyte = (w8-w-1)*8*l8-w8shift*l8+l8*i+l8-l-1;
				if (targetbyte >= 0) //make sure w8shift lines are discarded
					rbuffer[targetbyte]=cellbuffer_out[i];
				//left adjustment, can be skipped (slightly inflated tiffs)
			}
		}	
	} // 8 liner done 
	return; 
}

void tiff_rotate(int width, int length,char *buffer,char *rbuffer);
void tiff_rotate(int width, int length,char *buffer,char *rbuffer) {

	int w,l,rpow,wpow,p;	
		//rpow is read bit position, wpow is write bit position
	int w8 = (width-1)/8+1;
	int l8 = (length-1)/8+1;
	int l8shift = (8-length%8)%8;
	char cellbuffer_in[8];
	char cellbuffer_out[8];
	int i;
	if (verbose) printf("Width %d Length %d \n", width,length);
	//if (width/8 != width/8.0) {printf ("Warning width %d not divisible by 8!\n", width);}
	wpow = 1;
	for (l=l8-1; l>=0; l--) {
		//progress in 8 line segments
		for (w=0;w<w8;w++) {
			//read in 8 vertical bytes
			for (i=0;i<8;i++) {
				cellbuffer_in[i] = buffer[l*8*w8+i*w8+w];
			}
			if (verbose) printf("Cell l %d w %d\n", l, w);
			memset(cellbuffer_out,0,8);
			tiff_rotate_cell(cellbuffer_in, cellbuffer_out);
			//write 8 horizontal bytes
			for (i=0;i<8;i++) {
				if (verbose) printf("Writing into %d (w %d i %d l %d).\n", w*8*l8+l8*i+l8-l-1, w,i,l);
				rbuffer[w*8*l8+l8*i+l8-l-1]=cellbuffer_out[i];
				//left adjustment, can be skipped (slightly inflated tiffs)
				if (l8shift != 0) {
					if (l<l8-1) {
						if (verbose) printf("Shifting into %d from %d by %d.\n", w*8*l8+l8*i+l8-l-2, w*8*l8+l8*i+l8-l-1, l8shift);
						rbuffer[w*8*l8+l8*i+l8-l-2]=tiff_shift_left(rbuffer[w*8*l8+l8*i+l8-l-2], rbuffer[w*8*l8+l8*i+l8-l-1], l8shift); } 
					else {
						if (verbose) printf("Postponing edgeshift %d from %d by %d.\n", w*8*l8+l8*i+l8-l-2, w*8*l8+l8*i+l8-l-1, l8shift);
					}
				}
			}
		}	
	} // 8 liner done 
	if (l8shift != 0) { //shift rest last column (treated first,w=l8-1,l=l8-1)
		for (i=l8-1;i<w8*l8*8;i+=l8) {
			//printf("Postponed Edgeshift %d %d %d.\n", l8*i+l8-l-2, l8*i+l8-l-1, l8shift);
			if (verbose) printf("Postponed Edgeshift %d %d.\n", i, l8shift);
			rbuffer[i]=tiff_shift_left(rbuffer[i], (char)0, l8shift); 
		} 
	} 
	return; 
}

void tiff_vertswap (int width, int length, char *buffer, char*swapline);
void tiff_vertswap (int width, int length, char *buffer, char*swapline) {

	int w,l;
	int addvalue = 0;
	if (width/8 != width/8.0) {addvalue = 1; printf ("Warning width %d not divisible by 8!\n", width);}	
	for (w=0; w<width/8; w++) {
		for (l=0; l<length; l++)
			swapline[l]=buffer[l*(width/8+addvalue)+w];
		for (l=0; l<length; l++)
			buffer[(length-1-l)*(width/8+addvalue)+w]=swapline[l];	
	}
	return; 
}

void tiff_hoswap (int width, int length, char *buffer, char*swapline);
void tiff_hoswap (int width, int length, char *buffer, char*swapline) {

char lookup_table[256]={0,128,64,192,32,160,96,224,16,144,80,208,48,176,112,240,8,136,72,200,40,168,104,232,24,152,88,216,56,184,120,248,4,132,68,196,36,164,100,228,20,148,84,212,52,180,116,244,12,140,76,204,44,172,108,236,28,156,92,220,60,188,124,252,2,130,66,194,34,162,98,226,18,146,82,210,50,178,114,242,10,138,74,202,42,170,106,234,26,154,90,218,58,186,122,250,6,134,70,198,38,166,102,230,22,150,86,214,54,182,118,246,14,142,78,206,46,174,110,238,30,158,94,222,62,190,126,254,1,129,65,193,33,161,97,225,17,145,81,209,49,177,113,241,9,137,73,201,41,169,105,233,25,153,89,217,57,185,121,249,5,133,69,197,37,165,101,229,21,149,85,213,53,181,117,245,13,141,77,205,45,173,109,237,29,157,93,221,61,189,125,253,3,131,67,195,35,163,99,227,19,147,83,211,51,179,115,243,11,139,75,203,43,171,107,235,27,155,91,219,59,187,123,251,7,135,71,199,39,167,103,231,23,151,87,215,55,183,119,247,15,143,79,207,47,175,111,239,31,159,95,223,63,191,127,255};


	char swapbyte (char c) {
		int d,e;
		d = (int)c;
		d = d+256;
		e = (((d%256)/128)*1+((d%128)/64)*2+((d%64)/32)*4+((d%32)/16)*8+((d%16)/8)*16+((d%8)/4)*32+((d%4)/2)*64+((d%2)/1)*128);
		//printf("%d %d \n", d,e);
		return (char)e;
	}

	char ReverseBits(char b)
	{
		  char c;
		    c  = ((b >>  1) & 0x55) | ((b <<  1) & 0xaa);
			  c |= ((b >>  2) & 0x33) | ((b <<  2) & 0xcc);
			    c |= ((b >>  4) & 0x0f) | ((b <<  4) & 0xf0);
				    return(c);
	}
	
	int w,l;	
	int w8 = width/8;
	for (l=0; l<length; l++) {
		for (w=0; w<w8; w++)
			swapline[w]=buffer[l*w8+w];
		for (w=0; w<w8; w++)
			buffer[l*w8+(w8-1-w)]=swapbyte(swapline[w]);	
	}
	return; 
}
