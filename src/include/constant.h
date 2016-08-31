/*******************************************************************
 *       Filename:  constant.h                                     
 *                                                                 
 *    Description:                                         
 *                                                                 
 *        Version:  1.0                                            
 *        Created:  2016年08月14日 11时49分56秒                                 
 *       Revision:  none                                           
 *       Compiler:  gcc                                           
 *                                                                 
 *         Author:  Ruan Huabin                                      
 *          Email:  ruanhuabin@tsinghua.edu.cn                                        
 *        Company:  Dep. of CS, Tsinghua Unversity                                      
 *                                                                 
 *******************************************************************/


#ifndef CONSTANT_H_
#define CONSTANT_H_


#define ZIP_WINDOW_BITS (-15)
#define ZIP_COMPRESS_LEVEL (6)
#define ZIP_FAST Z_RLE
#define CHUNK_SIZE (6*1048576)

#define COMPRESSION_PATH_NUM 4

#define _OUTPUT_ZIP_ (1) //for testing throughput
#define _OUTPUT_UNZIP_ (1)  //for testing throughput
#define _PRINT_ZIPS_ (1)
#define _PRINT_DECISION_ (1)



#endif /* CONSTANT_H_ */
