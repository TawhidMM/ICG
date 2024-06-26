%{
#include <iostream>
#include <fstream>
#include "2005036_SymbolTable.h"
#include "2005036_SymbolInfo.h"
#include "2005036_FunctionParams.h"
#include "2005036_ParseTree.h"
#include "2005036_traverseTree.cpp"

using namespace std;


int yyparse(void);
int yylex(void);

extern int yylineno;
extern FILE *yyin;
FILE *fp;
ofstream log_out;
ofstream tree_out;
ofstream error_out;
streambuf* coutBuffer;


int BUCKET_NUM = 11;
SymbolTable *table = new SymbolTable(BUCKET_NUM);

SymbolInfo* sinfo = nullptr;
FunctionParams* funcParams = nullptr;

string dataType;
int constantNum;
bool expOperation = false;
int count = 0;
int totalLine;
int totalError = 0;


string operationDataType(ParseTree* oparand1, ParseTree* oparand2);
bool voidVariable(SymbolInfo* variable);
void insertVariable(SymbolInfo* symbolInfo, string type);
void insertFunction(SymbolInfo* symbolInfo, string returnType, int line);
bool checkFuncDefination(SymbolInfo* func, string returnType);
void checkFunctionCall(string calledType);
bool matchType(string t1, string t2);
void resetFuncCall();
bool devidedByZero(SymbolInfo* operand);

void yyerror(string s, int line = yylineno)
{
	error_out << "Line# " << line << ": " << s << endl;
	totalError++;
}

bool declared(SymbolInfo* symbolInfo){
	return table->lookup(symbolInfo->getName());
}

%}

%locations

%union {
    SymbolInfo* symbolInfo;
    ParseTree* parseTree;
}



%type <parseTree> start program unit var_declaration func_declaration func_definition
 		type_specifier parameter_list compound_statement statements declaration_list
 		statement expression_statement argument_list arguments 
		variable expression logic_expression rel_expression 
		simple_expression term unary_expression factor

%token <symbolInfo> CONST_INT CONST_FLOAT BITOP ID

%token	IF ELSE FOR WHILE DO BREAK INT CHAR FLOAT DOUBLE VOID RETURN 
		SWITCH CASE DEFAULT CONTINUE CONST_CHAR 
		PRINTLN  LPAREN RPAREN LCURL RCURL LTHIRD RTHIRD COMMA SEMICOLON 
		MULTI_LINE_STRING SINGLE_LINE_STRING

%left <symbolInfo> LOGICOP RELOP ADDOP MULOP
%right  ASSIGNOP INCOP DECOP NOT

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE


%%

start : 
		program
		{
			$$ = new ParseTree("start : program", @$.first_line, @$.last_line);
			$$->addLeftChild($1);

			$$->print(tree_out);

			cout << "start : program" << endl;
			
			cout << "Total Lines: " << yylineno << endl;
			cout << "Total Errors: " << totalError << endl;

			/* start code generation */
			cout.rdbuf(coutBuffer);

			cout << "parsing complete" << endl;
			traverseTree($$, "2005036_code.asm");
		}
		;

program : program unit
		{
			$1->addSibling($2);
			
			$$ = new ParseTree("program : program unit", @$.first_line, @$.last_line);
			$$->addLeftChild($1);

			cout << "program : program unit" << endl;
		}

		| unit
		{
			$$ = new ParseTree("program : unit", @$.first_line, @$.last_line);
			$$->addLeftChild($1);

			cout << "program : unit" << endl;
		}
		;
	
unit : 
		var_declaration
		{
			$$ = new ParseTree("unit : var_declaration", @$.first_line, @$.last_line);
			$$->addLeftChild($1);
			
			cout << "unit : var_declaration" << endl;
		}

		| func_declaration
		{
			$$ = new ParseTree("unit : func_declaration", @$.first_line, @$.last_line);
			$$->addLeftChild($1);
			
			cout << "unit : func_declaration" << endl;
			funcParams = nullptr;
		}

		| func_definition
		{
			$$ = new ParseTree("unit : func_definition", @$.first_line, @$.last_line);
			$$->addLeftChild($1);
			
			cout << "unit : func_definition" << endl;
			funcParams = nullptr;
		}
		;
     
func_declaration : 
		type_specifier ID LPAREN parameter_list RPAREN SEMICOLON
		{
			ParseTree* id = new ParseTree("ID", $2, @2.last_line, 0);
			ParseTree* lparen = new ParseTree("LPAREN : (" , @3.last_line, 0);
			ParseTree* rparen = new ParseTree("RPAREN : )" , @5.last_line, 0);
			ParseTree* semicolon = new ParseTree("SEMICOLON : ;" , @6.last_line, 0);

			$1->addSibling(id);
			id->addSibling(lparen);
			lparen->addSibling($4);
			$4->addSibling(rparen);
			rparen->addSibling(semicolon);

			$$ = new ParseTree("func_declaration : type_specifier ID LPAREN parameter_list RPAREN SEMICOLON", 
									@$.first_line, @$.last_line);
			
			$$->addLeftChild($1);


			insertFunction($2, $1->getDataType(), @2.last_line);

			cout << "func_declaration : type_specifier ID LPAREN " << 
				"parameter_list RPAREN SEMICOLON" << endl;
		}

		| type_specifier ID LPAREN RPAREN SEMICOLON
		{
			ParseTree* id = new ParseTree("ID", $2, @2.last_line, 0);
			ParseTree* lparen = new ParseTree("LPAREN : (" , @3.last_line, 0);
			ParseTree* rparen = new ParseTree("RPAREN : )" , @4.last_line, 0);
			ParseTree* semicolon = new ParseTree("SEMICOLON : ;" , @5.last_line, 0);

			$1->addSibling(id);
			id->addSibling(lparen);
			lparen->addSibling(rparen);
			rparen->addSibling(semicolon);

			$$ = new ParseTree("func_declaration : type_specifier ID LPAREN RPAREN SEMICOLON", 
									@$.first_line, @$.last_line);
			$$->addLeftChild($1);
			
			insertFunction($2, $1->getDataType(), @2.last_line);
			
			cout << "func_declaration : type_specifier ID LPAREN RPAREN SEMICOLON" << endl;
		}
		;
		 
func_definition : 
		type_specifier ID LPAREN parameter_list RPAREN compound_statement
		{
			if(checkFuncDefination($2, $1->getDataType())) {
				insertFunction($2, $1->getDataType(), @2.last_line);
			}
			else{
				yyerror("Conflicting types for '" + $2->getName() + "'", @2.last_line);
			}
			
			ParseTree* id = new ParseTree("ID", $2, @2.last_line, 0);
			ParseTree* lparen = new ParseTree("LPAREN : (" , @3.last_line, 0);
			ParseTree* rparen = new ParseTree("RPAREN : )" , @5.last_line, 0);
			
			$1->addSibling(id);
			id->addSibling(lparen);
			lparen->addSibling($4);
			$4->addSibling(rparen);
			rparen->addSibling($6);

			$$ = new ParseTree("func_definition : type_specifier ID LPAREN parameter_list RPAREN compound_statement", 
									@$.first_line, @$.last_line);
			$$->addLeftChild($1);
			
			cout << "func_definition : type_specifier ID LPAREN parameter_list " << 
				"RPAREN compound_statement" << endl;

		}

		| type_specifier ID LPAREN RPAREN compound_statement
		{
			if(checkFuncDefination($2, $1->getDataType())) {
				insertFunction($2, $1->getDataType(), @2.last_line);
			}
			else{
				yyerror("Conflicting types for '" + $2->getName() + "'", @2.last_line);
			}

			ParseTree* id = new ParseTree("ID", $2, @2.last_line, 0);
			ParseTree* lparen = new ParseTree("LPAREN : (" , @3.last_line, 0);
			ParseTree* rparen = new ParseTree("RPAREN : )" , @4.last_line, 0);
			
			$1->addSibling(id);
			id->addSibling(lparen);
			lparen->addSibling(rparen);
			rparen->addSibling($5);

			$$ = new ParseTree("func_definition : type_specifier ID LPAREN RPAREN compound_statement", 
									@$.first_line, @$.last_line);
			$$->addLeftChild($1);
			
			cout << "func_definition : type_specifier ID LPAREN RPAREN compound_statement" << endl;

		}
 		;				


parameter_list  : 
		parameter_list COMMA type_specifier ID
		{
			$4 = new SymbolInfo($4->getName(), "VARIABLE");
			$4->setDataType(dataType);
			funcParams->add($4);
			
			ParseTree* comma = new ParseTree("COMMA : ," , @2.last_line, 0);
			ParseTree* id = new ParseTree("ID", $4, @4.last_line, 0);
			constantNum = @4.last_line;
			
			$1->addSibling(comma);
			comma->addSibling($3);
			$3->addSibling(id);
			
			$$ = new ParseTree("parameter_list : parameter_list COMMA type_specifier ID", 
									@$.first_line, @$.last_line);
			$$->addLeftChild($1);

			cout << "parameter_list : parameter_list COMMA type_specifier ID" << endl;
		}

		| parameter_list COMMA type_specifier
		{
			ParseTree* comma = new ParseTree("COMMA : ," , @2.last_line, 0);
			
			$1->addSibling(comma);
			comma->addSibling($3);
			
			$$ = new ParseTree("parameter_list : parameter_list COMMA type_specifier", 
									@$.first_line, @$.last_line);
			$$->addLeftChild($1);
			
			cout << "parameter_list : parameter_list COMMA type_specifier" << endl;
		}

 		| type_specifier ID
		{
			funcParams = new FunctionParams();

			$2 = new SymbolInfo($2->getName(), "VARIABLE");
			$2->setDataType(dataType);
			funcParams->add($2);
			
			ParseTree* id = new ParseTree("ID", $2, @2.last_line, 0);
			$1->addSibling(id);
			constantNum = @2.last_line;
			
			$$ = new ParseTree("parameter_list : type_specifier ID", 
									@$.first_line, @$.last_line);
			$$->addLeftChild($1);

			cout << "parameter_list : type_specifier ID" << endl;
		}

		| type_specifier
		{
			$$ = new ParseTree("parameter_list : type_specifier", 
									@$.first_line, @$.last_line);
			$$->addLeftChild($1);

			cout << "parameter_list : type_specifier" << endl;
		}
 		;

 		
compound_statement : 
		LCURL 
		{
			table->enterScope();

			if(funcParams != nullptr){
				funcParams->moveToHead();
				while(!funcParams->lastParam()){
					SymbolInfo* parameter = funcParams->nextParam();
					/* parameter = new SymbolInfo(parameter->getName(), 
									parameter->getType(), parameter->getDataType()); */
					
					if(!table->insert(parameter)) {
						yyerror("Redefinition of parameter '" + parameter->getName() + "'", constantNum);
					}
				}
			}
		}
		statements RCURL
		{
			ParseTree* lcurl = new ParseTree("LCURL : {", @1.last_line, 0);
			ParseTree* rcurl = new ParseTree("RCURL : }", @4.last_line, 0);
			
			lcurl->addSibling($3);
			$3->addSibling(rcurl);
			
			$$ = new ParseTree("compound_statement : LCURL statements RCURL", 
									@$.first_line, @$.last_line);
			$$->addLeftChild(lcurl);
			
			cout << "compound_statement : LCURL statements RCURL" << endl;

			table->printAllScopeTables();
			table->exitScope();
		}

		| LCURL RCURL
		{
			ParseTree* lcurl = new ParseTree("LCURL : {", @1.last_line, 0);
			ParseTree* rcurl = new ParseTree("RCURL : }", @2.last_line, 0);
			
			lcurl->addSibling(rcurl);
			
			$$ = new ParseTree("compound_statement : LCURL RCURL", 
									@$.first_line, @$.last_line);
			$$->addLeftChild(lcurl);
			
			cout << "compound_statement : LCURL RCURL" << endl;
		}
		;
 		    
var_declaration : 
		type_specifier declaration_list SEMICOLON
		{
			ParseTree* semicolon = new ParseTree("SEMICOLON : ;" , @3.last_line, 0);
			
			$1->addSibling($2);
			$2->addSibling(semicolon);
			
			$$ = new ParseTree("var_declaration : type_specifier declaration_list SEMICOLON", 
									@$.first_line, @$.last_line);
			$$->addLeftChild($1);

			cout << "var_declaration : type_specifier declaration_list SEMICOLON" << endl;
		}
 		;
 		 
type_specifier : 
		INT
		{ 
			ParseTree* integer = new ParseTree("INT : int" , @1.last_line, 0);
			
			$$ = new ParseTree("type_specifier : INT", 
									@$.first_line, @$.last_line);
			$$->addLeftChild(integer);
			$$->setDataType("INT");

			dataType = "INT";
			cout << "type_specifier : INT" << endl;
		}

 		| FLOAT
		{ 
			ParseTree* flt = new ParseTree("FLOAT : float" , @1.last_line, 0);
			
			$$ = new ParseTree("type_specifier : FLOAT", 
									@$.first_line, @$.last_line);
			$$->addLeftChild(flt);
			$$->setDataType("FLOAT");

			dataType = "FLOAT";
			cout << "type_specifier : FLOAT" << endl;
		}

 		| VOID
		{ 
			ParseTree* vd = new ParseTree("VOID : void" , @1.last_line, 0);
			
			$$ = new ParseTree("type_specifier : VOID", 
									@$.first_line, @$.last_line);
			$$->addLeftChild(vd);
			$$->setDataType("VOID");

			dataType = "VOID";
			cout << "type_specifier : VOID" << endl;
		}
 		;
 		
declaration_list : 
		declaration_list COMMA ID
		{
			if(!voidVariable($3))
				insertVariable($3, "VARIABLE");
			
			ParseTree* comma = new ParseTree("COMMA : ," , @2.last_line, 0);
			ParseTree* id = new ParseTree("ID", $3, @3.last_line, 0);
			
			$1->addSibling(comma);
			comma->addSibling(id);
			
			$$ = new ParseTree("declaration_list : declaration_list COMMA ID", 
									@$.first_line, @$.last_line);
			$$->addLeftChild($1);

			cout << "declaration_list : declaration_list COMMA ID" << endl;
		}

		| declaration_list COMMA ID LTHIRD CONST_INT RTHIRD
		{
			if(!voidVariable($3))
				insertVariable($3, "ARRAY");

			ParseTree* comma = new ParseTree("COMMA : ," , @2.last_line, 0);
			ParseTree* id = new ParseTree("ID", $3, @3.last_line, 0);
			ParseTree* lthird = new ParseTree("LSQUARE : [" , @4.last_line, 0);
			ParseTree* const_int = new ParseTree("CONST_INT", $5, @5.last_line, 0);
			ParseTree* rthird = new ParseTree("RSQUARE : ]" , @6.last_line, 0);
			
			$1->addSibling(comma);
			comma->addSibling(id);
			id->addSibling(lthird);
			lthird->addSibling(const_int);
			const_int->addSibling(rthird);
			
			$$ = new ParseTree("declaration_list : declaration_list COMMA ID LSQUARE CONST_INT RSQUARE", 
									@$.first_line, @$.last_line);
			$$->addLeftChild($1);
			
			cout << "declaration_list : declaration_list COMMA ID LSQUARE CONST_INT RSQUARE" << endl;
		}

		| ID
		{ 
			if(!voidVariable($1))
				insertVariable($1, "VARIABLE");

			ParseTree* id = new ParseTree("ID", $1, @1.last_line, 0);
			
			$$ = new ParseTree("declaration_list : ID", 
									@$.first_line, @$.last_line);
			$$->addLeftChild(id);

			cout << "declaration_list : ID" << endl;
		}

		| ID LTHIRD CONST_INT RTHIRD
		{
			if(!voidVariable($1))
				insertVariable($1, "ARRAY");

			ParseTree* id = new ParseTree("ID", $1, @1.last_line, 0);
			ParseTree* lthird = new ParseTree("LSQUARE : [" , @2.last_line, 0);
			ParseTree* const_int = new ParseTree("CONST_INT", $3, @3.last_line, 0);
			ParseTree* rthird = new ParseTree("RSQUARE : ]" , @4.last_line, 0);
			
			id->addSibling(lthird);
			lthird->addSibling(const_int);
			const_int->addSibling(rthird);
			
			$$ = new ParseTree("declaration_list : ID LSQUARE CONST_INT RSQUARE", 
									@$.first_line, @$.last_line);
			$$->addLeftChild(id);
			
			cout << "declaration_list : ID LSQUARE CONST_INT RSQUARE" << endl;
		}
		;
 		  
statements : 
		statement
		{
			$$ = new ParseTree("statements : statement", 
									@$.first_line, @$.last_line);
			$$->addLeftChild($1);

			cout << "statements : statement" << endl;
		}

		| statements statement
		{
			$1->addSibling($2);
			$$ = new ParseTree("statements : statements statement", 
									@$.first_line, @$.last_line);
			$$->addLeftChild($1);

			cout << "statements : statements statement" << endl;
		}
		;
	   
statement : 
		var_declaration
		{
			$$ = new ParseTree("statement : var_declaration", 
									@$.first_line, @$.last_line);
			$$->addLeftChild($1);

			cout << "statement : var_declaration" << endl;
		}

		| expression_statement
		{
			$$ = new ParseTree("statement : expression_statement", 
									@$.first_line, @$.last_line);
			$$->addLeftChild($1);

			cout << "statement : expression_statement" << endl;
		}

		| compound_statement
		{
			$$ = new ParseTree("statement : compound_statement", 
									@$.first_line, @$.last_line);
			$$->addLeftChild($1);

			cout << "statement : compound_statement" << endl; 
		}

		| FOR LPAREN expression_statement expression_statement expression RPAREN statement
		{
			ParseTree* node_for = new ParseTree("FOR : for" , @1.last_line, 0);
			ParseTree* lparen = new ParseTree("LPAREN : (", @2.last_line, 0);
			ParseTree* rparen = new ParseTree("RPAREN : )", @6.last_line, 0);
			
			node_for->addSibling(lparen);
			lparen->addSibling($3);
			$3->addSibling($4);
			$4->addSibling($5);
			$5->addSibling(rparen);
			rparen->addSibling($7);
			
			$$ = new ParseTree("statement : FOR LPAREN expression_statement expression_statement expression RPAREN statement", 
									@$.first_line, @$.last_line);
			$$->addLeftChild(node_for);
			
			
			cout << "statement : FOR LPAREN expression_statement expression_statement" << 
					"expression RPAREN statement" << endl; 
		}

		| IF LPAREN expression RPAREN statement %prec LOWER_THAN_ELSE
		{
			ParseTree* node_if = new ParseTree("IF : if" , @1.last_line, 0);
			ParseTree* lparen = new ParseTree("LPAREN : (", @2.last_line, 0);
			ParseTree* rparen = new ParseTree("RPAREN : )", @4.last_line, 0);
			
			node_if->addSibling(lparen);
			lparen->addSibling($3);
			$3->addSibling(rparen);
			rparen->addSibling($5);
			
			$$ = new ParseTree("statement : IF LPAREN expression RPAREN statement", 
									@$.first_line, @$.last_line);
			$$->addLeftChild(node_if);
			
			cout << "statement : IF LPAREN expression RPAREN statement" << endl;

		}

		| IF LPAREN expression RPAREN statement ELSE statement
		{
			ParseTree* node_if = new ParseTree("IF : if" , @1.last_line, 0);
			ParseTree* lparen = new ParseTree("LPAREN : (", @2.last_line, 0);
			ParseTree* rparen = new ParseTree("RPAREN : )", @4.last_line, 0);
			ParseTree* node_else = new ParseTree("ELSE : else", @6.last_line, 0);
			
			node_if->addSibling(lparen);
			lparen->addSibling($3);
			$3->addSibling(rparen);
			rparen->addSibling($5);
			$5->addSibling(node_else);
			node_else->addSibling($7);
			
			$$ = new ParseTree("statement : IF LPAREN expression RPAREN statement ELSE statement", 
									@$.first_line, @$.last_line);
			$$->addLeftChild(node_if);
			
			cout << "statement : IF LPAREN expression RPAREN statement ELSE statement" << endl;
		}

		| WHILE LPAREN expression RPAREN statement
		{
			ParseTree* node_while = new ParseTree("WHILE : while" , @1.last_line, 0);
			ParseTree* lparen = new ParseTree("LPAREN : (", @2.last_line, 0);
			ParseTree* rparen = new ParseTree("RPAREN : )", @4.last_line, 0);
			
			node_while->addSibling(lparen);
			lparen->addSibling($3);
			$3->addSibling(rparen);
			rparen->addSibling($5);
			
			$$ = new ParseTree("statement : WHILE LPAREN expression RPAREN statement", 
									@$.first_line, @$.last_line);
			$$->addLeftChild(node_while);
			
			cout << "statement : WHILE LPAREN expression RPAREN statement" << endl;
		}

		| PRINTLN LPAREN ID RPAREN SEMICOLON
		{
			ParseTree* println = new ParseTree("PRINTLN : println" , @1.last_line, 0);
			ParseTree* lparen = new ParseTree("LPAREN : (", @2.last_line, 0);
			ParseTree* id = new ParseTree("ID", $3, @3.last_line, 0);
			ParseTree* rparen = new ParseTree("RPAREN : )", @4.last_line, 0);
			ParseTree* semicolon = new ParseTree("SEMICOLON : ;", @5.last_line, 0);
			
			println->addSibling(lparen);
			lparen->addSibling(id);
			id->addSibling(rparen);
			rparen->addSibling(semicolon);
			
			$$ = new ParseTree("statement : PRINTLN LPAREN ID RPAREN SEMICOLON", 
									@$.first_line, @$.last_line);
			$$->addLeftChild(println);
			
			cout << "statement : PRINTLN LPAREN ID RPAREN SEMICOLON" << endl;
		}

		| RETURN expression SEMICOLON
		{
			ParseTree* node_return = new ParseTree("RETURN : return" , @1.last_line, 0);
			ParseTree* semicolon = new ParseTree("SEMICOLON : ;", @3.last_line, 0);
			
			node_return->addSibling($2);
			$2->addSibling(semicolon);
			
			$$ = new ParseTree("statement : RETURN expression SEMICOLON", 
									@$.first_line, @$.last_line);
			$$->addLeftChild(node_return);
			
			cout << "statement : RETURN expression SEMICOLON" << endl;
		}
		;
	  

expression_statement : 
		SEMICOLON
		{
			ParseTree* semicolon = new ParseTree("SEMICOLON : ;", @1.last_line, 0);
		
			$$ = new ParseTree("expression_statement : SEMICOLON", 
									@$.first_line, @$.last_line);
			$$->addLeftChild(semicolon);

			cout << "expression_statement : SEMICOLON" << endl;
		}	

		| expression SEMICOLON 
		{
			ParseTree* semicolon = new ParseTree("SEMICOLON : ;", @2.last_line, 0);
			$1->addSibling(semicolon);
		
			$$ = new ParseTree("expression_statement : expression SEMICOLON", 
									@$.first_line, @$.last_line);
			$$->addLeftChild($1);
			
			cout << "expression_statement : expression SEMICOLON" << endl;
		}
		;


variable : ID
		{
			if(!declared($1)){
				string errorMsg = "Undeclared variable '" + $1->getName() + "'";
				yyerror(errorMsg);
			}

			ParseTree* id = new ParseTree("ID", $1, @1.last_line, 0);
		
			$$ = new ParseTree("variable : ID", 
									@$.first_line, @$.last_line);
			$$->addLeftChild(id);

			if($1->getType() == "VARIABLE")
				$$->setDataType($1->getDataType());
			else
				$$->setDataType("INVALID");
			
			cout << "variable : ID" << endl;
		}

		| ID LTHIRD expression RTHIRD 
		{
			if(!declared($1)) {
				string errorMsg = "Undeclared variable '" + $1->getName() + "'";
				yyerror(errorMsg);
			}
			if($1->getType() != "ARRAY") {
				string errorMsg = "'" + $1->getName() + "' is not an array";
				yyerror(errorMsg);
			}
			if($3->getDataType() != "INT") {
				yyerror("Array subscript is not an integer");
			}

			ParseTree* id = new ParseTree("ID", $1, @1.last_line, 0);
			ParseTree* lthird = new ParseTree("LSQUARE : [" , @2.last_line, 0);
			ParseTree* rthird = new ParseTree("RSQUARE : ]" , @4.last_line, 0);
			
			id->addSibling(lthird);
			lthird->addSibling($3);
			$3->addSibling(rthird);
			
			$$ = new ParseTree("variable : ID LSQUARE expression RSQUARE", 
									@$.first_line, @$.last_line);
			$$->addLeftChild(id);
			
			if($1->getType() == "ARRAY")
				$$->setDataType($1->getDataType());
			else
				$$->setDataType("INVALID");

			cout << "variable : ID LSQUARE expression RSQUARE" << endl;
		}
		;
	 
expression : 
		logic_expression
		{
			if(expOperation && $1->getDataType() == "VOID")
				yyerror("Void cannot be used in expression");
			
			$$ = new ParseTree("expression : logic_expression", 
									@$.first_line, @$.last_line);
			$$->addLeftChild($1);
			$$->setDataType($1->getDataType());
			
			cout << "expression : logic_expression" << endl;
			expOperation = false;
		}

		| variable ASSIGNOP logic_expression
		{
			if($3->getDataType() == "VOID")
				yyerror("Void cannot be used in expression");
			
			if(($1->getDataType() == "INT") && ($3->getDataType() == "FLOAT")){
				yyerror("Warning: possible loss of data in assignment of FLOAT to INT");
			}
				
			ParseTree* assignop = new ParseTree("ASSIGNOP : =", @2.last_line, 0);
			
			$1->addSibling(assignop);
			assignop->addSibling($3);
			
			$$ = new ParseTree("expression : variable ASSIGNOP logic_expression", 
									@$.first_line, @$.last_line);
			$$->addLeftChild($1);
			$$->setDataType($1->getDataType());
			
			cout << "expression : variable ASSIGNOP logic_expression" << endl;
			expOperation = false;
		} 	
		;
			
logic_expression : 
		rel_expression
		{
			$$ = new ParseTree("logic_expression : rel_expression", 
									@$.first_line, @$.last_line);
			$$->addLeftChild($1);
			$$->setDataType($1->getDataType());
			
			cout << "logic_expression : rel_expression" << endl;
		}

		| rel_expression LOGICOP rel_expression
		{
			ParseTree* logicop = new ParseTree("LOGICOP", $2, @2.last_line, 0);
			
			$1->addSibling(logicop);
			logicop->addSibling($3);
			
			$$ = new ParseTree("logic_expression : rel_expression LOGICOP rel_expression", 
									@$.first_line, @$.last_line);
			$$->addLeftChild($1);
			$$->setDataType("INT");
			
			cout << "logic_expression : rel_expression LOGICOP rel_expression" << endl;
			expOperation = true;
		} 	
		;
			
rel_expression	: 
		simple_expression 
		{
			$$ = new ParseTree("rel_expression : simple_expression", 
									@$.first_line, @$.last_line);
			$$->addLeftChild($1);
			$$->setDataType($1->getDataType());
			
			cout << "rel_expression : simple_expression" << endl;
		}

		| simple_expression RELOP simple_expression
		{
			ParseTree* relop = new ParseTree("RELOP", $2, @2.last_line, 0);
			
			$1->addSibling(relop);
			relop->addSibling($3);
			
			$$ = new ParseTree("rel_expression : simple_expression RELOP simple_expression", 
									@$.first_line, @$.last_line);
			$$->addLeftChild($1);
			$$->setDataType("INT");
			
			cout << "rel_expression : simple_expression RELOP simple_expression" << endl;
			expOperation = true;
		}	
		;
				
simple_expression :
		term 
		{
			$$ = new ParseTree("simple_expression : term", 
									@$.first_line, @$.last_line);
			$$->addLeftChild($1);
			$$->setDataType($1->getDataType());
			
			cout << "simple_expression : term" << endl;
		}

		| simple_expression ADDOP term
		{
			ParseTree* addop = new ParseTree("ADDOP", $2, @2.last_line, 0);
			
			$1->addSibling(addop);
			addop->addSibling($3);
			
			$$ = new ParseTree("simple_expression : simple_expression ADDOP term", 
									@$.first_line, @$.last_line);
			$$->addLeftChild($1);
			$$->setDataType(operationDataType($1, $3));

			cout << "simple_expression : simple_expression ADDOP term" << endl;
			expOperation = true;
		} 
		;
					
term :	unary_expression
		{
			$$ = new ParseTree("term : unary_expression", 
									@$.first_line, @$.last_line);
			$$->addLeftChild($1);
			$$->setDataType($1->getDataType());
			
			cout << "term :	unary_expression" << endl;
		}

		| term MULOP unary_expression
		{
			ParseTree* mulop = new ParseTree("MULOP", $2, @2.last_line, 0);
			
			$1->addSibling(mulop);
			mulop->addSibling($3);
			
			$$ = new ParseTree("term : term MULOP unary_expression", 
									@$.first_line, @$.last_line);
			$$->addLeftChild($1);

			string type = operationDataType($1, $3);
			
			if($2->getName() == "%" && type != "INT") {
				yyerror("Operands of modulus must be integers");
				$$->setDataType("INVALID");
			}
			else
				$$->setDataType(type);

			if(devidedByZero($2)) {
				yyerror("Warning: division by zero i=0f=1Const=0");
			}
			
			cout << "term :	term MULOP unary_expression" << endl;
			expOperation = true;
		}
		;

unary_expression : 
		ADDOP unary_expression  
		{
			ParseTree* addop = new ParseTree("ADDOP", $1, @1.last_line, 0);
			
			addop->addSibling($2);
			
			$$ = new ParseTree("unary_expression : ADDOP unary_expression", 
									@$.first_line, @$.last_line);
			$$->addLeftChild(addop);
			$$->setDataType($1->getDataType());
			
			cout << "unary_expression : ADDOP unary_expression" << endl;
			expOperation = true;
		}

		| NOT unary_expression 
		{
			ParseTree* node_not = new ParseTree("NOT : !", @1.last_line, 0);
			
			node_not->addSibling($2);
			
			$$ = new ParseTree("unary_expression : NOT unary_expression", 
									@$.first_line, @$.last_line);
			$$->addLeftChild(node_not);
			$$->setDataType($2->getDataType());
			
			cout << "unary_expression : NOT unary_expression" << endl;
			expOperation = true;
		}

		| factor 
		{
			$$ = new ParseTree("unary_expression : factor", 
									@$.first_line, @$.last_line);
			$$->addLeftChild($1);
			$$->setDataType($1->getDataType());

			cout << "unary_expression : factor" << endl;
		}
		;
	
factor	: 
		variable 
		{
			$$ = new ParseTree("factor : variable", 
									@$.first_line, @$.last_line);
			$$->addLeftChild($1);
			$$->setDataType($1->getDataType());
			
			cout << "factor : variable" << endl;
		}

		| ID 
		{
			funcParams = $1->getFunctionParams();
			if(funcParams) 
				funcParams->moveToHead();
			sinfo = $1;
		}
		LPAREN argument_list RPAREN
		{
			if(!declared(sinfo)){
				string errorMsg = "Undeclared function '" + sinfo->getName() + "'";
				yyerror(errorMsg);
			}
			
			if(funcParams && !funcParams->lastParam()){
				yyerror("Too few arguments to function '" + sinfo->getName() + "'");
			}

			ParseTree* id = new ParseTree("ID", sinfo, @1.last_line, 0);
			ParseTree* lparen = new ParseTree("LPAREN : (" , @3.last_line, 0);
			ParseTree* rparen = new ParseTree("RPAREN : )" , @5.last_line, 0);
			
			id->addSibling(lparen);
			lparen->addSibling($4);
			$4->addSibling(rparen);
			
			$$ = new ParseTree("factor : ID LPAREN argument_list RPAREN", 
									@$.first_line, @$.last_line);
			$$->addLeftChild(id);
			$$->setDataType(sinfo->getDataType());
			

			cout << "factor : ID LPAREN argument_list RPAREN" << endl;
			resetFuncCall();
		}

		| LPAREN expression RPAREN
		{
			ParseTree* lparen = new ParseTree("LPAREN : (" , @1.last_line, 0);
			ParseTree* rparen = new ParseTree("RPAREN : )" , @3.last_line, 0);
			
			lparen->addSibling($2);
			$2->addSibling(rparen);
			
			$$ = new ParseTree("factor : LPAREN expression RPAREN", 
									@$.first_line, @$.last_line);
			$$->addLeftChild(lparen);
			$$->setDataType($2->getDataType());

			cout << "factor : LPAREN expression RPAREN" << endl;
		}

		| CONST_INT
		{
			
			
			ParseTree* const_int = new ParseTree("CONST_INT", $1, @1.last_line, 0);
			
			$$ = new ParseTree("factor : CONST_INT", 
									@$.first_line, @$.last_line);
			$$->addLeftChild(const_int);
			$$->setDataType("INT");

			constantNum = stoi($1->getName());
			cout << "factor : CONST_INT" << endl;
		}

		| CONST_FLOAT
		{
			ParseTree* const_float = new ParseTree("CONST_FLOAT : " + $1->getName() , @1.last_line, 0);
			
			$$ = new ParseTree("factor : CONST_FLOAT", 
									@$.first_line, @$.last_line);
			$$->addLeftChild(const_float);
			$$->setDataType("FLOAT");

			constantNum = stoi($1->getName());
			cout << "factor : CONST_FLOAT" << endl;
		}

		| variable INCOP 
		{
			ParseTree* inop = new ParseTree("INCOP : ++", @1.last_line, 0);
			
			$1->addSibling(inop);
			
			$$ = new ParseTree("factor : variable INCOP", 
									@$.first_line, @$.last_line);
			$$->addLeftChild($1);
			$$->setDataType($1->getDataType());

			cout << "factor : variable INCOP" << endl;
			expOperation = true;
		}

		| variable DECOP
		{
			ParseTree* decop = new ParseTree("INCOP : --", @1.last_line, 0);
			
			$1->addSibling(decop);
			
			$$ = new ParseTree("factor : variable DECOP", 
									@$.first_line, @$.last_line);
			$$->addLeftChild($1);
			$$->setDataType($1->getDataType());
			
			cout << "factor : variable DECOP" << endl;
			expOperation = true;
		}
		;
	
argument_list : 
		arguments
		{
			$$ = new ParseTree("argument_list : arguments", 
									@$.first_line, @$.last_line);
			$$->addLeftChild($1);
			cout << "argument_list : arguments" << endl;
		}
		|
		{
			$$ = new ParseTree("argument_list : ", 
									@$.first_line, @$.last_line);
			cout << "argument_list : " << endl;
		}
		;
	
arguments : 
		arguments COMMA logic_expression
		{
			if(sinfo->getType()=="FUNCTION")
				checkFunctionCall($3->getDataType());
			
			ParseTree* comma = new ParseTree("COMMA : ," , @2.last_line, 0);
			
			$1->addSibling(comma);
			comma->addSibling($3);
			
			$$ = new ParseTree("arguments : arguments COMMA logic_expression", 
									@$.first_line, @$.last_line);
			$$->addLeftChild($1);
			
			cout << "arguments : arguments COMMA logic_expression" << endl;
		}

		| logic_expression
		{
			if(sinfo->getType()=="FUNCTION")
				checkFunctionCall($1->getDataType());
			
			$$ = new ParseTree("arguments : logic_expression", 
									@$.first_line, @$.last_line);
			$$->addLeftChild($1);
			
			cout << "arguments : logic_expression" << endl;
		}
		;
 

%%
int main(int argc,char *argv[])
{
	string LOG_FILE = "2005036_log.txt";
	string ERROR_FILE = "2005036_error.txt";
	string TREE_FILE = "2005036_parsetree.txt";

	if((fp=fopen(argv[1],"r"))==NULL)
	{
		printf("Cannot Open Input File.\n");
		exit(1);
	}

	log_out.open(LOG_FILE);
	tree_out.open(TREE_FILE);
	error_out.open(ERROR_FILE);

	/* streambuf*  */coutBuffer = cout.rdbuf();
	cout.rdbuf(log_out.rdbuf());

	yyin=fp;
	yyparse();


	
	/* cout.rdbuf(coutBuffer); */

	log_out.close();
	tree_out.close();
	error_out.close();


	return 0;
}

string operationDataType(ParseTree* oparand1, ParseTree* oparand2) {
	if((oparand1->getDataType() == "INVALID") || 
			(oparand2->getDataType() == "INVALID")) {
				return "INVALID";
	}
	else if((oparand1->getDataType() == "VOID") || 
				(oparand2->getDataType() == "VOID")) {
					return "VOID";
	}
	else if((oparand1->getDataType() == "FLOAT") || 
				(oparand2->getDataType() == "FLOAT")) {
					return "FLOAT";
	}
	else
		return "INT";
}

bool voidVariable(SymbolInfo* variable){
	if(dataType == "VOID"){
		string errorMsg = "Variable or field '" + variable->getName() + 
							"' declared void";
		yyerror(errorMsg);

		return true;
	}

	return false;
}

void insertVariable(SymbolInfo* symbolInfo, string type) {
	/* alredy in symbol table, could be in diff scope */
	SymbolInfo* matchedInfo = table->lookup(symbolInfo->getName());
	if(matchedInfo != nullptr)
		symbolInfo = new SymbolInfo(symbolInfo->getName(), type);
	
	/* successful insert only if not in curr scope */
	if(table->insert(symbolInfo)){
		symbolInfo->setDataType(dataType);
		symbolInfo->setType(type);
	}
	else {
		if(matchedInfo->getType() != type)
			yyerror("'" + symbolInfo->getName()  + "' redeclared as different kind of symbol");
		else if(matchedInfo->getDataType() == dataType)
			yyerror("Redefinition of variable '"+ symbolInfo->getName() + "'");
		else
			yyerror("Conflicting types for '"+ symbolInfo->getName() + "'");
	}
}

void insertFunction(SymbolInfo* symbolInfo, string returnType, int line){

	if(table->insert(symbolInfo)) {
		symbolInfo->setDataType(returnType);
		symbolInfo->setType("FUNCTION");
		symbolInfo->setFunctionParam(funcParams);
	}
	else if(symbolInfo->getType() != "FUNCTION") {
		yyerror("'" + symbolInfo->getName()  + "' redeclared as different kind of symbol", line);
	}

	funcParams = nullptr;
}

bool checkFuncDefination(SymbolInfo* func, string returnType) {
	/* function not declared or matched symbol not a function */
	if((func->getDataType() == "") || (func->getType() != "FUNCTION")) {
		return true;
	}
	
	/* mismatch in return type */
	if(returnType != func->getDataType()){
		return false;
	}

	FunctionParams* declaredParams = func->getFunctionParams();

	/* no function params, nothing to check */
	if(declaredParams == nullptr && funcParams == nullptr) {
		return true;
	}
	else if(declaredParams == nullptr || funcParams == nullptr) {
		return false;
	}
	/* check parameters */
	else{

		/* mismatch in parameter num */
		if(declaredParams->getParamNum() != funcParams->getParamNum()) {
			return false;
		}

		/* check params data type  */
		declaredParams->moveToHead();
		funcParams->moveToHead();

		while(!declaredParams->lastParam() && !funcParams->lastParam()) {
			SymbolInfo* definedInfo = funcParams->nextParam();
			SymbolInfo* declaredInfo = declaredParams->nextParam();

			if(definedInfo->getDataType() != declaredInfo->getDataType()) {
				return false;
			}
		}
		return true;
	}
}


void checkFunctionCall(string calledType){
	count++;

	if(funcParams == nullptr){
		yyerror("Too many arguments to function '" + sinfo->getName() + "'");
		return;
	}

	SymbolInfo* param = funcParams->nextParam();
	if(param != nullptr) {
		if(!matchType(param->getDataType(), calledType)) {
			yyerror("Type mismatch for argument " + to_string(count) + 
				" of '" + sinfo->getName() + "'");
		}
	}
	else{
		yyerror("Too many arguments to function '" + sinfo->getName() + "'");
	}
}

void resetFuncCall(){
	sinfo = nullptr;
	funcParams = nullptr;
	count = 0;
}

bool matchType(string t1, string t2) {
	return (t1 == t2) || (t1 == "FLOAT" && t2 == "INT");
}

bool devidedByZero(SymbolInfo* operand) {
	return ((operand->getName() == "%") || (operand->getName() == "/")) &&
				(constantNum == 0);

}