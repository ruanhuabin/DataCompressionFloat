#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <assert.h>
#include "zlib.h"
#include "lz4.h"
#include "lz4hc.h"
#include "nzips.h"
#include "common.h"
#include "constant.h"


void _lz4_def(mzip_t *zip, char **p, int *len)
{
	if (*len > 0)
	{
		*p = zip->out;
		pack_header(*p, COMPRESSED, *len);
		*len += HDR_SIZE;
	}
	else
	{
		*p = zip->in;
		pack_header(*p, RAW, zip->inlen);
		*len = zip->inlen + HDR_SIZE;
	}

	//TIP:for accounting
	zip->fsz += zip->inlen;
	zip->zfsz += *len;
}

int mlz4_def(mzip_t *zip, char **p, int *len)
{
	//*len is 0 if fails
	*len = LZ4_compress_limitedOutput(zip->zin, zip->zout, zip->inlen,
			zip->inlen);

	_lz4_def(zip, p, len);
	return 0;
}

int mlz4hc_def(mzip_t *zip, char **p, int *len)
{
	*len = LZ4_compressHC_limitedOutput(zip->zin, zip->zout, zip->inlen,
			zip->inlen);
	_lz4_def(zip, p, len);
	return 0;
}

int mlz4_inf(mzip_t *zip, btype_t btype, int len, char **p)
{
	if (btype == RAW)
	{
		*p = zip->in;
	}
	else
	{
		LZ4_uncompress(zip->in, zip->out, len);
		*p = zip->out;
	}

	//TIP: for accounting
	zip->fsz += len;
	zip->zfsz += zip->inlen;
	return 0;
}

/*-------- zlib --------*/
int _mzlib_init(z_stream* strm)
{
	//minor
	strm->zalloc = Z_NULL;
	strm->zfree = Z_NULL;
	strm->opaque = Z_NULL;
	strm->data_type = Z_BINARY; //no difference

	//the z_stream memory
	strm->next_in = NULL;
	strm->avail_in = 0;
	strm->total_in = 0;
	strm->next_out = NULL;
	strm->avail_out = 0;
	strm->total_out = 0;

	return 0;
}

int zlib_def_init(z_stream *strm, int level, int strategy)
{
	int ret;
	int window, memlevel;

	//strategy = ZIP_STRATEGY;//Z_DEFAULT_STRATEGY,
	//strategy = Z_DEFAULT_STRATEGY;//Z_DEFAULT_STRATEGY,
	window = ZIP_WINDOW_BITS;
	memlevel = 9;
	ret = deflateInit2(strm, level, Z_DEFLATED, window, memlevel, strategy);
	if (ret != Z_OK)
	{
		fprintf(stderr, "zlib_init failed\n");
		return ret;
	}
	return 0;
}

z_stream* new_zlib_def(int level, int strategy)
{
	z_stream *strm = malloc(sizeof(z_stream));
	if (strm == NULL)
	{
		fprintf(stderr, "fail malloc\n");
		return NULL;
	}

	_mzlib_init(strm);
	zlib_def_init(strm, level, strategy);
	return strm;
}

z_stream* new_zlib_inf()
{
	int window, ret;
	z_stream *strm = malloc(sizeof(z_stream));
	if (strm == NULL)
	{
		fprintf(stderr, "fail malloc\n");
		return NULL;
	}

	_mzlib_init(strm);

	window = ZIP_WINDOW_BITS;
	ret = inflateInit2(strm, window);
	if (ret != Z_OK)
	{
		fprintf(stderr, "zlib_init failed\n");
		return NULL;
	}
	return strm;
}

int mzlib_def(mzip_t *zip, char **p, int *len)
{
	z_stream *strm = (z_stream*) zip->zipper;

	strm->next_in = (unsigned char*) zip->zin;
	strm->avail_in = (unsigned) zip->inlen;

	strm->next_out = (unsigned char*) zip->zout;
	strm->avail_out = zip->chk;

	/**
	 *  The Z_FULL_FLUSH May need to change to Z_NO_FLUSH or Z_FINISH for optimization
	 */
	deflate(strm, Z_FULL_FLUSH);

	*len = zip->chk - strm->avail_out;
	if (zip->inlen > *len + HDR_SIZE)
	{
		//compressable
		*p = zip->out;
		pack_header(*p, COMPRESSED, *len);
		*len += HDR_SIZE;
	}
	else //not compressable
	{
		*p = zip->in;
		pack_header(*p, RAW, zip->inlen);
		*len = zip->inlen + HDR_SIZE;
	}

	//TIP:for accounting
	zip->fsz += zip->inlen;
	zip->zfsz += *len;
	return 0;
}

int mzlib_def_ex(mzip_t *zip, char **p, int *len, int flushFlag, FILE *fout)
{
	z_stream *strm = (z_stream*) zip->zipper;

	strm->next_in = (unsigned char*) zip->zin;
	strm->avail_in = (unsigned) zip->inlen;

	do
	{
		strm->next_out = (unsigned char*) zip->zout;
		strm->avail_out = zip->chk;

		deflate(strm, flushFlag);

		*len = zip->chk - strm->avail_out;
		if (zip->inlen > *len + HDR_SIZE)
		{
			//compressable
			*p = zip->out;
			pack_header(*p, COMPRESSED, *len);
			*len += HDR_SIZE;
		}
		else //not compressable
		{
			*p = zip->in;
			pack_header(*p, RAW, zip->inlen);
			*len = zip->inlen + HDR_SIZE;
		}

		//TIP:for accounting
		zip->fsz += zip->inlen;
		zip->zfsz += *len;

		fwrite(*p, 1, *len, fout);

	} while (strm->avail_out == 0);

	assert(strm->avail_in == 0);
	return 0;
}

void print_zerror(int ret)
{
	switch (ret)
	{
	case Z_NEED_DICT:
		fprintf(stderr, "Dict Error\n");
		break;
	case Z_DATA_ERROR:
		fprintf(stderr, "Data Error\n");
		break;
	case Z_MEM_ERROR:
		fprintf(stderr, "Mem Error\n");
		break;
	case Z_BUF_ERROR:
		fprintf(stderr, "Buf Error\n");
		break;
	default:
		fprintf(stderr, "unknown Error\n");
	}
}

int mzlib_inf(mzip_t *zip, btype_t btype, int len, char **p)
{
	if (btype == RAW)
	{
		*p = zip->in;
	}
	else
	{
		z_stream *strm = (z_stream*) zip->zipper;

		strm->next_in = (unsigned char*) (zip->in);
		strm->avail_in = (unsigned) zip->inlen;

		strm->next_out = (unsigned char*) (zip->out);
		strm->avail_out = zip->chk;

		inflate(strm, Z_FINISH);

		*p = zip->out;

	}

	//TIP: for accounting
	zip->fsz += len;
	zip->zfsz += zip->inlen;
	return 0;
}

/*-------- mzip --------*/

int mzip_init(mzip_t *zip, uint32_t chk, ztype_t ztype, int strategy)
{
	zip->ztype = ztype;
	switch (ztype)
	{
	case ZLIB_DEF:
		zip->zipper = new_zlib_def(LEVEL, strategy);
		zip->zipfun = mzlib_def;
		zip->unzipfun = NULL;
		break;
	case ZLIB_INF:
		zip->zipper = new_zlib_inf();
		zip->zipfun = NULL;
		zip->unzipfun = mzlib_inf;
		break;
	case LZ4_DEF:
	case LZ4_INF:
		zip->zipper = NULL;
		zip->zipfun = mlz4_def;
		zip->unzipfun = mlz4_inf;
		break;
	case LZ4HC_DEF:
	case LZ4HC_INF:
		zip->zipper = NULL;
		zip->zipfun = mlz4hc_def;
		zip->unzipfun = mlz4_inf;
		break;
	default:
		fprintf(stderr, "unkown zip type:%d\n", ztype);
		return -1;
	}

	if (chk >= MAX_BLOCK_SIZE)
	{
		fprintf(stderr, "too large chunk size\n");
		return -2;
	}

	zip->chk = chk;
	zip->inlen = 0;
	zip->outlen = 0;

	zip->in = malloc(HDR_SIZE + chk);
	zip->out = malloc(HDR_SIZE + chk);

	zip->zin = zip->in + HDR_SIZE;
	zip->zout = zip->out + HDR_SIZE;

	//accounting
	zip->fnum = 0;
	zip->fsz = 0;
	zip->zfsz = 0;
	zip->time1 = 0.0;
	zip->time2 = 0.0;
	return 0;
}

void mzip_term(mzip_t *zip)
{
	switch (zip->ztype)
	{
	case ZLIB_DEF:
		deflateEnd((z_stream*) zip->zipper);
		break;
	case ZLIB_INF:
		inflateEnd((z_stream*) zip->zipper);
		break;
	case LZ4_DEF:
	case LZ4_INF:
	case LZ4HC_DEF:
	case LZ4HC_INF:
		break;
	default:
		fprintf(stderr, "unkown zip type:%d\n", zip->ztype);
	}

	if (zip->zipper)
		free(zip->zipper);
	free(zip->in);
	free(zip->out);
}

/*----------- for header  ------------------- */
void pack_header(char *nbuf, btype_t btype, uint32_t len)
{
	unsigned char *buf = (unsigned char*) nbuf;
	buf[0] = len & 0xFF;
	buf[1] = (len >> 8) & 0xFF;
	buf[2] = (len >> 16) & 0xFF;
	buf[3] = (len >> 24) & 0xFF;
	buf[3] |= (btype << 7);

	//unpack_header(buf, &btype, &len);
	//printf("btype:%d, len:%d\n", btype, len);
}

void unpack_header(const char *nbuf, btype_t *btype, uint32_t *len)
{
	unsigned char *buf = (unsigned char*) nbuf;
	*btype = (buf[3] & 0x80) >> 7;
	*len = buf[0] | (buf[1] << 8) | (buf[2] << 16) | ((buf[3] & 0x7f) << 24);

	//if(*len == 0)
	//  *len = MAX_BLOCK_SIZE;
}

void displayResults(mzip_t *zips, int n, const char *hintMsg)
{
	int i;
	double m;
	uint64_t fsz, zfsz;
	double zipTime, unzipTime;

	fsz = 0.0;
	zfsz = 0.0;
	zipTime = 0.0;
	unzipTime = 0.0;
	m = 1024.0 * 1024.0;
	printf("-------------------%s Information--------------\n", hintMsg);

	const char *firstColumnHeader = "[ByteStreamIndex]   ";
	const char *secondColumnHeader = "[Before Compress(Bytes)]   ";
	const char *thirdColumnHeader = "[After Compress(Bytes)]   ";
	const char *fourthColumnHeader = "[Compress Ratio]   ";
	printf("%s%s%s%s\n", firstColumnHeader, secondColumnHeader,
			thirdColumnHeader, fourthColumnHeader);

	int firstColumnLen = strlen(firstColumnHeader);
	int secondColumnLen = strlen(secondColumnHeader);
	int thirdColumnLen = strlen(thirdColumnHeader);
	int fourthColumnLen = strlen(fourthColumnHeader);
	double compressRatio = 0.0;

	/**
	 *  Print compress ration for each byte stream
	 */
	for (i = 0; i < n; i++)
	{
		fsz += zips[i].fsz;
		zfsz += zips[i].zfsz;

		zipTime += zips[i].time1;
		unzipTime += zips[i].time2;

		compressRatio = (double) (zips[i].zfsz) / (double) (zips[i].fsz);
		printf("%-*d%-*ld%-*ld%-*.*f\n", firstColumnLen, i, secondColumnLen,
				zips[i].fsz, thirdColumnLen, zips[i].zfsz, fourthColumnLen, 4,
				compressRatio);
	}

	printf("%-*s", firstColumnLen, "Whole File");
	printf("%-*ld%-*ld%-*.*f\n", secondColumnLen, fsz, thirdColumnLen, zfsz,
			fourthColumnLen, 4, (double) (zfsz) / (double) (fsz));

	if (zipTime > 0.001)
	{
		printf("---------------------------------------\n");
		printf(
				"%s Overall: zipTime = %f, original file size = %ld, compressed file size = %ld, file reduced = %.4f%s\n",
				hintMsg, zipTime, fsz, zfsz,
				(1.0 - (double) (zfsz) / (double) (fsz)) * 100.0, "%");
		printf("---------------------------------------\n");
	}
	if (zipTime > 0.001)
	{
		printf("Compression Throughputs: %f MB/s\n", fsz / (m * zipTime));
		printf("---------------------------------------\n");
	}

	if (unzipTime > 0.001)
	{
		printf("---------------------------------------\n");
		printf("Decompression Throughput: %f MB/s\n", fsz / (m * unzipTime));
		printf("---------------------------------------\n");
	}

}

