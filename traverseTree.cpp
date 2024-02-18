#include<iostream>
#include<fstream>
#include<stack>
#include"2005036_ParseTree.h"

using namespace std;


class RegisterStatus {
public:
    bool used = false;
    bool pushed = false;
};

class Label {
private:
    int count = 0;
public:
    string nextLabel(){
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


ofstream asm_out;
bool globalScope = true;
bool expValuePushed = false;
bool printFuncCalled = false;
bool arrayDeclared = false;
int localVarOffset = 0;
int funcParameters = 0;

stack<int> offsetStack;


RegisterStatus BX;
RegisterStatus CX;
Label label;


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

        offsetStack.push(localVarOffset);
        localVarOffset = 0;

        handleFuncDefinition(root);

        globalScope = true;
        localVarOffset = offsetStack.top();
        offsetStack.pop();
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
    
    if(nodeStr.find("func_definition : type_specifier ID")==0) {
        string funcName = root->getChild()->getSibling()->getSymbol()->getName();

        asm_out << funcName << " PROC" << endl;

        if(funcName == "main") {
            asm_out << 
                "\tMOV AX, @DATA\n" <<
                "\tMOV DS, AX" 
            << endl;
        }
        asm_out << 
            "\tPUSH BP\n" <<
            "\tMOV BP, SP"
        << endl;

        funcParameters = 0;
    }
    else if(nodeStr.find("parameter_list")==0) {
        ParseTree* id = root->getChild();
        
        while(id->getNode()!="ID"){
            id = id->getSibling();
        }
        cout << id->getSymbol()->getName() << endl;
        id->getSymbol()->offset = -(4 + funcParameters);
        id->getSymbol()->scope = "LOCAL";

        funcParameters += 2;
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
        root->next = label.nextLabel();
        ParseTree* expression = root->getChild()->getSibling()->getSibling();
        
        expression->trueLabel = label.nextLabel();       
        expression->falseLabel = root->next;

        ParseTree* temp = expression;

        while(temp){
            if(temp->getNode().find("ELSE")==0) {
                expression->falseLabel = label.nextLabel();
                break;
            }
            temp = temp->getSibling();
        }
    }
    else if(nodeStr=="statement : WHILE LPAREN expression RPAREN statement") {
        root->next = label.nextLabel();
        ParseTree* expression = root->getChild()->getSibling()->getSibling();
        
        expression->next = label.nextLabel();
        expression->trueLabel = label.nextLabel();       
        expression->falseLabel = root->next;
    }
    else if(nodeStr.find("statement : FOR LPAREN expression_statement")==0) {
        root->next = label.nextLabel();
        ParseTree* expression = root->getChild()->getSibling()->
                getSibling()->getSibling();
        
        expression->next = label.nextLabel();
        expression->trueLabel = label.nextLabel();       
        expression->falseLabel = root->next;

        expression->getSibling()->next = label.nextLabel();
    }
    else if(nodeStr=="expression : logic_expression"|| 
                nodeStr.find("expression_statement : expression")==0) {

        root->getChild()->next = root->next;
        root->getChild()->trueLabel = root->trueLabel;
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

            asm_out << expression->trueLabel << ":" << endl;
        }
        else if(nodeStr.find("statement : FOR LPAREN expression_statement")==0 &&
                child->getNode().find("RPAREN")==0) 
        {
            ParseTree* checkExpression = root->getChild()->getSibling()->
                getSibling()->getSibling();

            asm_out << "\tJMP " << checkExpression->next << endl;
            asm_out << checkExpression->trueLabel << ":" << endl;
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
    
    else if(nodeStr.find("func_definition : type_specifier ID")==0) {
        string funcName = root->getChild()->getSibling()->getSymbol()->getName();
        
        if(localVarOffset != 0){
            asm_out << "\tADD SP, " << localVarOffset << endl;
        }
        asm_out << "\tPOP BP" << endl;

        if(funcName == "main"){
            asm_out << 
                "\tMOV AX,4CH\n"
                "\tINT 21H" 
            << endl;
        }
        else {
            asm_out << "\tRET " << funcParameters << endl;
        }

        asm_out << funcName << " ENDP\n" << endl;
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
            if(variable->getSymbol()->offset >= 0) {
                asm_out << "\tMOV [BP-" << variable->getSymbol()->offset << "], AX" << endl; 
            }
            else {
                asm_out << "\tMOV [BP+" << (-1*variable->getSymbol()->offset) << "], AX" << endl; 
            }
            
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

            if(root->getSymbol()->offset >= 0) {
                asm_out << "\tMOV AX, [BP-" << root->getSymbol()->offset << "]" << endl;
            }
            else {
                asm_out << "\tMOV AX, [BP+" << (-1*root->getSymbol()->offset) << "]" << endl;
            }
           
        }    
    }
    else if(nodeStr=="logic_expression : rel_expression LOGICOP rel_expression") {
        ParseTree* logiop = root->getChild()->getSibling();

        if(logiop->getSymbol()->getName()=="||") {
            root->getChild()->trueLabel = root->trueLabel;
            root->getChild()->falseLabel = label.nextLabel();     
        }
        else if(logiop->getSymbol()->getName()=="&&"){
            root->getChild()->trueLabel = label.nextLabel();
            root->getChild()->falseLabel = root->falseLabel;  
        }

        logiop->getSibling()->trueLabel = root->trueLabel;
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
    else if(nodeStr=="factor : ID LPAREN argument_list RPAREN"){
        if(CX.used) {
            asm_out << "\tPUSH CX" << endl;
            CX.pushed = true;
        }
        if(BX.used) {
            asm_out << "\tPUSH BX" << endl;
            BX.pushed = true;
        }
        
    } 
    
    ParseTree* child = root->getChild();
   
    while(child != nullptr) {
        if(nodeStr!="logic_expression : rel_expression LOGICOP rel_expression") {
            child->trueLabel = root->trueLabel;
            child->falseLabel = root->falseLabel;
        }
        if(child->getNode()=="LOGICOP"){
            if(child->getSymbol()->getName()=="||") {
                asm_out << root->getChild()->falseLabel << ":" << endl;
            }
            else if(child->getSymbol()->getName()=="&&"){
                asm_out << root->getChild()->trueLabel << ":" << endl;
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
        else {
            BX.used = false;     
        }
          
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
        else{
            CX.used = false; 
        } 
        
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

        if(root->trueLabel == ""){
            return;
        }

        asm_out << "\tCMP BX, AX" << endl;

        if(relop->getSymbol()->getName()=="=="){
            asm_out << "\tJE " << root->trueLabel << endl;
        }
        else if(relop->getSymbol()->getName()=="!="){
            asm_out << "\tJNE " << root->trueLabel << endl;
        }
        else if(relop->getSymbol()->getName()==">"){
            asm_out << "\tJG " << root->trueLabel << endl;
        }
        else if(relop->getSymbol()->getName()==">="){
            asm_out << "\tJGE " << root->trueLabel << endl;
        }
        else if(relop->getSymbol()->getName()=="<"){
            asm_out << "\tJL " << root->trueLabel << endl;
        }
        else if(relop->getSymbol()->getName()=="<="){
            asm_out << "\tJLE " << root->trueLabel << endl;
        }

        asm_out << "\tJMP " << root->falseLabel << endl;
        if(BX.pushed){
            asm_out << "\tPOP BX" << endl;
            BX.pushed = false;
        } else {
            BX.used = false; 
        }
        
    }
    else if(nodeStr=="factor : ID LPAREN argument_list RPAREN"){
        asm_out << 
            "\tCALL " << root->getChild()->getSymbol()->getName() 
        << endl;

        if(BX.pushed){
            asm_out << "\tPOP BX" << endl;
            BX.pushed = false;
        }   
        if(CX.pushed){
            asm_out << "\tPOP CX" << endl;
            CX.pushed = false;
        } 
    }
    else if(nodeStr.find("arguments")==0){
        asm_out << "\tPUSH AX" << endl;
    }
}


void handlePrint(ParseTree* root) {
    ParseTree* child = root->getChild();

    while(child->getNode() != "ID") {
        child = child->getSibling();
    }

    if(child->getSymbol()->scope=="LOCAL") {
        if(child->getSymbol()->offset>=0){
            asm_out << "\tMOV AX, [BP-" << child->getSymbol()->offset << "]" << endl;
        }
        else{
            asm_out << "\tMOV AX, [BP+" << (-1*child->getSymbol()->offset) << "]" << endl;
        }
        
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






