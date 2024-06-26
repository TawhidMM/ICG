#!/bin/bash

yacc -d -y 2005036.y
# echo 'Generated the parser C file as well the header file'
g++ -w -c -o y.o y.tab.c
# echo 'Generated the parser object file'
flex 2005036.l
# echo 'Generated the scanner C file'
g++ -w -c -o l.o lex.yy.c
# if the above command doesn't work try g++ -fpermissive -w -c -o l.o lex.yy.c
# echo 'Generated the scanner object file'
g++ y.o l.o -lfl -o compiler
# echo 'All ready, running'
./compiler input.c


# delete the following files
DELETE_FILES=("y.tab.h" "y.tab.c" "y.o" "lex.yy.c" "l.o")

# Loop through each file and delete it
for FILE in "${DELETE_FILES[@]}"
do
    rm -f $FILE
done
