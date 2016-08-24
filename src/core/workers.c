#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "workers.h"
#include "constant.h"
#include "mrczip.h"

static int bitsMaskTable[] =
{     0xFFFFFFFF, 0xFFFFFFFE, 0xFFFFFFFC, 0xFFFFFFF8, 0xFFFFFFF0, 0xFFFFFFE0,
		0xFFFFFFC0, 0xFFFFFF80, 0xFFFFFF00, 0xFFFFFE00, 0xFFFFFC00, 0xFFFFF800,
		0xFFFFF000, 0xFFFFE000, 0xFFFFC000, 0xFFFF8000, 0xFFFF0000, 0xFFFE0000,
		0xFFFC0000, 0xFFF80000, 0xFFF00000, 0xFFE00000, 0xFFC00000, 0xFF800000,
		0xFF000000, 0xFE000000, 0xFC000000, 0xF8000000, 0xF0000000, 0xE0000000,
		0xC0000000, 0x80000000, 0x00000000
};

 int isTestThroughput = 0;


void term_zips(mzip_t zips[])
{
	int i;
	for (i = 0; i < 4; i++)
		mzip_term(&zips[i]);
}

void uncompress_byte_stream(FILE *fin, mzip_t zips[], char *outs[], uint32_t chk)
{
	int i;
	char *p;
	char bhd[HDR_SIZE * 4] =
	{ 0 };
	btype_t btypes[4];

	double start;

	/*Read header in every block*/
	fread(bhd, 1, HDR_SIZE * 4, fin);
	for (i = 0, p = bhd; i < 4; i++)
	{
		/*In every block, the sum of each byte stream length and HDR_SIZE will be saved in zips[j].inlen*/
		unpack_header(p, &(btypes[i]), &(zips[i].inlen));
		p += HDR_SIZE;
		fread(zips[i].in, 1, zips[i].inlen, fin);
	}

	for (i = 0; i < 4; i++)
	{
		start = now_sec();
		/*uncompress each byte stream, the uncompress data will be saved in outs[i]*/
		zips[i].unzipfun(&(zips[i]), btypes[i], chk, &(outs[i]));
		zips[i].time2 += (now_sec() - start);
	}
	return;
}

void apply_mask(float *buffer, int startIndex, int num, const int bitsToErase,
		int isFirstChk)
{

	int *p = (int *) buffer;

	/**
	 *  We should ignore first 1024 bytes in first chunk since it is the header info
	 */
	if (isFirstChk)
	{
		startIndex = startIndex + 256;
		p = p + 256;
	}

	for (int i = startIndex; i < num; i++)
	{

		*p = (*p) & bitsMaskTable[bitsToErase];
		p = p + 1;
	}
}

void apply_mask_to_int(int *buffer, int startIndex, int num, const int bitsToErase,
		int isFirstChk)
{

	int *p = buffer;

	/**
	 *  We should ignore first 1024 bytes in first chunk since it is the header info
	 */
	if (isFirstChk)
	{
		startIndex = startIndex + 256;
		p = p + 256;
	}

	for (int i = startIndex; i < num; i++)
	{

		*p = (*p) & bitsMaskTable[bitsToErase];
		p = p + 1;
	}
}

void split_float_to_byte_stream(float *buf, int num, char *zins[], int bitsToMask,	int isFirstChk)
{
	char *p0, *p1, *p2, *p3;
	char *p = (char*) buf;

	p0 = zins[0];
	p1 = zins[1];
	p2 = zins[2];
	p3 = zins[3];

	/**
	 *  1024 Bytes in the first chunk is the header info in MRC file, we should not apply lossy compression to it
	 */
	apply_mask(buf, 0, num, bitsToMask, isFirstChk);
	/**
	 * Make zips[i] to corresponding byte stream
	 */
	for (int i = 0; i < num; i++)
	{
		*p0++ = *p++;
		*p1++ = *p++;
		*p2++ = *p++;
		*p3++ = *p++;
	}
}

void merge_byte_to_float_stream(float *buf, int num, char *zouts[])
{
	int i;
	char *p0, *p1, *p2, *p3;
	char *p = (char*) buf;

	p0 = zouts[0];
	p1 = zouts[1];
	p2 = zouts[2];
	p3 = zouts[3];
	for (i = 0; i < num; i++)
	{
		*p++ = *p0++;
		*p++ = *p1++;
		*p++ = *p2++;
		*p++ = *p3++;
	}
	return;
}

void split_int_to_byte_stream(float *buf, int num, char *zins[], int bitsToMask,	int isFirstChk)
{

	fprintf(stderr, "Start to covert float stream to int stream\n");

	int *intBuffer = (int *)malloc(num * sizeof(int));
	for(int i = 0; i < num; i ++)
	{
		intBuffer[i] = (int)(buf[i] * 100.0);
	}

	char *p0, *p1, *p2, *p3;
	char *p = (char*) intBuffer;

	p0 = zins[0];
	p1 = zins[1];
	p2 = zins[2];
	p3 = zins[3];

	/**
	 *  1024 Bytes in the first chunk is the header info in MRC file, we should not apply lossy compression to it
	 */
	apply_mask_to_int(intBuffer, 0, num, bitsToMask, isFirstChk);
	/**
	 * Make zips[i] to corresponding byte stream
	 */
	for (int i = 0; i < num; i++)
	{
		*p0++ = *p++;
		*p1++ = *p++;
		*p2++ = *p++;
		*p3++ = *p++;
	}
}

void merge_byte_to_int_stream(float *buf, int num, char *zouts[])
{
	int i;
	char *p0, *p1, *p2, *p3;
	char *p = (char*) buf;

	p0 = zouts[0];
	p1 = zouts[1];
	p2 = zouts[2];
	p3 = zouts[3];
	for (i = 0; i < num; i++)
	{
		*p++ = *p0++;
		*p++ = *p1++;
		*p++ = *p2++;
		*p++ = *p3++;


	}
	return;
}



int run_uncompress(FILE *fin, ctx_t *ctx, mrczip_header_t *hd, FILE *fout)
{
	uint32_t i, j, num;
	uint32_t chk_size;
	uint64_t fsz;
	float *buf;
	mzip_t zips[4]; /* points to 4 bytes stream in float stream */
	char *outs[4];
	double start;

	fsz = hd->fsz / 4;
	chk_size = hd->chk;
	start = now_sec();
	buf = (float*) malloc(sizeof(float) * chk_size);

	for (j = 0; j < 4; j++)
		init_mrc_zip_stream(&(zips[j]), chk_size, hd->ztypes[j] + 1, 0);

	ctx->unzipTime += (now_sec() - start);

	/*Calculate how many chunks we have*/
	num = fsz / chk_size;
	for (i = 0; i < num; i++)
	{
		uncompress_byte_stream(fin, zips, outs, chk_size);
		merge_byte_to_float_stream(buf, chk_size, outs);

//		for(int i = 0; i < chk_size; i ++ )
//		{
//			buf[i] = buf[i] / 100.0f;
//		}

//#ifdef _OUTPUT_UNZIP_

		//if(_OUTPUT_UNZIP_ == 1)
		if(isTestThroughput  != 1)
		{
			//printf("======> start to write file1\n");
			fwrite(buf, sizeof(float), chk_size, fout);
		}

//#endif
	}

	chk_size = fsz % chk_size;
	if (chk_size > 0)
	{
		uncompress_byte_stream(fin, zips, outs, chk_size);

		start = now_sec();
		merge_byte_to_float_stream(buf, chk_size, outs);
		ctx->unzipTime += (now_sec() - start);

//#ifdef _OUTPUT_UNZIP_
	//if(_OUTPUT_UNZIP_ == 1)
	if(isTestThroughput != 1)
	{
		//printf("======> start to write file1\n");
		fwrite(buf, sizeof(float), chk_size, fout);
	}
//#endif
	}

	free(buf);

#ifdef _PRINT_ZIPS_
	print_result(zips, 4, "Decompress Result Info");
#endif
	for (j = 0; j < 4; j++)
	{
		ctx->allFileSize += zips[j].fsz;
		ctx->allZipFileSize += zips[j].zfsz;
		ctx->unzipTime += zips[j].time2;
		mzip_term(&(zips[j]));
	}

	return 0;
}

int run_compress(FILE *fin, ctx_t *ctx, FILE *fout, const int bitsToMask)
{
	int j;
	int num;
	mrczip_header_t hd;
#ifdef _OUTPUT_ZIP_
	char bhd[COMPRESSION_PATH_NUM * HDR_SIZE] = 	{ 0 };
#endif
	float *buf;

	mzip_t zips[COMPRESSION_PATH_NUM];
	char *zins[COMPRESSION_PATH_NUM], *zouts[COMPRESSION_PATH_NUM];
	int32_t zlens[COMPRESSION_PATH_NUM];

	double begin, zbegin; //for timing

	begin = now_sec();
	buf = (float*) malloc(sizeof(float) * CHUNK_SIZE);
	if (buf == NULL)
	{
		fprintf(stderr, "[%s:%d] ERROR: fail to alloc mem\n", __FILE__, __LINE__);
		exit(-1);
	}
	/*
	 * Firstly, we init 4 compressor for each byte of mrc float stream
	 * */
	for(int i = 0; i < COMPRESSION_PATH_NUM; i ++)
	{
		init_mrc_zip_stream(&zips[i], CHUNK_SIZE, ZLIB_DEF, ZIP_FAST);
//		init_mrc_zip_stream(&zips[1], CHUNK_SIZE, ZLIB_DEF, ZIP_FAST);
//		init_mrc_zip_stream(&zips[2], CHUNK_SIZE, ZLIB_DEF, ZIP_FAST);
//		init_mrc_zip_stream(&zips[3], CHUNK_SIZE, ZLIB_DEF, ZIP_FAST);
	}

	/*
	 * Make zins[i] point to compressed data buffer of 4 zip streams
	 * */
	for (j = 0; j < 4; j++)
	{
		zins[j] = zips[j].zin;
	}

	/* begin to write zip file header */
	init_mrczip_header(&hd, 0);
	hd.chk = CHUNK_SIZE;

	for (j = 0; j < 4; j++)
	{
		hd.ztypes[j] = zips[j].ztype;
	}

	ctx->zipTime += (now_sec() - begin);
	hd.fsz = get_file_size(fin);

	num = fread(buf, sizeof(uint32_t), CHUNK_SIZE, fin);

	/**
	 *  if we have  data need to be compressed, we will firstly write header into zip file, the zip file format is defined as following:
	 *  8 bytes: original file size
	 *  4 bytes: pre-defined chunk size
	 *  1 byte: compressed type,like compress_0,compress_1, compress_2, compress_3
	 *  5 bytes: compress method, like ZLIB_DEF, LZ4_DEF, LZ4HC_DEF，  for each byte stream
	 *  N bytes compress data organized as compressed chunk list:
	 *      each compressed chunk is organized as following:
	 *          16 bytes chunk header: every 4 bytes indicate the length of each compressed byte stream
	 *          4 compressed byte stream data: first compressed stream data is the first byte data in the incomming float number sequence, second compressed stream data is the second byte data in the incomming float number sequence, ...
	 */
	if (num > 0)
	{
//#ifdef _OUTPUT_ZIP_
		//if(_OUTPUT_ZIP_ == 1)
		if(isTestThroughput != 1) // we will ot write file if we want to test throughput, 1 means we are going to test throughput
		{
			write_mrczip_header(fout, &hd);
		}
//#endif
	}

	int isFirstChk = 1;
	while (num > 0)
	{
		begin = now_sec();
		split_float_to_byte_stream(buf, num, zins, bitsToMask, isFirstChk);
		//split_int_to_byte_stream(buf, num, zins, bitsToMask, isFirstChk);
		/**
		 *  We have processed the first chunk, so from next loop, this flag should be false
		 */
		isFirstChk = 0;
		ctx->zipTime += (now_sec() - begin);

		/**
		 *  TODO:we can select the compress method for every byte stream here, for example:
		 *  set the corresponding method value  in hd.ztypes[j]
		 */
		begin = now_sec();
		for (j = 0; j < 4; j++)
		{
			zbegin = now_sec();
			zips[j].inlen = num;

			/**
			 *  Here will invoke the mzlib_def() to compress the data
			 *  for each chunk;
			 *    4 input  byte streams are pointed by zips[0..3];
			 *    4 output compressed byte streams are pointed by zouts[0..3];
			 *    the length of each compressed byte stream is stored in zlens[0..3]
			 */
			zips[j].zipfun(&(zips[j]), &zouts[j], &zlens[j]);
			//mzlib_def(&(zips[j]), &zouts[j], &zlens[j]);
			zips[j].compressTime += (now_sec() - zbegin);
		}
		ctx->zipTime += (now_sec() - begin);

//#ifdef _OUTPUT_ZIP_
		//if(_OUTPUT_ZIP_ == 1)
		if(isTestThroughput != 1)
		{
			/**
			 * The first 4 bytes data in zouts[j][0..3] is the length of each compressed byte stream, we extracted from zouts[0..3] respectly, and write the the header of each compress blocked
			 */
			for (j = 0; j < 4; j++)
				memcpy(bhd + j * HDR_SIZE, zouts[j], HDR_SIZE);

			fwrite(bhd, 1, 4 * HDR_SIZE, fout);

			/**
			 *  Write the compressed data of each chunk to the zip file, the start position of each compressed byte stream is from zouts[j] + HDR_SIZE, where HDR_SIZe = 4
			 */
			for (j = 0; j < 4; j++)
				fwrite(zouts[j] + HDR_SIZE, 1, zlens[j] - HDR_SIZE, fout);
	}
//#endif

		num = fread(buf, sizeof(float), CHUNK_SIZE, fin);
	}

	free(buf);

#ifdef _PRINT_ZIPS_
	print_result(zips, 4, "Compression Summary Result");
#endif

	/**
	 *  ctx->zfsz stored  the sum of the length of each compressed byte stream
	 */
	for (j = 0; j < 4; j++)
		ctx->allZipFileSize += zips[j].zfsz;

	for (j = 0; j < 4; j++)
		mzip_term(&zips[j]);
	return 0;
}

