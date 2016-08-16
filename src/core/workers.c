#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "workers.h"
#include "nzips.h"
#include "constant.h"


int bitsMask[] =
{     0xFFFFFFFF, 0xFFFFFFFE, 0xFFFFFFFC, 0xFFFFFFF8, 0xFFFFFFF0, 0xFFFFFFE0,
		0xFFFFFFC0, 0xFFFFFF80, 0xFFFFFF00, 0xFFFFFE00, 0xFFFFFC00, 0xFFFFF800,
		0xFFFFF000, 0xFFFFE000, 0xFFFFC000, 0xFFFF8000, 0xFFFF0000, 0xFFFE0000,
		0xFFFC0000, 0xFFF80000, 0xFFF00000, 0xFFE00000, 0xFFC00000, 0xFF800000,
		0xFF000000, 0xFE000000, 0xFC000000, 0xF8000000, 0xF0000000, 0xE0000000,
		0xC0000000, 0x80000000, 0x00000000
};

/* ---------------- aulix ----------------- */
void init_zips(mzip_t zips[], int chk)
{

	/**
	 *  Initial 4 zips to compress 4 byte stream in float number sequence
	 */
	mzip_init(&zips[0], chk, ZLIB_DEF, ZIP_FAST);
	mzip_init(&zips[1], chk, ZLIB_DEF, ZIP_FAST);
	mzip_init(&zips[2], chk, ZLIB_DEF, ZIP_FAST);
	mzip_init(&zips[3], chk, ZLIB_DEF, ZIP_FAST);
}

void term_zips(mzip_t zips[])
{
	int i;
	for (i = 0; i < 4; i++)
		mzip_term(&zips[i]);
}

/* ------- these two for uncompress ---------- 
 *  unzip_1 is for uncompress_0 and 1
 *  unzip_3 is for uncompress_2 and 3
 */
void _unzip_1(FILE *fin, mzip_t zips[], char *outs[], uint32_t chk)
{
	int j;
	char *p;
	char bhd[HDR_SIZE * 4] =
	{ 0 };
	btype_t btypes[4];

	double begin; //Timing

	//zip-block headers
	fread(bhd, 1, HDR_SIZE * 4, fin);
	for (j = 0, p = bhd; j < 4; j++)
	{
		unpack_header(p, &(btypes[j]), &(zips[j].inlen));
		p += HDR_SIZE;

		fread(zips[j].in, 1, zips[j].inlen, fin);
	}

	for (j = 0; j < 4; j++)
	{
		begin = now_sec();
		zips[j].unzipfun(&(zips[j]), btypes[j], chk, &(outs[j]));
		zips[j].time2 += (now_sec() - begin);
	}

	return;
}

void maskBitsEx(float *buffer, int startIndex, int num, const int bitsToErase,
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

		*p = (*p) & bitsMask[bitsToErase];
		p = p + 1;
	}
}

void splitFloatsEx(float *buf, int num, char *zins[], int bitsToMask,
		int isFirstChk)
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
	maskBitsEx(buf, 0, num, bitsToMask, isFirstChk);
	for (int i = 0; i < num; i++)
	{
		*p0++ = *p++;
		*p1++ = *p++;
		*p2++ = *p++;
		*p3++ = *p++;
	}
}

void _unconvert0(float *buf, int num, char *zouts[])
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

int run_decompress(FILE *fin, ctx_t *ctx, nz_header *hd, FILE *fout)
{
	uint32_t i, j, num;
	uint32_t chk;
	uint64_t fsz;
	float *buf;

	mzip_t zips[4]; /* 4 float zips, 1 zero zip */
	char *outs[4];

	double begin; // Timing

	fsz = hd->fsz / 4;
	chk = hd->chk;

	begin = now_sec();

	buf = (float*) malloc(sizeof(float) * chk);

	for (j = 0; j < 4; j++)
		mzip_init(&(zips[j]), chk, hd->ztypes[j] + 1, 0);

	ctx->unzipTime += (now_sec() - begin);

	num = fsz / chk;
	for (i = 0; i < num; i++)
	{
		_unzip_1(fin, zips, outs, chk);
		_unconvert0(buf, chk, outs);

#ifdef _OUTPUT_UNZIP_
		fwrite(buf, sizeof(float), chk, fout);
#endif
	}

	chk = fsz % chk;
	if (chk > 0)
	{
		_unzip_1(fin, zips, outs, chk);

		begin = now_sec();
		_unconvert0(buf, chk, outs);
		ctx->unzipTime += (now_sec() - begin);

#ifdef _OUTPUT_UNZIP_
		fwrite(buf, sizeof(float), chk, fout);
#endif
	}

	free(buf);

#ifdef _PRINT_ZIPS_
	displayResults(zips, 4, "Decompress");
#endif
	for (j = 0; j < 4; j++)
	{
		ctx->fsz += zips[j].fsz;
		ctx->zfsz += zips[j].zfsz;
		ctx->unzipTime += zips[j].time2;
		mzip_term(&(zips[j]));
	}

	return 0;
}

int run_compress(FILE *fin, ctx_t *ctx, FILE *fout, const int bitsToMask)
{
	int j;
	int num, chk;
	nz_header hd;
#ifdef _OUTPUT_ZIP_
	char bhd[4 * HDR_SIZE] = 	{ 0 };
#endif
	float *buf;

	mzip_t zips[4];
	char *zins[4], *zouts[4];
	int32_t zlens[4];

	double begin, zbegin; //for timing

	chk = CHUNK_SIZE;

	begin = now_sec();
	buf = (float*) malloc(sizeof(float) * chk);
	if (buf == NULL)
	{
		fprintf(stderr, "[ERROR]: fail to alloc mem\n");
		exit(-1);
	}

	init_zips(zips, chk);
	for (j = 0; j < 4; j++)
		zins[j] = zips[j].zin;

	/* begin to write zip file header */
	init_mrczip_header(&hd, 0);
	hd.chk = chk;

	for (j = 0; j < 4; j++)
		hd.ztypes[j] = zips[j].ztype;

	ctx->zipTime += (now_sec() - begin);
	hd.fsz = get_file_size(fin);

	num = fread(buf, sizeof(uint32_t), chk, fin);

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
#ifdef _OUTPUT_ZIP_ 
		nz_header_write(fout, &hd);
#endif
	}

	int isFirstChk = 1;
	while (num > 0)
	{

		begin = now_sec();
		/*
		 *_convert0(buf, num, zins);
		 */
		/*
		 *splitFloats(buf, num, zins, bitsToMask);
		 */
		splitFloatsEx(buf, num, zins, bitsToMask, isFirstChk);

		/**
		 *  We have processed the first chunk, so from next loop, this flag should be false
		 */
		isFirstChk = 0;
		ctx->zipTime += (now_sec() - begin);

		/**
		 *  TODO:we can select the compress method for every byte stream here, for example:
		 *  set the corresponding method value  in hd.ztypes[j]
		 */

		//adapt: select the compressor
		/*
		 *        if(flag == 0)
		 *        {
		 *            begin = now_sec();
		 *            for(j = 0; j < ADAPT_NUMZ; j++)
		 *            {
		 *                mzip_def_test0(&(zips[j]));
		 *                hd.ztypes[j] = zips[j].ztype;
		 *            }
		 *
		 *            ctx->time1 += (now_sec() - begin);
		 *#ifdef _OUTPUT_ZIP_
		 *            nz_header_write(fout, &hd);
		 *#endif
		 *            flag = 1;
		 *        }
		 */
		//end of select compressors
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
			zips[j].time1 += (now_sec() - zbegin);
		}
		ctx->zipTime += (now_sec() - begin);

#ifdef _OUTPUT_ZIP_

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
#endif

		num = fread(buf, sizeof(float), chk, fin);
	}

	free(buf);

#ifdef _PRINT_ZIPS_
	displayResults(zips, 4, "Compression");
#endif

	/**
	 *  ctx->zfsz stored  the sum of the length of each compressed byte stream
	 */
	for (j = 0; j < 4; j++)
		ctx->zfsz += zips[j].zfsz;

	for (j = 0; j < 4; j++)
		mzip_term(&zips[j]);
	return 0;
}

