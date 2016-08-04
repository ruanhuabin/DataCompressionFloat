/*******************************************************************
 *       Filename:  mrcviewer.c                                     
 *                                                                 
 *    Description:                                        
 *                                                                 
 *        Version:  1.0                                            
 *        Created:  2016年08月03日 09时24分27秒                                 
 *       Revision:  none                                           
 *       Compiler:  gcc                                           
 *                                                                 
 *         Author:  Ruan Huabin                                      
 *          Email:  ruanhuabin@gmail.com                                        
 *        Company:  HPC tsinghua                                      
 *                                                                 
 *******************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
typedef struct _mrc_header__
{
	/* Number of columns, rows, and sections */
	int nx;
	int ny;
	int nz;

	/* Type of value in image:
	 * 0 =unsigned or signed bytes depending on flag in imodStamp,only unsigned bytes before IMOD 4.2.23
	 * 1 = signed short integers (16 bits)
	 * 2 = float
	 * 3 = short * 2, (used for complex data)
	 * 4 = float * 2, (used for complex data)
	 * 6 = unsigned 16-bit integers (non-standard)
	 * 16 = unsigned char * 3 (for rgb data, non-standard)
	 * */
	int mod;
	
	/* Start point of sub image */
	int nxstart;
	int nystart;
	int nzstart;
	
	/* Grid size in X, Y, and Z */
	int mx;
	int my;
	int mz;

	/* Cell size: pixel spacing = xlen/max, ylen/my, zlen/mz */
	float xlen;
	float ylen;
	float zlen;

	/* cell angles */
	float alpha;
	float beta;
	float gamma;

	int mapc;
	int mapr;
	int maps;
	
	float amin;
	float amax;
	float amean;

	int ispg;
	int next;

	char otherparts[1024 - 24 * 4];

}mrc_header_t;

typedef struct _mrc_fmt_
{
	mrc_header_t header;
	float *imageData;
}mrc_fmt_t;

void print_mrc_header(mrc_header_t *header)
{
	printf("Number of colums  : nx  = %d\n", header->nx);
	printf("Number of rows    : ny  = %d\n", header->ny);
	printf("Number of sections: nz  = %d\n", header->nz);
	printf("image data type   : mod = %d\n", header->mod);

}

void print_mrc_data(float *data, int nx, int ny, int nz)
{
	for(int i = 0; i < nz; i ++)
	{
		printf("Sections: %d\n", i);
		for(int j = 0; j < ny; j ++)
		{
			for(int k = 0; k < nx; k ++)
			{
				printf("%.6f ", data[i * ny * nx + j * ny + k]);
			}
			printf("\n");
		}

		printf("\n");
	}
}

int main(int argc, char **argv)
{

	if(argc != 2)
	{
		printf("Usage: %s <mrc file name>\n", argv[0]);
		printf("e.g: %s image.mrc\n", argv[0]);
		exit(-1);
	}


	FILE *mrcFile = fopen(argv[1], "rb");
	
	if(mrcFile == NULL)
	{
		printf("Open input file [%s] failed: [%s:%d]", argv[1], __FILE__, __LINE__);
		exit(-1);
	}



	mrc_fmt_t mrcInstance;
	fread(&mrcInstance.header, sizeof(mrc_header_t), 1, mrcFile);
	print_mrc_header(&mrcInstance.header);

	int nx = mrcInstance.header.nx;
	int ny = mrcInstance.header.ny;
	int nz = mrcInstance.header.nz;

	mrcInstance.imageData = (float *)malloc(nx * ny * nz * sizeof(float));
	if(mrcInstance.imageData == NULL)
	{
		printf("Memory allocate failed: [%s:%d]\n", __FILE__, __LINE__);
		exit(-1);
	}

	fread(mrcInstance.imageData, sizeof(float), nx * ny * nz, mrcFile);

	print_mrc_data(mrcInstance.imageData, nx, ny, nz);


	fclose(mrcFile);

	return 0;
}
