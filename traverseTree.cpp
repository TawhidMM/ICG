#include<iostream>
#include<fstream>
#include"2005036_ParseTree.h"

using namespace std;


class RegisterStatus {
public:
    bool used = false;
    bool pushed = false;
};

class Lebel {
private:
    int count = 0;
public:
    string nextLebel(){
        count++;
        return ("L" + to_string(count));
    }
};


void traverse(ParseTree* root);
void handleVarDeclaration(ParseTree* root);
void handleFuncDefinition(ParseTree* root);
void handleExpression(ParseTree* root);
void handlePrint(ParseTree* root);
void addPrintFunction();
void handleAssignment(ParseTree* root);
void assign(ParseTree* variable);


/* 

******** problems ***********
1. sub 
2. lebel after expression


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
Lebel lebel;


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

    cout << "code generation complete" << endl;
}



void traverse(ParseTree* root) {
    string node = root->getNode();

    if(node.find("var_declaration") == 0) {
        handleVarDeclaration(root);

        /* if global var declaration, start the code segment */
        if(globalScope)
            asm_out << ".CODE" << endl;
        return;
    }
    else if(node.find("func_definition") == 0) {
        globalScope = false;
        
        handleFuncDefinition(root);
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

void handleVarDeclaration(ParseTree* root) {
    string nodeStr = root->getNode();

    if(nodeStr.find("ID") == 0) {
        if(globalScope) {
            root->getSymbol()->scope = "GLOBAL";
            asm_out << "\t" << root->getSymbol()->getName() << " DW ";
        }
        else {
            root->getSymbol()->scope = "LOCAL";

            if(root->getSymbol()->getType()=="VARIABLE"){
                asm_out << "\tSUB SP, 2" << endl;
                localVarOffset += 2;
                root->getSymbol()->offset = (localVarOffset); 
            }
        }
    }
    else if (nodeStr.find("CONST_INT") == 0) {
        if(globalScope) {
            asm_out << root->getSymbol()->getName() << " DUP (0000H)" << endl;
            arrayDeclared = true;
        } 
        else {
            localVarOffset += stoi(root->getSymbol()->getName())*2;
            asm_out << "\tSUB SP, " << stoi(root->getSymbol()->getName())*2 << endl;
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
        handleVarDeclaration(child);
        child = child->getSibling();
    }

    /* set offset for local array */
    if(nodeStr.find("ID LSQUARE CONST_INT RSQUARE")!=string::npos) {
        ParseTree* id = root->getChild();

        while(id->getNode()!="ID"){
            id = id->getSibling();
        }
        

        if(id->getSymbol()->scope=="LOCAL"){
            id->getSymbol()->offset = localVarOffset;
        }
    }

}



void handleFuncDefinition(ParseTree* root) {
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
        handleAssignment(root);
        return;
    }
    else if(nodeStr.find("logic_expression") == 0) {
        if(root->next != ""){
            asm_out << root->next << ":" << endl;
        }

        handleExpression(root);

        return;
    }
    else if(nodeStr.find("statement : IF LPAREN expression RPAREN statement") == 0) {
        root->next = lebel.nextLebel();
        ParseTree* expression = root->getChild()->getSibling()->getSibling();
        
        expression->trueLebel = lebel.nextLebel();       
        expression->falseLabel = root->next;

        ParseTree* temp = expression;

        while(temp){
            if(temp->getNode().find("ELSE")==0) {
                expression->falseLabel = lebel.nextLebel();
                break;
            }
            temp = temp->getSibling();
        }
    }
    else if(nodeStr=="statement : WHILE LPAREN expression RPAREN statement") {
        root->next = lebel.nextLebel();
        ParseTree* expression = root->getChild()->getSibling()->getSibling();
        
        expression->next = lebel.nextLebel();
        expression->trueLebel = lebel.nextLebel();       
        expression->falseLabel = root->next;
    }
    else if(nodeStr.find("statement : FOR LPAREN expression_statement")==0) {
        root->next = lebel.nextLebel();
        ParseTree* expression = root->getChild()->getSibling()->
                getSibling()->getSibling();
        
        expression->next = lebel.nextLebel();
        expression->trueLebel = lebel.nextLebel();       
        expression->falseLabel = root->next;

        expression->getSibling()->next = lebel.nextLebel();
    }
    else if(nodeStr=="expression : logic_expression"|| 
                nodeStr.find("expression_statement : expression")==0) {

        root->getChild()->next = root->next;
        root->getChild()->trueLebel = root->trueLebel;
        root->getChild()->falseLabel = root->falseLabel;    
    }
    else if(nodeStr.find("PRINTLN") != string::npos) {
        printFuncCalled = true;
        handlePrint(root);
        return;
        
    }
    else if(nodeStr.find("var_declaration") == 0) {
        handleVarDeclaration(root);
        return;
    }
   
    ParseTree* child = root->getChild();

    while(child != nullptr){

        if(child->getNode().find("ELSE")==0){
            ParseTree* expression = root->getChild()->getSibling()->getSibling();

            asm_out << "\tJMP " << root->next << endl;
            asm_out << expression->falseLabel << ":" << endl;
        }
        else if((nodeStr.find("statement : IF LPAREN expression RPAREN")==0 ||
                    nodeStr.find("statement : WHILE LPAREN expression")==0) &&
                    child->getNode().find("RPAREN")==0) 
        {
            ParseTree* expression = root->getChild()->getSibling()->getSibling();

            asm_out << expression->trueLebel << ":" << endl;
        }
        else if(nodeStr.find("statement : FOR LPAREN expression_statement")==0 &&
                child->getNode().find("RPAREN")==0) 
        {
            ParseTree* checkExpression = root->getChild()->getSibling()->
                getSibling()->getSibling();

            asm_out << "\tJMP " << checkExpression->next << endl;
            asm_out << checkExpression->trueLebel << ":" << endl;
        }

        handleFuncDefinition(child);
        child = child->getSibling();

        
    }

    if(nodeStr.find("statement : IF LPAREN expression RPAREN statement")==0) {
        asm_out << root->next << ":" << endl;
    }
    else if(nodeStr.find("statement : FOR LPAREN expression_statement")==0 ||
                nodeStr.find("statement : WHILE LPAREN expression RPAREN statement")==0) 
    {
        ParseTree* expression = root->getChild();
        while(expression->getNode().find("expression :")!=0){
            expression = expression->getSibling();
        }

        asm_out << "\tJMP " << expression->next << endl;
        asm_out << root->next << ":" << endl;
    }
    else if(nodeStr=="func_definition : type_specifier ID LPAREN RPAREN compound_statement") {
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
        asm_out << 
            root->getChild()->getSibling()->getSymbol()->getName() << 
            " ENDP" << 
        endl;
    }

}

void handleAssignment(ParseTree* root){
    ParseTree* child = root->getChild();

    while(child->getSibling()) {
        child = child->getSibling();
    }

    handleExpression(child);
   
    child = root->getChild();

    assign(child);

}

void assign(ParseTree* variable) {
    
    if(variable->getNode()=="variable : ID"){
        variable = variable->getChild();

        if(variable->getSymbol()->scope=="GLOBAL") {
            asm_out << "\tMOV " << 
                variable->getSymbol()->getName() << 
                ", AX" 
            << endl;
        }
        else if(variable->getSymbol()->scope=="LOCAL") {
            asm_out << "\tMOV [BP-" << variable->getSymbol()->offset << "], AX" << endl; 
        }

    }
    else if(variable->getNode()=="variable : ID LSQUARE expression RSQUARE") {
        /* right side exp value pushed */
        asm_out << "\tPUSH AX" << endl;

        variable = variable->getChild();
        handleExpression(variable->getSibling()->getSibling());

        if(variable->getSymbol()->scope=="GLOBAL"){
            /* array index at AX, pop the right side value at CX */
            asm_out << 
                "\tPOP CX\n" <<
                "\tMOV DX, 2\n" <<
                "\tMUL DX\n" <<
                "\tMOV BX, AX\n" <<
                "\tMOV " << variable->getSymbol()->getName() << "[BX], CX" << endl;
        }
        else if(variable->getSymbol()->scope=="LOCAL"){
            /* array index at AX, pop the right side value at CX */
            asm_out << 
                "\tPOP CX\n" <<
                "\tMOV DX, 2\n" <<
                "\tMUL DX\n" <<
                "\tMOV BX, AX\n" <<
                "\tMOV AX, " << variable->getSymbol()->offset << "\n"
                "\tSUB AX, BX\n" <<
                "\tMOV SI, AX\n" <<
                "\tNEG SI\n" <<
                "\tMOV [BP+SI], CX" << endl;
        } 
    }

}


void handleExpression(ParseTree* root) {
    string nodeStr = root->getNode();
    
    if(nodeStr.find("CONST_INT")==0){
        asm_out << "\tMOV AX," << root->getSymbol()->getName() << endl;

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
    }
    else if(nodeStr=="logic_expression : rel_expression LOGICOP rel_expression") {
        ParseTree* logiop = root->getChild()->getSibling();

        if(logiop->getSymbol()->getName()=="||") {
            root->getChild()->trueLebel = root->trueLebel;
            root->getChild()->falseLabel = lebel.nextLebel();     
        }
        else if(logiop->getSymbol()->getName()=="&&"){
            root->getChild()->trueLebel = lebel.nextLebel();
            root->getChild()->falseLabel = root->falseLabel;  
        }

        logiop->getSibling()->trueLebel = root->trueLebel;
        logiop->getSibling()->falseLabel = root->falseLabel;
    }
    else if(nodeStr.find("ADDOP")==0||nodeStr.find("RELOP")==0){
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
   
    while(child != nullptr) {
        if(nodeStr!="logic_expression : rel_expression LOGICOP rel_expression") {
            child->trueLebel = root->trueLebel;
            child->falseLabel = root->falseLabel;
        }
        if(child->getNode()=="LOGICOP"){
            if(child->getSymbol()->getName()=="||") {
                asm_out << root->getChild()->falseLabel << ":" << endl;
            }
            else if(child->getSymbol()->getName()=="&&"){
                asm_out << root->getChild()->trueLebel << ":" << endl;
            }
        }

        handleExpression(child);
        child = child->getSibling();
    }

    if(nodeStr == "simple_expression : simple_expression ADDOP term"){
        string operation = root->getChild()->getSibling()->getSymbol()->getName();
       
        if(operation == "+")
            asm_out << "\tADD AX, BX" << endl;            
        else if(operation == "-"){
            asm_out << "\tXCHG BX, AX" << endl; 
            asm_out << "\tSUB AX, BX" << endl; 
        }
        if(BX.pushed){
            asm_out << "\tPOP BX" << endl;
            BX.pushed = false;
        }   
        BX.used = false;       
    }
    else if(nodeStr == "variable : ID LSQUARE expression RSQUARE") {
        /* index already in AX */
        if(root->getChild()->getSymbol()->scope=="GLOBAL"){
            asm_out << 
                "\tMOV DX, 2\n" <<
                "\tMUL DX\n" <<
                "\tMOV SI, AX\n" <<
                "\tMOV AX, " << root->getChild()->getSymbol()->getName() << "[SI]" 
            << endl;
        }
        else if(root->getChild()->getSymbol()->scope=="LOCAL") {
            asm_out << 
                "\tMOV DX, 2\n" <<
                "\tMUL DX\n" <<
                "\tMOV DX, AX\n" <<
                "\tMOV AX, " << root->getChild()->getSymbol()->offset << "\n"
                "\tSUB AX, DX\n" <<
                "\tMOV SI, AX\n" <<
                "\tNEG SI\n" <<
                "\tMOV AX, [BP+SI]" 
            << endl;
        }
        
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
    else if(nodeStr=="factor : variable INCOP") {
        /* value already at AX */
        asm_out << "\tPUSH AX" << endl;
        asm_out << "\tINC AX" << endl;
        assign(root->getChild());
        asm_out << "\tPOP AX" << endl;
    }
    else if(nodeStr=="factor : variable DECOP") {
        /* value already at AX */
        asm_out << "\tPUSH AX" << endl;
        asm_out << "\tDEC AX" << endl;
        assign(root->getChild());
        asm_out << "\tPOP AX" << endl;
    }
    else if(nodeStr=="unary_expression : ADDOP unary_expression") {
        /* value already at AX */
        asm_out << "\tNEG AX" << endl;
    }
    else if(nodeStr.find("simple_expression RELOP simple_expression")!=string::npos) {
        ParseTree* relop = root->getChild()->getSibling();

        if(root->trueLebel == ""){
            return;
        }

        asm_out << "\tCMP BX, AX" << endl;

        if(relop->getSymbol()->getName()=="=="){
            asm_out << "\tJE " << root->trueLebel << endl;
        }
        else if(relop->getSymbol()->getName()=="!="){
            asm_out << "\tJNE " << root->trueLebel << endl;
        }
        else if(relop->getSymbol()->getName()==">"){
            asm_out << "\tJG " << root->trueLebel << endl;
        }
        else if(relop->getSymbol()->getName()==">="){
            asm_out << "\tJGE " << root->trueLebel << endl;
        }
        else if(relop->getSymbol()->getName()=="<"){
            asm_out << "\tJL " << root->trueLebel << endl;
        }
        else if(relop->getSymbol()->getName()=="<="){
            asm_out << "\tJLE " << root->trueLebel << endl;
        }

        asm_out << "\tJMP " << root->falseLabel << endl;
        if(BX.pushed){
            asm_out << "\tPOP BX" << endl;
            BX.pushed = false;
        }   
        BX.used = false; 
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
        cout << "Error opening print function file." << endl;
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






