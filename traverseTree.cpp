#include<iostream>
#include<fstream>
#include"2005036_ParseTree.h"

using namespace std;


void traverse(ParseTree* root);
ParseTree* handleVarDeclaration(ParseTree* root);
void declareGlobalVar(string varName);
ParseTree* handleFuncDefinition(ParseTree* root);
ParseTree* handleExpression(ParseTree* root, string& operation);


ofstream asm_out;
bool globalScope = true;
string str;



/* int main(){
    cout << INIT_DATA_SEGMENT << endl;
    cout << GLOBAL_VAR(x);
    
} */



void traverseTree(ParseTree* tree, string FILE) {
    asm_out.open(FILE);

    if (!asm_out.is_open()) {
        cout << "asm file is not open." << std::endl;
        return;
    }

    asm_out << ".MODEL SMALL" << endl;
    asm_out << ".STACK 1000H" << endl;
    asm_out << ".Data" << endl;

    traverse(tree);

    asm_out << "END main" << endl;
}



void traverse(ParseTree* root) {
    string node = root->getNode();

    if(node.find("var_declaration") == 0) {
        root = handleVarDeclaration(root);

        /* if global var declaration, start the code segment */
        if(globalScope)
            asm_out << ".CODE" << endl;
    }
    else if(node.find("func_definition") == 0) {
        globalScope = false;
        root = handleFuncDefinition(root);
        globalScope = true;
    }
    
    ParseTree* child = root->getChild();

    while(child != nullptr){
        traverse(child);
        child = child->getSibling();
    }

}

ParseTree* handleVarDeclaration(ParseTree* root) {
    string nodeStr = root->getNode();

    if(nodeStr.find("ID") == 0) {
        if(globalScope)
            declareGlobalVar(root->getSymbol()->getName());
    }

    ParseTree* child = root->getChild();

    while(child != nullptr){
        root = handleVarDeclaration(child);
        child = child->getSibling();
    }

    return root;
}

void declareGlobalVar(string varName) {
    asm_out << "\t" << varName  << " DW 1 DUP (0000H)" << endl;
}


ParseTree* handleFuncDefinition(ParseTree* root) {
    string nodeStr = root->getNode();

    if(nodeStr.find("ID") == 0) {
        str = root->getSymbol()->getName();

        asm_out << str << " PROC" << endl;

        if(str == "main") {
            asm_out << 
                "\tMOV AX, @DATA\n" <<
                "\tMOV DS, AX" 
            << endl;
        }
    }
    else if(nodeStr.find("expression") == 0) {
        string operation;
        root = handleExpression(root, operation);
        
    }
    else if(nodeStr == "RCURL : }"){
        if(str == "main"){
            asm_out << 
                "\tMOV AX,4CH\n"
                "\tINT 21H" 
            << endl;
        }
        asm_out << str << " ENDP" << endl;
    }

    ParseTree* child = root->getChild();

    while(child != nullptr){
        root = handleFuncDefinition(child);
        child = child->getSibling();
    }

    return root;
}


ParseTree* handleExpression(ParseTree* root, string& operation) {
    string nodeStr = root->getNode();
    
    if(nodeStr.find("CONST_INT")==0){
        asm_out << "\tMOV AX," << root->getSymbol()->getName() << endl;
    }
    else if(nodeStr.find("ADDOP")==0){
        operation = root->getSymbol()->getName();
        asm_out << "\tMOV DX, AX" << endl;
    }
    /* else if(nodeStr.find("MULOP")==0){
        operation = root->getSymbol()->getName();
        asm_out << "\tMOV CX, AX" << endl;
    } */

    ParseTree* child = root->getChild();

    while(child != nullptr){
        root = handleExpression(child, operation);
        child = child->getSibling();
    }

    if(nodeStr == "simple_expression : simple_expression ADDOP term"){
        if(operation == "+")
            asm_out << "\tADD AX, DX" << endl;
        else if(operation == "-")
            asm_out << "\tSUB AX, DX" << endl;    
    }
    /* else if(nodeStr == "term : term MULOP unary_expression") {
        if(operation == "*"){
            asm_out << "\tMUL CX" << endl;
        }
    } */

    return root;
}


