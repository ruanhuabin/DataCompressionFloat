#!/bin/bash - 
#===============================================================================
#
#          FILE:  run_full_test.sh
# 
#         USAGE:  ./run_full_test.sh 
# 
#   DESCRIPTION:  
# 
#       OPTIONS:  ---
#  REQUIREMENTS:  ---
#          BUGS:  ---
#         NOTES:  ---
#        AUTHOR: Ruan Huabin <ruanhuabin@gmail.com>
#       COMPANY: Tsinghua University
#       CREATED: 2016年08月15日 12时07分48秒 CST
#      REVISION:  ---
#===============================================================================

set -o nounset                              # Treat unset variables as an error
projectRoot=/root/datacompression/DataCompressionFloat/
appName=${projectRoot}/script/mrc_tarx_c
timestamp=`date +%Y-%m-%d:%H:%M:%S`
bitsToMask=0
if [ "$#" -eq 1 ]
then
    bitsToMask=$1
fi


function compress_and_uncompress()
{
    bitsToMask=$1
    echo >&2 "bitsToMask = ${bitsToMask}"
    inputFileName="stack.mrc"
    inputFilePrefix=`echo $inputFileName | cut -d . -f 1`
    inputFileSuffix=.`echo $inputFileName | cut -d . -f 2`

    inputFile=$inputFilePrefix$inputFileSuffix
    outputFile=ys_${inputFilePrefix}_${timestamp}_mask${bitsToMask}bits${inputFileSuffix}
    echo >&2 "output file name: $outputFile"

    inputFilePath=$projectRoot/data/${inputFile}
    outputFilePath=$projectRoot/tmp/${outputFile}

    echo >&2 "input file path: ${inputFilePath} "
    echo >&2 "output file path: ${outputFilePath} "

    zipCMDLineArgs=" -i $inputFilePath -o ${outputFilePath} -b ${bitsToMask} -t zip"
    echo >&2 "Compress Command: ${appName} ${zipCMDLineArgs}"
    #run compression
    ${appName} ${zipCMDLineArgs} 1>&2

    sleep 1
    #run decompression
    jyOutputFilePath=$projectRoot/tmp/jy_${inputFilePrefix}_${timestamp}_mask${bitsToMask}bits${inputFileSuffix}
    unzipCMDLineArgs=" -i ${outputFilePath} -o ${jyOutputFilePath} -t unzip"
    echo >&2 "Decompress Command: ${appName} ${unzipCMDLineArgs}"

    ${appName} ${unzipCMDLineArgs} 1>&2
    sleep 1
    #diff ${inputFilePath} ${jyOutputFilePath} 1>&2
    #if [ $? -eq 0 ]
    #then
        #echo >&2 "Comparing result is correct "
    #else
        #echo >&2 "Comparing result is wrong!!!"
    #fi

    echo >&2 -e "----------------------------------------------------------\n\n\n"

    echo "$jyOutputFilePath"
}

count=0
N=4
if [ "$#" -eq 1 ]
then
    N=$1
fi

N=$[N+1]
for((i=0; i<${N}; i++))
do
    result=$(compress_and_uncompress $i)
    
   referenceFile="maskdata-mrc-large/mask${i}bits.mrc"

   
   diff ${referenceFile} ${result}

   if [ $? -eq 0 ]
   then
       echo "file [ $referenceFile ] and [ $result ] is the same"
       echo "---->:Current bits to mask: $count"
       count=$[count+1]
   else
       echo "====>Error: file [$referenceFile] and [$result] is different"
       echo "Current bits to mask: $count"
       break 
   fi
done


if [ $count -eq $N ]
then
    echo "All Tests are Passed: $count"
else
    echo "!!!Test Failed!!!"
fi
