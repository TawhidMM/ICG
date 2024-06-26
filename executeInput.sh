g++ executeInputFile.cpp
./a.out

g++ executableInput.c -o executableInput
./executableInput

# delete the following files
DELETE_FILES=("a.out" "executableInput.c" "executableInput")

# Loop through each file and delete it
for FILE in "${DELETE_FILES[@]}"
do
    rm -f $FILE
done