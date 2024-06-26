#ifndef FUNCPARAMS_H
#define FUNCPARAMS_H

#include <iostream>
using namespace std;

class SymbolInfo;

class SymbolInfoList{

public:
    SymbolInfo* param;
    SymbolInfoList* nextParam;

    SymbolInfoList(SymbolInfo* head){
        param = head;
        nextParam = nullptr;
    }
};

class FunctionParams {

private:
    int paramNum;
    SymbolInfoList* paramList;
    SymbolInfoList* currParam;

public:
    FunctionParams(){
        paramNum = 0;
        paramList = currParam = nullptr;
    }

    void add(SymbolInfo* info){
        if(paramList == nullptr){
            paramList = currParam = new SymbolInfoList(info);
        }
            
        else {
            SymbolInfoList* temp = paramList;

            while(temp->nextParam != nullptr)
                temp = temp->nextParam;

            temp->nextParam = new SymbolInfoList(info);
        }

        paramNum++;
    }

    int getParamNum(){
        return paramNum;
    }

    void moveToHead(){
        currParam = paramList;
    }

    SymbolInfo* nextParam(){
        if(lastParam())
            return nullptr;

        SymbolInfo* current = currParam->param;
        currParam = currParam->nextParam;
        return current;
    }

    bool lastParam(){
        return currParam == nullptr;
    }
    
};


#endif