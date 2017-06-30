/*******************************************************************
 *       Filename:  mrccounter.c                                     
 *                                                                 
 *    Description:                                        
 *                                                                 
 *        Version:  1.0                                            
 *        Created:  2016年11月15日 09时34分07秒                                 
 *       Revision:  none                                           
 *       Compiler:  gcc                                           
 *                                                                 
 *         Author:  Ruan Huabin                                      
 *          Email:  ruanhuabin@tsinghua.edu.cn                                        
 *        Company:  Dep. of CS, Tsinghua Unversity                                      
 *                                                                 
 *******************************************************************/

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <map>
#include <string>
#include <iostream>
#include <utility>
#include <algorithm>
#include <vector>
using namespace std;
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

int mycompare(const void *a, const void *b)
{
    int ia = *(int *)a;
    int ib = *(int *)b;

    if(ia < ib)
    {
        return 1;
    }
    else if(ia > ib)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}
void print_freq_info(int *freq_info, int n)
{

    //qsort(freq_info, n, sizeof(int), mycompare);

    for(int i = 0; i < n; i ++)
    {
        if(freq_info[i] != 0)
        {
           printf("%6d:%10d\n", i - 127, freq_info[i]);
        }
     

    }
}

void count_freq(float *data, int n, int *freq_info)
{
    short int currValue = 0;
    for(int i = 0; i < n; i ++)
    {
        currValue = (short)round(data[i]);
        //some values are negative, so we add an offset with value 127 to be the array's index
        freq_info[currValue + 127] ++;
    }
}


void count_freq2(float *data, int n , map<string, int> &freq_info)
{
    char currValue[16];
    memset(currValue, '\0', sizeof(currValue));
    for(int i = 0; i < n; i ++)
    {
        snprintf(currValue, 16, "%.2f", data[i]);
        string strCurrValue = currValue;
        freq_info[strCurrValue] ++;
    }
}

void print_freq_info2(map<string, int> &freq_info)
{

    map<string, int>::iterator iter = freq_info.begin();
    for(; iter != freq_info.end(); iter ++)
    {
        cout<<iter->first<<":"<<iter->second<<endl;
    }
}

void print_freq_info3(vector<pair<string,int> > &freq_info, size_t totalNumItems)
{

    static size_t totalItems = 0;
    float percent = 0.0f;
    char percentStr[16];
    for(size_t i = 0; i < freq_info.size(); i ++)
    {
        totalItems += freq_info[i].second;

        percent = (float)freq_info[i].second / (float)totalNumItems;
        snprintf(percentStr, 16, "%.2f", percent * 100.0);
        strcat(percentStr, "%\0");
        cout<<freq_info[i].first<<" : "<<freq_info[i].second<<" ( " << percentStr <<" ) "<<endl;
    }

    cout<<"Total Items: " << totalItems <<endl;
    
}


struct CmpByValue {  
      bool operator()(const pair<string,int> & lhs, const pair<string,int> & rhs)
      {
          return lhs.second > rhs.second;
      }  
};

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
    size_t nx = mrcInstance.header.nx;
    size_t ny = mrcInstance.header.ny;
    size_t nz = mrcInstance.header.nz;

    /*
     *int *freq_info = (int *)malloc(1024 * sizeof(int));
     */

    map<string, int> freq_info;
    /*
     *for(int i = 0; i < 1024; i ++)
     *{
     *    freq_info[i] = 0;
     *}
     */
    mrcInstance.imageData = (float *) malloc(nx * ny * sizeof(float));
    if (mrcInstance.imageData == NULL)
    {
        printf("Memory allocate failed: [%s:%d]\n", __FILE__, __LINE__);
        exit(-1);
    }

    
    nz = 10;
    for(size_t i = 0; i < nz; i ++)
    {
        fread(mrcInstance.imageData, sizeof(float), nx * ny, mrcFile);
        printf("Start to count frame: %ld\n", i);
        count_freq2(mrcInstance.imageData, nx * ny, freq_info);
    }


    vector<pair<string,int> > freq_vector(freq_info.begin(),freq_info.end());
    sort(freq_vector.begin(),freq_vector.end(),CmpByValue());


    print_freq_info3(freq_vector, nx * ny * nz);

    /*
     *free(freq_info);
     */
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
 *
 *    count_freq(mrcInstance.imageData, nx * ny * nz, freq_info);
 *
 *    print_freq_info(freq_info, 1024);
 *
 *    free(freq_info);
 *
 *    free(mrcInstance.imageData);
 *    //print_mrc_data(mrcInstance.imageData, nx, ny, nz);
 *    fclose(mrcFile);
 *    return 0;
 */
}

