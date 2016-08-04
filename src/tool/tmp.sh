#!/bin/bash - 
#===============================================================================
#
#          FILE:  tmp.sh
# 
#         USAGE:  ./tmp.sh 
# 
#   DESCRIPTION:  
# 
#       OPTIONS:  ---
#  REQUIREMENTS:  ---
#          BUGS:  ---
#         NOTES:  ---
#        AUTHOR: YOUR NAME (), 
#       COMPANY: 
#       CREATED: 2016年04月05日 11时54分47秒 CST
#      REVISION:  ---
#===============================================================================

g++ -Wall -O2  -g -I(/home/hruan/project-makefile-dev/src/tool/include/,/home/hruan/project-makefile-dev/src/tool/../../src/include/) -L/home/hruan/project-makefile-dev/src/tool/lib /home/hruan/project-makefile-dev/src/tool/../../lib/ -o ../../bin/tool_cc tool.cc -lutil

