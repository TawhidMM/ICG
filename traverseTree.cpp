#include<iostream>
#include<fstream>
#include"2005036_ParseTree.h"

using namespace std;


class RegisterStatus {
public:
    bool used = false;
    bool pushed = false;
};


void traverse(ParseTree* root);
ParseTree* handleVarDeclaration(ParseTree* root);
void declareGlobalVar(string varName);
void declareLocalVar();
/* ParseTree* */void handleFuncDefinition(ParseTree* root);
/* ParseTree* */void handleExpression(ParseTree* root);
void handlePrint(ParseTree* root);
void addPrintFunction();
void handleAssignment(ParseTree* root);


/* 

******** problems ***********
1. sub 


 */

ofstream asm_out;
bool globalScope = true;
bool expValuePushed = false;
bool printFuncCalled = false;
bool arrayDeclared = false;
int localVarOffset = 0;
string str;

RegisterStatus BX;
RegisterStatus CX;


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
    asm_out << "\tnumber DB \"00000$\"" << endl;

    traverse(tree);

    if(printFuncCalled) addPrintFunction();

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
        /* root = */ handleFuncDefinition(root);
        globalScope = true;
        localVarOffset = 0;
        return;
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
        if(globalScope) {
            root->getSymbol()->scope = "GLOBAL";
            asm_out << "\t" << root->getSymbol()->getName() << " DW ";
        }
        else {
            root->getSymbol()->scope = "LOCAL";
            
            asm_out << "\tSUB SP, 2" << endl;
            localVarOffset += 2;
            root->getSymbol()->offset = (localVarOffset); 
        }
    }
    else if (nodeStr.find("CONST_INT") == 0) {
        if(globalScope) {
            asm_out << root->getSymbol()->getName() << " DUP (0000H)" << endl;
            arrayDeclared = true;
        } 
    }
    else if(nodeStr.find("COMMA")==0 || nodeStr.find("SEMICOLON")==0) {
        if(globalScope && !arrayDeclared) {
            asm_out << "1 DUP (0000H)" << endl;
        } 
        arrayDeclared = false;
    }

    ParseTree* child = root->getChild();

    while(child != nullptr){
        root = handleVarDeclaration(child);
        child = child->getSibling();
    }

    return root;
}



/* ParseTree* */ void handleFuncDefinition(ParseTree* root) {
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
        asm_out << 
            "\tPUSH BP\n" <<
            "\tMOV BP, SP"
        << endl;
    }
    else if(nodeStr.find("expression : variable ASSIGNOP logic_expression") == 0) {
        /* root = */ handleAssignment(root);
        return;
        
    }
    else if(nodeStr.find("PRINTLN") != string::npos) {
        printFuncCalled = true;
        handlePrint(root);
        return;
        
    }
    else if(nodeStr.find("var_declaration") == 0) {
        root = handleVarDeclaration(root);
    }
    else if(nodeStr == "RCURL : }") {
        if(localVarOffset != 0){
            asm_out << "\tADD SP, " << localVarOffset << endl;
        }
        asm_out << "\tPOP BP" << endl;

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
        /* root = */ handleFuncDefinition(child);
        child = child->getSibling();
    }

    /* return root; */
}

void handleAssignment(ParseTree* root){
    ParseTree* child = root->getChild();

    while(child->getSibling()) {
        child = child->getSibling();
    }

    handleExpression(child);
   
    child = root->getChild();
    
    if(child->getNode()=="variable : ID"){
        child = child->getChild();

        if(child->getSymbol()->scope=="GLOBAL") {
            asm_out << "\tMOV " << 
                child->getSymbol()->getName() << 
                ", AX" 
            << endl;
        }
        else if(child->getSymbol()->scope=="LOCAL") {
            asm_out << "\tMOV [BP-" << child->getSymbol()->offset << "], AX" << endl; 
        }

    }
    else if(child->getNode()=="variable : ID LSQUARE expression RSQUARE") {
        /* right side exp value pushed */
        asm_out << "\tPUSH AX" << endl;

        child = child->getChild();
        handleExpression(child->getSibling()->getSibling());

        /* array index at AX, pop the right side value at CX */
        asm_out << 
            "\tPOP CX\n" <<
            "\tMOV DX, 2\n" <<
            "\tMUL DX\n" <<
            "\tMOV BX, AX\n" <<
            "\tMOV " << child->getSymbol()->getName() << "[BX], CX" << endl;
    }
    
}


/* ParseTree* */void handleExpression(ParseTree* root) {
    string nodeStr = root->getNode();
    
    if(nodeStr.find("CONST_INT")==0){
        asm_out << "\tMOV AX," << root->getSymbol()->getName() << endl;

        cout << root->getSymbol()->getName() << " int found " << endl;
    }
   
    else if(nodeStr.find("ID")==0){
        string scope = root->getSymbol()->scope;
        string type = root->getSymbol()->getType();

        if(scope=="GLOBAL" && type=="VARIABLE") {
            asm_out << "\tMOV AX," << root->getSymbol()->getName() << endl;
        }
        else if(scope=="LOCAL" && type=="VARIABLE") {
            asm_out << "\tMOV AX, [BP-" << root->getSymbol()->offset << "]" << endl;
        }

        cout << root->getSymbol()->getName() << " id found " << endl;
            
    }
    else if(nodeStr.find("ADDOP")==0){
        if(BX.used){
            asm_out << "\tPUSH BX" << endl;
            BX.pushed = true;
        }
        asm_out << "\tMOV BX, AX" << endl;
        BX.used = true;
    }
    else if(nodeStr.find("MULOP")==0){
        if(CX.used) {
            asm_out << "\tPUSH CX" << endl;
            CX.pushed = true;
        }
        asm_out << "\tMOV CX, AX" << endl;
        CX.used = true;
    } 
    
    ParseTree* child = root->getChild();
   
    while(child != nullptr){
        /* root = */ handleExpression(child);
        child = child->getSibling();
    }

    if(nodeStr == "simple_expression : simple_expression ADDOP term"){
        string operation = root->getChild()->getSibling()->getSymbol()->getName();
       
        if(operation == "+")
            asm_out << "\tADD AX, BX" << endl;            
        else if(operation == "-")
            asm_out << "\tSUB AX, BX" << endl; 

        if(BX.pushed){
            asm_out << "\tPOP BX" << endl;
            BX.pushed = false;
        }   
        BX.used = false;       
    }
    else if(nodeStr == "variable : ID LSQUARE expression RSQUARE") {
        asm_out << 
            "\tMOV DX, 2\n" <<
            "\tMUL DX\n" <<
            "\tMOV BX, AX\n" <<
            "\tMOV AX, " << root->getChild()->getSymbol()->getName() << "[BX]" 
        << endl;
    }
    else if(nodeStr == "term : term MULOP unary_expression") {
        string operation = root->getChild()->getSibling()->getSymbol()->getName();

        if(operation == "*"){
            asm_out << "\tMUL CX" << endl;
        }
        else if(operation == "/" || operation == "%"){
            asm_out << "\t;SWAP AX, CX FOR DIV" << endl;
            asm_out << "\tXCHG AX, CX" << endl;
            
            asm_out << "\t;CLEAR DX FOR DIV" << endl;
            asm_out << "\tXOR DX, DX" << endl;

            asm_out << "\tDIV CX" << endl;

            if(operation == "%")
                asm_out << "\tMOV AX, DX" << endl;
        }
        
        if(CX.pushed){
            asm_out << "\tPOP CX" << endl;
            CX.pushed = false;
        }   
        CX.used = false; 
    }
}


void handlePrint(ParseTree* root) {
    ParseTree* child = root->getChild();

    while(child->getNode() != "ID") {
        child = child->getSibling();
    }

    if(child->getSymbol()->scope=="LOCAL") {
        asm_out << "\tMOV AX, [BP-" << child->getSymbol()->offset << "]" << endl;
    }
    else if(child->getSymbol()->scope=="GLOBAL") {
        asm_out << "\tMOV AX, " << child->getSymbol()->getName() << endl;
    }

    asm_out << "\tCALL print_output" << endl;
	asm_out << "\tCALL new_line" << endl;
    

}


void addPrintFunction() {
    string FILE_NAME = "printProc.lib";

    ifstream printFile(FILE_NAME);

    // Check if the source file is open
    if (!printFile.is_open()) {
        std::cerr << "Error opening print function file." << std::endl;
    }


    asm_out << "\n\n;-------------------------------" << endl;
    asm_out << ";           print library       " << endl;      
    asm_out << ";-------------------------------" << endl;

    string line;
    while (getline(printFile, line)) {
        asm_out << line << std::endl;
    }

    asm_out << ";-------------------------------" << endl;

    printFile.close();
}






