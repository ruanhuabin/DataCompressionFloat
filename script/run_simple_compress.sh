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
projectRoot=/root/datacompression/DataCompressionFloat
app=$projectRoot/script/simple_compress_c
timestamp=`date +%Y-%m-%d:%H:%M:%S`
dimInfo="1000 1000 32"
inputFileName="data1000x1000x32.mrc"
inputFilePrefix=`echo $inputFileName | cut -d . -f 1`
inputFileSuffix=.`echo $inputFileName | cut -d . -f 2`
#echo "inputFilePrefix = ${inputFilePrefix}, inputFileSuffix = ${inputFileSuffix}"

inputFile=$inputFilePrefix$inputFileSuffix
outputFile=ys_${inputFilePrefix}_${timestamp}${inputFileSuffix}
echo "output file name: $outputFile"

inputFilePath=$projectRoot/data/${inputFile}
outputFilePath=$projectRoot/tmp/${outputFile}

echo "input file path: ${inputFilePath} "
echo "output file path: ${outputFilePath} "

#run compression
${app} -oz $inputFilePath ${outputFilePath} ${dimInfo} 

#sleep 1
#run decompression
jyOutputFilePath=$projectRoot/tmp/jy_${inputFilePrefix}_${timestamp}${inputFileSuffix}
${app} -ou $outputFilePath ${jyOutputFilePath}

#sleep 1
diff ${inputFilePath} ${jyOutputFilePath}
if [ $? -eq 0 ]
then
    echo "Comparing result is correct "
else
    echo "Comparing result is wrong!!!"
fi

