#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "bs_codec.h"
#include "oki_codec.h"
#include "yma_codec.h"
#include "ymb_codec.h"
#include "ymz_codec.h"

void encode(uint8_t *source,uint8_t *dest,long length,int adpcm_mode)
{
	if(adpcm_mode == 0)
		bs_encode((int16_t*)source,dest,length);
	else if(adpcm_mode == 1)
		ymz_encode((int16_t*)source,dest,length);
	else if(adpcm_mode == 2)
		oki_encode((int16_t*)source,dest,length);
	else if(adpcm_mode == 3)
		oki6258_encode((int16_t*)source,dest,length);
	else if(adpcm_mode == 4)
		yma_encode((int16_t*)source,dest,length);
	else if(adpcm_mode == 5)
		ymb_encode((int16_t*)source,dest,length);
	else if(adpcm_mode == 6)
		aica_encode((int16_t*)source,dest,length);
	else
		exit(-1);
}

void decode(uint8_t *source,uint8_t *dest,long length,int adpcm_mode)
{
	if(adpcm_mode == 0)
		bs_decode(source,(int16_t*)dest,length);
	else if(adpcm_mode == 1)
		ymz_decode(source,(int16_t*)dest,length);
	else if(adpcm_mode == 2)
		oki_decode(source,(int16_t*)dest,length);
	else if(adpcm_mode == 3)
		oki6258_decode(source,(int16_t*)dest,length);
	else if(adpcm_mode == 4)
		yma_decode(source,(int16_t*)dest,length);
	else if(adpcm_mode == 5)
		ymb_decode(source,(int16_t*)dest,length);
	else if(adpcm_mode == 6)
		aica_decode(source,(int16_t*)dest,length);
	else
		exit(-1);
}

int main(int argc, char* argv [])
{

	unsigned long i;

	printf("PCM <-> ADPCM conversion tool\n");
	printf("2018-2022 by ctr\n");
	printf("Input format: signed 16 bit PCM little endian\n");

	if(argc<4)
	{
		printf("Usage: <s|o|x|a|b|c|z><d|e> <file> <destination file> \n");
		printf("List of codecs:\n");
		printf("\ts - Brian Schmidt (BSMT2000 / QSound)\n");
		printf("\to - Oki/Dialogic VOX (MSM6295)\n");
		printf("\tx - Oki X68000 (MSM6258)\n");
		printf("\ta - Yamaha ADPCM-A (YM2610)\n");
		printf("\tb - Yamaha ADPCM-B (Y8950 / YM2608 / YM2610)\n");
		printf("\tc - Yamaha AICA (AICA)\n");
		printf("\tz - Yamaha / Creative (YMZ280B)\n");
		exit(EXIT_FAILURE);
	}

	char* mode = argv[1];
	char* file1 = argv[2];
	char* file2 = argv[3];

	int res;

	FILE* sourcefile;
	/* Load sample file */
	sourcefile = fopen(file1,"rb");
	if(!sourcefile)
	{
		printf("Could not open %s\n",file1);
		exit(EXIT_FAILURE);
	}
	fseek(sourcefile,0,SEEK_END);
	unsigned long sourcefile_size = ftell(sourcefile);
	rewind(sourcefile);
	uint8_t* source = (uint8_t*)malloc(sourcefile_size+1);
	uint8_t* dest = (uint8_t*)malloc(sourcefile_size*4+1);
	res = fread(source,1,sourcefile_size,sourcefile);
	if(res != sourcefile_size)
	{
		printf("Reading error\n");
		exit(EXIT_FAILURE);
	}
	fclose(sourcefile);

	int length;
	int adpcm_mode = -1;

	if(*mode == 's')
	{
		adpcm_mode = 0;
		printf("Using Brian Schmidt algorithm\n");
		mode++;
	}
	else if(*mode == 'z')
	{
		adpcm_mode = 1;
		printf("Using Yamaha YMZ280B algorithm\n");
		mode++;
	}
	else if(*mode == 'o')
	{
		adpcm_mode = 2;
		printf("Using OKI/Dialogic VOX (MSM6295) algorithm\n");
		mode++;
	}
	else if(*mode == 'x')
	{
		adpcm_mode = 3;
		printf("Using OKI X68000 (MSM6258) algorithm\n");
		mode++;
	}
	else if(*mode == 'a')
	{
		adpcm_mode = 4;
		printf("Using Yamaha ADPCM-A algorithm\n");
		mode++;
	}
	else if(*mode == 'b')
	{
		adpcm_mode = 5;
		printf("Using Yamaha ADPCM-B algorithm\n");
		mode++;
	}
	else if(*mode == 'c')
	{
		adpcm_mode = 6;
		printf("Using Yamaha AICA algorithm\n");
		mode++;
	}
	else
	{
		printf("Please specify algorithm\n");
		exit(-1);
	}

	if(*mode == 'e')
	{
		length = sourcefile_size/2;
		if(length & 1)
			length++;	// make the size even
		printf("encoding... (len= %d)\n",length);
		encode(source,dest,length,adpcm_mode);
		length /= 2;
	}

	else if(*mode == 'd')
	{
		length = sourcefile_size*2;
		printf("decoding... (len= %d)\n",length);
		decode(source,dest,length,adpcm_mode);
		length *= 2;
	}

	FILE *destfile;
	destfile = fopen(file2,"wb");
	if(!destfile)
	{
		printf("Could not open %s\n",file2);
		exit(EXIT_FAILURE);
	}
	for(i=0;i<length;i++)
	{
		putc(*((uint8_t*)dest+i),destfile);
	}
	fclose(destfile);
	printf("write ok %lu\n",length);

	free(source);
}
