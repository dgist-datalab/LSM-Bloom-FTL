#include <simdcomp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

size_t compress(uint32_t * datain, size_t length, uint8_t * buffer) {
    uint32_t offset;
    uint8_t * initout;
    size_t k;
    if(length/SIMDBlockSize*SIMDBlockSize != length) {
        printf("Data length should be a multiple of %i \n",SIMDBlockSize);
    }
    offset = 0;
    initout = buffer;
    for(k = 0; k < length / SIMDBlockSize; ++k) {
        uint32_t b = simdmaxbitsd1(offset,
                                   datain + k * SIMDBlockSize);
        *buffer++ = b;
        simdpackwithoutmaskd1(offset, datain + k * SIMDBlockSize, (__m128i *) buffer,
                              b);
        offset = datain[k * SIMDBlockSize + SIMDBlockSize - 1];
        buffer += b * sizeof(__m128i);
    }
    return buffer - initout;
}

uint32_t * decompress(uint8_t *buffer, uint32_t num, uint32_t *backbuffer){
	uint32_t offset=0;
	for(int k=0; k * SIMDBlockSize<num; ++k){
		uint8_t b= *buffer++;
		simdunpackd1(offset, (__m128i *) buffer, backbuffer, b);
		buffer +=b *sizeof(__m128i);
		offset=backbuffer[SIMDBlockSize-1];
	}
	return (uint32_t*)buffer;
}


uint32_t n_compress(uint32_t * datain, uint32_t number, uint8_t *buffer, uint32_t b){
	__m128i *endofbuf=simdpack_length(datain, number, (__m128i*)buffer, b);
	return (endofbuf-(__m128i*)buffer)*sizeof(__m128i);
}

uint32_t* n_decompress(uint8_t *buffer, uint32_t number, uint32_t *backbuffer, uint32_t b){
	simdunpack_length((const __m128i*)buffer, number, backbuffer, b);
	return (uint32_t*)buffer;
}



int main(int argc, char *argv[]){

	if(argc < 2){
		printf("USAGE: %s [FILE]\n", argv[0]);
		abort();
	}

	FILE *f=fopen(argv[1], "r");
	if(!f){
		printf("ttt!\n");
		abort();
	}

	uint32_t piece=atoi(argv[2]);

	uint32_t target_num=1024*piece;
	uint8_t *buffer=malloc(target_num * sizeof(uint32_t) + target_num/SIMDBlockSize);

	ssize_t read;
	size_t len=0;
	char *line;

	uint32_t a,b;
	uint32_t compsize=0;
	uint32_t *datain=(uint32_t*)malloc(sizeof(uint32_t) * target_num);
	uint32_t data_idx=0;
	uint32_t cnt=0;

	uint32_t *backbuffer=malloc(SIMDBlockSize * sizeof(uint32_t));
	uint64_t time=0;
	uint32_t compressed_cnt=0;
	uint32_t max_bits;
	while((read=getline(&line, &len, f))!=-1){
		sscanf(line, "%d%d", &a, &b);
		if(b==0) continue;
		datain[data_idx++]=b;
		if(data_idx==target_num){
			/*
			compsize+=compress(datain, data_idx, buffer);
			memset(datain, 0, sizeof(uint32_t) *target_num);
			uint32_t *test=decompress(buffer, target_num, backbuffer);*/
			for(uint32_t k=0; k<10; k++) printf("%u ", datain[k]);
			max_bits=maxbits_length(datain, data_idx);
			compsize+=n_compress(datain, data_idx, buffer, max_bits);
			uint32_t *test=n_decompress(buffer, data_idx, backbuffer, max_bits);
			printf("\n");
			for(uint32_t k=0; k<10; k++) printf("%u ", buffer[k]);
			printf("\n");
			compressed_cnt++;
			data_idx=0;
		}


		cnt++;
	}

	float ratio=(float)compsize/(cnt*4);
	float bits=ratio * 32;
	printf("%.2lf\n",bits);
	return 1;
}
