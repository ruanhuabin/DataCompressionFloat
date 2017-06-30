/*******************************************************************
 *       Filename:  mrcviewer2.c                                     
 *                                                                 
 *    Description:                                        
 *                                                                 
 *        Version:  1.0                                            
 *        Created:  2017年06月30日 10时01分25秒                                 
 *       Revision:  none                                           
 *       Compiler:  gcc                                           
 *                                                                 
 *         Author:  Ruan Huabin                                      
 *          Email:  ruanhuabin@tsinghua.edu.cn                                        
 *        Company:  Dep. of CS, Tsinghua Unversity                                      
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

} mrc_header_t;

typedef struct _mrc_fmt_
{
    mrc_header_t header;
    float *imageData;
} mrc_fmt_t;

void print_mrc_header(mrc_header_t *header)
{
    printf("Number of colums  : nx  = %d\n", header->nx);
    printf("Number of rows    : ny  = %d\n", header->ny);
    printf("Number of sections: nz  = %d\n", header->nz);
    printf("image data type   : mod = %d\n", header->mod);
}

void print_mrc_data(float *data, int nx, int ny, int nz)
{
    for (int i = 0; i < nz; i++)
    {
        printf("Sections: %d\n", i);

        for (int j = 0; j < ny; j++)
        {
            for (int k = 0; k < nx; k++)
            {
                printf("%.6f ", data[i * ny * nx + j * ny + k]);
            }

            printf("\n");
        }

        printf("\n");
    }
}
void print_mrc_data2(float *data, int nx, int ny, int nz)
{
    for (int i = 0; i < nz; i++)
    {
        printf("Sections: %d\n", i);

        for (int j = 0; j < ny; j++)
        {
            for (int k = 0; k < nx; k++)
            {
                printf("%.6f\n", data[i * ny * nx + j * ny + k]);
            }

        }

        printf("\n");
    }
}

void print_mrc_data3(float *data, int nx, int ny, int frameIndex)
{
    printf("Sections: %d\n", frameIndex);
    
    for(int i = 0; i < nx; i ++)
    {
        for(int j = 0; i < ny; j ++)
        {
            printf("%.6f\t", data[i * ny + j]);

            if( (j + 1) % 8 == 0)
            {
                printf("\n");
            }
        }
    }
}
int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Usage: %s <mrc file name>\n", argv[0]);
        printf("e.g: %s image.mrc\n", argv[0]);
        exit(-1);
    }

    FILE *mrcFile = fopen(argv[1], "rb");

    if (mrcFile == NULL)
    {
        printf("Open input file [%s] failed: [%s:%d]", argv[1], __FILE__,
               __LINE__);
        exit(-1);
    }


    mrc_fmt_t mrcInstance;
    fread(&mrcInstance.header, sizeof(mrc_header_t), 1, mrcFile);
    print_mrc_header(&mrcInstance.header);
    int nx = mrcInstance.header.nx;
    int ny = mrcInstance.header.ny;
    int nz = mrcInstance.header.nz;



    mrcInstance.imageData = (float *) malloc(nx * ny * sizeof(float));
    if (mrcInstance.imageData == NULL)
    {
        printf("Memory allocate failed: [%s:%d]\n", __FILE__, __LINE__);
        exit(-1);
    }
    for(int i = 0; i < nz; i ++)
    {
        fread(mrcInstance.imageData, sizeof(float), nx * ny, mrcFile);
        print_mrc_data3(mrcInstance.imageData, nx, ny, i);
    }



    free(mrcInstance.imageData);
    fclose(mrcFile);


    return 0;
/*
 *    mrcInstance.imageData = (float *) malloc(nx * ny * nz * sizeof(float));
 *
 *    if (mrcInstance.imageData == NULL)
 *    {
 *        printf("Memory allocate failed: [%s:%d]\n", __FILE__, __LINE__);
 *        exit(-1);
 *    }
 *
 *    fread(mrcInstance.imageData, sizeof(float), nx * ny * nz, mrcFile);
 *    print_mrc_data2(mrcInstance.imageData, nx, ny, nz);
 *    fclose(mrcFile);
 */
    return 0;
}

