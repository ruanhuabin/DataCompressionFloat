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
app=$projectRoot/script/mrc_tar_c
timestamp=`date +%Y-%m-%d:%H:%M:%S`
bitsToMask=0
if [ "$#" -eq 1 ]
then
    bitsToMask=$1
fi

echo "command line arguments number = $#"
inputFileName="stack.mrc"
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

zipCMDLineArgs=" -i $inputFilePath -o ${outputFilePath} -b ${bitsToMask} -t zip"
echo "Compress Command: ${app} ${zipCMDLineArgs}"
#run compression
${app} ${zipCMDLineArgs}
#${app} -oz $inputFilePath ${outputFilePath} -cp ${bitsToMask} 


