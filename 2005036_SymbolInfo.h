#ifndef SYMBOLINFO_H
#define SYMBOLINFO_H


#include<iostream>
#include "2005036_FunctionParams.h"

using namespace std;

class SymbolInfo {
private: 
    string name;
    string type;
    string dataType;
    SymbolInfo* nextSymbolInfo;
    FunctionParams* params;

public:
    int offset = 0;
    string scope = "";
        
    SymbolInfo(string name, string type){
        this->name = name;
        this->type = type;
        this-> nextSymbolInfo = NULL;
        this->dataType = "";
        this->params = nullptr;
    }

    SymbolInfo(string name, string type, string dataType){
        this->name = name;
        this->type = type;
        this-> nextSymbolInfo = NULL;
        this->dataType = dataType;
        this->params = nullptr;
    }

    string getName(){
        return name;
    }

    void setType(string type){
        this->type = type; 
    }
    string getType(){
        return type;
    }

    void setDataType(string dataType){
        this->dataType = dataType;
    }
    string getDataType(){
        return dataType;
    }

    SymbolInfo* getNext(){
        return nextSymbolInfo;
    }
    void setNext(SymbolInfo* nextSymbolInfo){
        this->nextSymbolInfo = nextSymbolInfo;
    }


    void setFunctionParam(FunctionParams* params){
        this->params = params;
    }
    FunctionParams* getFunctionParams(){
        return params;
    }
    




    friend ostream& operator<<(ostream& os, const SymbolInfo& obj) {
        string symbolType = obj.dataType;
        if(obj.type == "ARRAY")
            symbolType = "ARRAY";

        os << "<" << obj.name << ",";
        if(obj.type == "FUNCTION")
            os << obj.type << "," ;
        os << symbolType << ">";

        return os;
    }
};


#endif
