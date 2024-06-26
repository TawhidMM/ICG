#ifndef PARSETREE_H
#define PARSETREE_H


#include<iostream>
#include<fstream>
#include"2005036_SymbolInfo.h"

using namespace std;


class ParseTree {

private:
    string node;
    SymbolInfo* symbol;
    int startLine;
    int finishLine;
    string dataType;
    ParseTree* leftChild;
    ParseTree* sibling;
    
public:
    string next = "";
    string trueLabel = "";
    string falseLabel = "";
    
    ParseTree(string node, int startLine, int finishLine){
        this->node = node;
        this->startLine = startLine;
        this->finishLine = finishLine;

        dataType = "";
        leftChild = nullptr;
        sibling = nullptr;
        symbol = nullptr;
    }

    ParseTree(string node, SymbolInfo* symbol, int startLine, int finishLine){
        this->node = node;
        this->symbol = symbol;
        this->startLine = startLine;
        this->finishLine = finishLine;

        dataType = "";
        leftChild = nullptr;
        sibling = nullptr;
        
    }


    string getNode() {
        return node;
    }

    void addLeftChild(ParseTree* child){
        leftChild = child;
    }
    ParseTree* getChild(){
        return leftChild;
    }


    void addSibling(ParseTree* sibling){
        this->sibling = sibling;
    }
    ParseTree* getSibling(){
        return sibling;
    }

    void setDataType(string type){
        dataType = type;
    }
    string getDataType(){
        return dataType;
    }

    void setSymbol(SymbolInfo* symbol){
        this->symbol = symbol;
    }
    SymbolInfo* getSymbol(){
        return symbol;
    }


    void print(ofstream& fout){
        int rootSpace = 0;
        printTree(this, rootSpace, fout);
    }


private:

    void printTree(ParseTree* root, int spaces, ofstream& fout){
        printSpaces(spaces, fout);
        fout << *root << endl;

        ParseTree* child = root->getChild();

        while(child != nullptr){
            printTree(child, spaces + 1, fout);
            child = child->getSibling();
        }
    }

    void printSpaces(int spaces, ofstream& fout){
        for(int i = 0; i < spaces; i++)
            fout << " ";
    }

    friend ostream& operator<<(ostream& os, ParseTree& obj) {
        string node;

        if(obj.node.find(':') == string::npos){
            node = obj.node + string(" : ") + obj.getSymbol()->getName();
        }
        else {
            node = obj.node;
        }

        if(obj.leftChild != nullptr){
            os << node << " \t" << "<Line: " << obj.startLine <<
                "-" << obj.finishLine << ">";
        }
        else {
            os << node << "\t" << "<Line: " << obj.startLine << ">";
        }
            
        return os;
    }

};

#endif



