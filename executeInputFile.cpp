/* this code creates a copy of input.c file with  
definition to println() function */

#include<iostream>
#include<fstream>

using namespace std;
 
int main(){
    string INPUT_FILE = "input.c";
    string OUTPUT_FILE = "executableInput.c";
    string PRINTLN_FUNCTION = 
    "void println(int a){\nprintf(\"%d\\n\", a);\n}";

    ifstream inputFile(INPUT_FILE);
    if(!inputFile.is_open()){
        cout << "unable to open " << INPUT_FILE << endl;
    }

    ofstream outputFile(OUTPUT_FILE);

    outputFile << "#include<stdio.h>" << endl;
    outputFile << endl << endl;
    outputFile << PRINTLN_FUNCTION << endl;

    string line;
    while(getline(inputFile, line)){
        outputFile << line << endl;
    }

    return 0;
}