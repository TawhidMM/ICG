#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include <iostream>
#include <string>
#include <fstream>
#include "2005036_ScopeTable.h"
#include "2005036_SymbolInfo.h"

using namespace std;

class SymbolTable{
private:
    ScopeTable* currScopeTable;
    int bucketNum;

public:
    SymbolTable(int bucketNum){
        this->bucketNum = bucketNum;
        currScopeTable = new ScopeTable(bucketNum, NULL);
    }

    ~SymbolTable(){
        /* delete all but main scope */
        while(currScopeTable->getParentScope()){
            exitScope();
        }

        /* delete main scope */
        delete currScopeTable;
    }

    void enterScope(){
        currScopeTable = new ScopeTable(bucketNum, currScopeTable);
    }

    void exitScope(){
        if(currScopeTable->getParentScope()){
            ScopeTable* prev = currScopeTable;
            currScopeTable = currScopeTable->getParentScope();

            delete prev;
        } 

    }

    bool insert(string name, string type){
        return currScopeTable->insert(name, type);
    }

    bool insert(SymbolInfo* symbolInfo){
        return currScopeTable->insert(symbolInfo);
    }

    bool remove(string name){
        if(!currScopeTable->deleteSymbol(name)){
            return false;
        }
        else
            return true;
    }

    SymbolInfo* lookup(string name){
        ScopeTable* parent = currScopeTable;
        SymbolInfo* foundInfo = NULL;

        while(parent != NULL && foundInfo == NULL){
            foundInfo = parent->lookUp(name);
            
            parent = parent->getParentScope();
        }

        return foundInfo;
    }

    void printCurrScopeTable(){
        currScopeTable->print();
    }

    void printAllScopeTables(){
        ScopeTable* parent = currScopeTable;
        
        while(parent != NULL){
            parent->print();
            parent = parent->getParentScope();
        }

    }


};

#endif