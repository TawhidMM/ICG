%option noyywrap
%option yylineno


%x CONST_CHAR
%x STRING_STATE
%x SINGLE_LINE_CMNT
%x MULTI_LINE_CMNT


%{
#include <iostream>
#include <fstream>
#include "2005036_SymbolTable.h"
#include "2005036_SymbolInfo.h"
#include "2005036_ParseTree.h"
#include "y.tab.h"

using namespace std;


int yycolumn = 1;

#define YY_USER_ACTION \
	yylloc.first_line = yylloc.last_line = yylineno; \
	yylloc.first_column = yycolumn; \
	yylloc.last_column = yycolumn + yyleng - 1; \
	yycolumn += yyleng;


extern SymbolTable *table;
extern ofstream log_out;

void printLogFile(string tokenName, string lexeme);
void printTokenFile(string tokenType, string token);
void printError(string error, string lexeme);
void replaceEscapeChar(const char* chars);
void printIndentationError();
void checkIndentation();


int errorCount = 0;
int warningCount = 0;

int expectedTab = 0;
int tabCount = 0;
int spaceCount = 0;
bool foundNewLine = true;

int lineCount;
bool isMultiline;

string lexeme;
string token;

%}

DIGIT [0-9]
DIGITS	{DIGIT}*

INT {DIGIT}+

EXP_NUM		{INT}E[+-]?{INT}
DECIMAL_POINT_NUM	{DIGITS}\.{INT}
EXP_AND_DEC_NUM		{DIGITS}(\.{INT})(E[+-]?{INT})

FLOAT	{EXP_NUM}|{DECIMAL_POINT_NUM}|{EXP_AND_DEC_NUM}

REDUNDENT_DECIMAL	{DIGITS}\.({DIGITS}\.)+{DIGITS}(E[+-]?{INT})?
ILL_FORMED_NUM	{INT}E({FLOAT}|{REDUNDENT_DECIMAL})
INVALID_NUM		({INT}|{FLOAT}){CHAR}+

NEWLINE		\n|\r\n
TAB		\t
SPACE	[ ]

CHAR	[A-Za-z]
ALPHANUMERIC	[A-Za-z0-9_]
ESP_CHAR \\['"ntafrbv0\\]

%%

{NEWLINE}	{ 
	foundNewLine = true;
	tabCount = 0;
	spaceCount = 0; 
	yycolumn = 1;
}
{TAB}	{ 
	tabCount++; 
}
{SPACE}	{
	spaceCount++;
}

"if"		{
	printLogFile("IF", yytext);
	return IF;
}	
"else"  	{
	printLogFile("ELSE", yytext);
	return ELSE;
}
"for"   	{
	printLogFile("FOR", yytext);
	return FOR;
}
"while"		{
	printLogFile("WHILE", yytext);
	return WHILE;
}
"do"    	{
	printLogFile("DO", yytext);
	return DO;
}
"break" 	{
	printLogFile("BREAK", yytext);
	return BREAK;
}
"int"   	{
	printLogFile("INT", yytext);
	return INT;
}
"char"  	{
	printLogFile("CHAR", yytext);
	return CHAR;
}
"float"		{
	printLogFile("FLOAT", yytext);
	return FLOAT;
}
"double"	{
	printLogFile("DOUBLE", yytext);
	return DOUBLE;
}
"void"		{
	printLogFile("VOID", yytext);
	return VOID;
}
"return"	{
	printLogFile("RETURN", yytext);
	return RETURN;
}
"switch"    {
	printLogFile("SWITCH", yytext);
	return SWITCH;
}
"case"	    {
	printLogFile("CASE", yytext);
	return CASE;
}
"default"   {
	printLogFile("DEFAULT", yytext);
	return DEFAULT;
}
"continue"	{ 
	printLogFile("CONTINUE", yytext);
	return CONTINUE;
}
"println"	{ 
	printLogFile("PRINTLN", yytext);
	return PRINTLN;
}




{INT}	{ 
	// //checkIndentation();
	printLogFile("CONST_INT", yytext);
	
	SymbolInfo *s= new  SymbolInfo(yytext, "CONST_INT");
	yylval.symbolInfo = /* (YYSTYPE) */s;
	return CONST_INT;
}
{FLOAT}		{ 
	// //checkIndentation();
	printLogFile("CONST_FLOAT", yytext);
	
	SymbolInfo *s= new  SymbolInfo(yytext, "CONST_FLOAT");
	yylval.symbolInfo = /* (YYSTYPE) */s;
	return CONST_FLOAT;
}
{REDUNDENT_DECIMAL}		{
	// //checkIndentation();
	errorCount++;
	// printError("TOO_MANY_DECIMAL_POINTS", yytext);
}
{ILL_FORMED_NUM}	{
	// //checkIndentation();
	errorCount++;
	// printError("ILLFORMED_NUMBER", yytext);
}
{INVALID_NUM}	{
	// //checkIndentation();
	errorCount++;
	// printError("INVALID_ID_SUFFIX_NUM_PREFIX", yytext);
}


\'	{
	// //checkIndentation();
	BEGIN CONST_CHAR;
	lexeme = yytext;
	token = "";
}
<CONST_CHAR>\'	{
	lexeme += yytext;

	if(token.size() == 0){
		errorCount++;
		// printError("EMPTY_CONST_CHAR", lexeme);
	}
	else if(token.size() == 1){
		printLogFile("CONST_CHAR", token);
		//printTokenFile("CONST_CHAR", token);

		SymbolInfo *s= new  SymbolInfo(token, "CONST_CHAR");
		yylval.symbolInfo = /* (YYSTYPE) */s;
		return CONST_CHAR;


	}
	else{
		errorCount++;
		// printError("MULTICHAR_CONST_CHAR", lexeme);
	}

	BEGIN INITIAL;
}
<CONST_CHAR>[^'\n\r]	{
	lexeme += yytext;
	token += yytext;
}
<CONST_CHAR>{ESP_CHAR}	{
	lexeme += yytext;
	replaceEscapeChar(yytext);
}
<CONST_CHAR>{NEWLINE}	{
	//re-initiating indentation params
	yycolumn = 1;
	foundNewLine = true;
	tabCount = 0;
	spaceCount = 0;

	//pointing to unfinished char line
	yylineno--;

	errorCount++;
	// printError("UNFINISHED_CONST_CHAR", lexeme);
	
	//restoring original line count
	yylineno++;
	BEGIN INITIAL;
}


"+"|"-"		{
	// //checkIndentation();
	//printTokenFile("ADDOP", yytext);
	printLogFile("ADDOP", yytext);

	SymbolInfo *s= new  SymbolInfo(yytext, "ADDOP");
	yylval.symbolInfo = /* (YYSTYPE) */s;
	return ADDOP;
}
"*"|"/"|"%"	{
	// //checkIndentation();
	//printTokenFile("MULOP", yytext);
	printLogFile("MULOP", yytext);

	SymbolInfo *s= new  SymbolInfo(yytext, "MULOP");
	yylval.symbolInfo = /* (YYSTYPE) */s;
	return MULOP;
}
"++"	{
	// //checkIndentation();
	//printTokenFile("INCOP", yytext);
	printLogFile("INCOP", yytext);

	return INCOP;
}
"--"	{
	// //checkIndentation();
	//printTokenFile("INCOP", yytext);
	printLogFile("INCOP", yytext);

	return DECOP;
}
"<"|"<="|">"|">="|"=="|"!="		{
	// //checkIndentation();
	//printTokenFile("RELOP", yytext);
	printLogFile("RELOP", yytext);

	SymbolInfo *s= new  SymbolInfo(yytext, "RELOP");
	yylval.symbolInfo = /* (YYSTYPE) */s;
	return RELOP;
}
"="			{
	// //checkIndentation();
	//printTokenFile("ASSIGNOP", yytext);
	printLogFile("ASSIGNOP", yytext);

	return ASSIGNOP;
}
"&&"|"||"	{
	// //checkIndentation();
	//printTokenFile("LOGICOP", yytext);
	printLogFile("LOGICOP", yytext);

	SymbolInfo *s= new  SymbolInfo(yytext, "LOGICOP");
	yylval.symbolInfo = /* (YYSTYPE) */s;
	return LOGICOP;
}
"&"|"|"|"^"|"<<"|">>"	{
	// //checkIndentation();
	//printTokenFile("BITOP", yytext);
	printLogFile("BITOP", yytext);

	SymbolInfo *s= new  SymbolInfo(yytext, "BITOP");
	yylval.symbolInfo = /* (YYSTYPE) */s;
	return BITOP;
}

"!"			{
	// //checkIndentation();
	//printTokenFile("NOT", yytext);
	printLogFile("NOT", yytext);

	return NOT;
}
"("			{
	// //checkIndentation();
	//printTokenFile("LPAREN", yytext);
	printLogFile("LPAREN", yytext);

	return LPAREN;
}
")"			{
	// //checkIndentation();
	//printTokenFile("RPAREN", yytext);
	printLogFile("RPAREN", yytext);

	return RPAREN;
}
"{"			{
	// //checkIndentation();
	expectedTab++;
	printLogFile("LCURL", yytext);
	
	/* printTokenFile("LCURL", yytext);
	printLogFile("LCURL", yytext); */

	// table->enterScope();

	return LCURL;
}
"}"			{
	expectedTab--;
	// //checkIndentation();
	
	//printTokenFile("RCURL", yytext);
	printLogFile("RCURL", yytext);

	/* table->printAllScopeTables();
	table->exitScope(); */

	return RCURL;
}

"["			{
	// //checkIndentation();
	//printTokenFile("LSQUARE", yytext);
	printLogFile("LSQUARE", yytext);
	return LTHIRD;
}
"]"			{
	// //checkIndentation();
	//printTokenFile("RSQUARE", yytext);
	printLogFile("RSQUARE", yytext);
	return RTHIRD;
}
","			{
	// //checkIndentation();
	//printTokenFile("COMMA", yytext);
	printLogFile("COMMA", yytext);

	return COMMA;

}
";"			{
	//checkIndentation();
	//printTokenFile("SEMICOLON", yytext);
	printLogFile("SEMICOLON", yytext);
	
	return SEMICOLON;
}	

(_|{CHAR}){ALPHANUMERIC}*	{
	//checkIndentation();
	
	
	printLogFile("ID", yytext);
	/* printTokenFile("ID", yytext);

	if(symbolTable->insert(yytext, "ID"))
		symbolTable->printAllScopeTables();
	else
		cout << '\t' << yytext << " already exists in the current ScopeTable" 
			<< endl; */

	SymbolInfo* s = table->lookup(yytext);
	if(s == NULL){
		s = new SymbolInfo(yytext, "ID");
		cout << "null " << yytext << endl;
	}else{
		cout << s->getName() << " found " << s->getType() << endl;
	}			
		

	yylval.symbolInfo = /* (YYSTYPE) */s;
			
	return ID;
}



\"  {
		//checkIndentation();
		lineCount = yylineno;
		lexeme = yytext;
		token = "";
		isMultiline = false;
		BEGIN STRING_STATE;
}

<STRING_STATE>\"	{
	//checkIndentation();
	lexeme += yytext;

	int tempLineNum = yylineno;
	yylineno = lineCount;

	if(isMultiline) {
		/* printTokenFile("MULTI LINE STRING", token);
		printLogFile("MULTI LINE STRING", lexeme);
 */
		SymbolInfo *s= new  SymbolInfo(token, "MULTI_LINE_STRING");
		yylval.symbolInfo = /* (YYSTYPE) */s;
		return MULTI_LINE_STRING;
	}	
	else {
		/* printTokenFile("SINGLE LINE STRING", token);
		printLogFile("SINGLE LINE STRING", lexeme); */
		SymbolInfo *s= new  SymbolInfo(token, "SINGLE_LINE_STRING");
		yylval.symbolInfo = /* (YYSTYPE) */s;
		return SINGLE_LINE_STRING;
	}
		

	yylineno = tempLineNum;
	
	BEGIN INITIAL;
}
<STRING_STATE>\\{NEWLINE}  {
	//checkIndentation();

	//re-initiating indentation params
	yycolumn = 1;
	foundNewLine = true;
	tabCount = 0;
	spaceCount = 0;

	lexeme += yytext ;
	isMultiline = true;
}
<STRING_STATE>{NEWLINE}  	{
	//re-initiating indentation params
	yycolumn = 1;
	foundNewLine = true;
	tabCount = 0;
	spaceCount = 0;

	//pointing to unfinished string line
	yylineno--;
	
	errorCount++;
	// printError("UNFINISHED_STRING", lexeme);

	//restoring original line count
	yylineno++;
	BEGIN INITIAL;
}
<STRING_STATE><<EOF>>  	{
	
	errorCount++;
	// printError("UNFINISHED_STRING", lexeme);
	
	BEGIN INITIAL;
}
<STRING_STATE>{ESP_CHAR}	{
	//checkIndentation();
	lexeme += yytext;
	replaceEscapeChar(yytext);
}
<STRING_STATE>{TAB}		{
	tabCount++;
	lexeme += yytext ;
	token += yytext ;
}
<STRING_STATE>{SPACE}	{
	spaceCount++;
	lexeme += yytext ;
	token += yytext ;
}
<STRING_STATE>[^\n\r\\\"\t ]+ {
	//checkIndentation();
	lexeme += yytext ;
	token += yytext ;
}			



\/\/	{
	lineCount = yylineno;
	lexeme = yytext;

	BEGIN SINGLE_LINE_CMNT;
}
<SINGLE_LINE_CMNT>(\\{NEWLINE}|[^\n\r])*	{
	lexeme += yytext;
}
<SINGLE_LINE_CMNT>{NEWLINE}		{
	//re-initiating indentation params
	yycolumn = 1;
	foundNewLine = true;
	tabCount = 0;
	spaceCount = 0;

	int tempLineNum = yylineno;
	yylineno = lineCount;

	//printLogFile("SINGLE LINE COMMENT", lexeme);

	yylineno = tempLineNum;
	BEGIN INITIAL;
}

"/*"	{
	BEGIN MULTI_LINE_CMNT;
	lineCount = yylineno;
	lexeme = yytext;
}
<MULTI_LINE_CMNT>"*/"	{
	lexeme += yytext;

	int tempLineNum = yylineno;
	yylineno = lineCount;
	
	//printLogFile("MULTI LINE COMMENT", lexeme);

	BEGIN INITIAL;
	yylineno = tempLineNum;
}

<MULTI_LINE_CMNT>[^*]*	{
	lexeme += yytext;
}
<MULTI_LINE_CMNT>\*	{
	lexeme += yytext;
}

<MULTI_LINE_CMNT><<EOF>>	{
	
	errorCount++;
	///printError("UNFINISHED_COMMENT", lexeme);	

	BEGIN INITIAL;
}

.	{
	//checkIndentation();
	errorCount++;
	//printError("UNRECOGNIZED_CHAR", yytext);	
}

%%

/* int yywrap(void)
{
	return 1;
} */

void printLogFile(string tokenName, string lexeme){
	log_out << "Line# " << yylineno << ": Token <" << tokenName << "> Lexeme " <<
		lexeme << " found" << endl;
}
/* void printError(string error, string lexeme){
	cout << "Error at line# " << yylineno << ": " << error << " " <<
		lexeme << endl;
}
void printTokenFile(string tokenType, string token){
	tokenout << "<" << tokenType << ", " << token << ">" << endl;
}


void checkIndentation(){
	if(foundNewLine){
		if(spaceCount != 0){
			warningCount++;

			cout << "Line# " << yylineno << ": Warning, tab requrired but got space." << endl;
		}
		else if(expectedTab != tabCount){
			cout << "Line# " << yylineno << ": warning, " << expectedTab <<
				" of tabs needed but got " << tabCount << " tabs." << endl;
			 
			warningCount++;
		}
	}
	foundNewLine = false;
}
 */

void replaceEscapeChar(const char* chars){

	switch(chars[1]){
		case 't':
			token += '\t';
			break;
		case '\'':
			token += '\'';
			break;
		case '\"':
			token += '\"';
			break;
		case '\\':
			token += '\\';
			break;
		case 'n':
			token += '\n';
			break;
		case 'r':
			token += '\r';
			break;
		case 'a':
			token += '\a';
			break;
		case 'f':
			token += '\f';
			break;
		case 'b':
			token += '\b';
			break;
		case 'v':
			token += '\v';
			break;
		case '0':
			token += '\0';
			break;
	}
}
