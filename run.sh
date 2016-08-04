#!/bin/bash - 
#===============================================================================
#
#          FILE:  run.sh
# 
#         USAGE:  ./run.sh 
# 
#   DESCRIPTION:  
# 
#       OPTIONS:  ---
#  REQUIREMENTS:  ---
#          BUGS:  ---
#         NOTES:  ---
#        AUTHOR: YOUR NAME (), 
#       COMPANY: 
#       CREATED: 2016年08月04日 14时55分37秒 CST
#      REVISION:  ---
#===============================================================================
timestamp=`date +%Y-%m-%d:%H:%M:%S`
dimInfo="11522 5186 3"
inputFileName="test_large.nc"
inputFilePrefix=`echo $inputFileName | cut -d . -f 1`
inputFileSuffix=.`echo $inputFileName | cut -d . -f 2`
#echo "inputFilePrefix = ${inputFilePrefix}, inputFileSuffix = ${inputFileSuffix}"

inputFile=$inputFilePrefix$inputFileSuffix
outputFile=ys_${inputFilePrefix}_${timestamp}${inputFileSuffix}
echo "output file name: $outputFile"

inputFilePath=../data/${inputFile}
outputFilePath=./tmp/${outputFile}

echo "input file path: ${inputFilePath} "
echo "output file path: ${outputFilePath} "

#run compression
./hpos -oz $inputFilePath ${outputFilePath} ${dimInfo} 

#sleep 1
#run decompression
jyOutputFilePath=tmp/jy_${inputFilePrefix}_${timestamp}${inputFileSuffix}
./hpos -ou $outputFilePath ${jyOutputFilePath}

#sleep 1
diff ${inputFilePath} ${jyOutputFilePath}
if [ $? -eq 0 ]
then
    echo "Comparing result is correct "
else
    echo "Comparing result is wrong!!!"
fi

