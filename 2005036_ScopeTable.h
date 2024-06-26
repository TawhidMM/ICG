#ifndef SCOPETABLE_H
#define SCOPETABLE_H

#include <iostream>
#include <string>
#include <fstream>
#include "2005036_SymbolInfo.h"

using namespace std;

class ScopeTable{
private:
    int bucketNum;
    string id;
    SymbolInfo** hashTable;
    ScopeTable* parentScope;
    int childScopeNum;
    
public:
    ScopeTable(int bucketNum, ScopeTable* parentScope){
        this->bucketNum = bucketNum;
        this->parentScope = parentScope;
        
        hashTable = new SymbolInfo*[bucketNum];
        for(int i = 0; i < bucketNum; i++) 
            hashTable[i] = NULL;

        childScopeNum = 0;

        if(parentScope == NULL)
            id = "1";
        else{
            int scopeNum = parentScope->getChildScopeNum() + 1;
            parentScope->increaseChildScopeNum();

            id = parentScope->getId() + "." + to_string(scopeNum);
        }

    }

    ~ScopeTable(){
        /* delete all infos */
        /* for(int i = 0; i < bucketNum; i++){
            SymbolInfo* head = hashTable[i];
            SymbolInfo* temp;

            while(head != NULL){
                temp = head;
                head = head->getNext();

                delete temp;
            }
        } */
        /* delete the main array */
        delete[] hashTable;

    }

    bool insert(string name, string type){
        int position;
       
        if(lookUpInfo(name, position) == NULL) {
            SymbolInfo* newSymbolInfo = new SymbolInfo(name, type);
            int index = hashIndex(name);
            
            /* insert at intermediate position */
            if(hashTable[index] == NULL){
                hashTable[index] = newSymbolInfo;
                position = 1;
            }
            /* insert at head */
            else{
                SymbolInfo* prevSymbolInfo = lastSymbolInfo(index, position);
                prevSymbolInfo->setNext(newSymbolInfo);
                position++;
            }
            
            return true;
        } 
        else{
            return false; 
        }
            
    }

    bool insert(SymbolInfo* symbolInfo){
        int position;
        string name = symbolInfo->getName();
        
        if(lookUpInfo(name, position) == NULL) {
            int index = hashIndex(name);
            
            /* insert at intermediate position */
            if(hashTable[index] == NULL){
                hashTable[index] = symbolInfo;
                position = 1;
            }
            /* insert at head */
            else{
                SymbolInfo* prevSymbolInfo = lastSymbolInfo(index, position);
                prevSymbolInfo->setNext(symbolInfo);
                position++;
            }
            
            return true;
        } 
        else{
            
            return false; 
        }
            
    }

    SymbolInfo* lookUp(string name){
        int position;
        int index = hashIndex(name);

        SymbolInfo* foundInfo = lookUpInfo(name, position);
            
        return foundInfo;
    }

    bool deleteSymbol(string name){
        int index = hashIndex(name);
        int position;
        SymbolInfo* head = hashTable[index];
        bool deleted;

        /* bucket empty */
        if(head == NULL)         
            deleted = false;
        /* matches with head */
        else if(head->getName() == name) {
            hashTable[index] = head->getNext();
            position = 1;
            delete head;

            deleted = true;
        } 
        /* search internal infos */
        else {                     
            SymbolInfo* prevInfo = prevSymbolInfo(name, head, position);

            /* not found */
            if(prevInfo == NULL)
                deleted = false;
            /* found */
            else {
                SymbolInfo* matchedInfo = prevInfo->getNext();
                position++;

                prevInfo->setNext(matchedInfo->getNext());
                delete matchedInfo;

                deleted = true;
            }      
        } 

        return deleted;

    }

    void print(){
        cout << "\t" << "ScopeTable# " << id << endl;

        for(int i = 0; i < bucketNum; i++){
            if(hashTable[i] != NULL){
                cout << "\t" << (i + 1);
                printChain(hashTable[i]);
                cout << endl; 
            }  
        }
    }

    string getId(){
        return id;
    }
    
    ScopeTable* getParentScope(){
        return parentScope;
    }

    int getChildScopeNum(){
        return childScopeNum;
    }
    void increaseChildScopeNum(){
        childScopeNum++;
    }


private:
    int hashIndex(string str){
        return sdbmHash(str.c_str()) % bucketNum;
    }

    SymbolInfo* lookUpInfo(string name, int& position){
        int index = hashIndex(name);
        position = -1;

        SymbolInfo* head = hashTable[index];
        SymbolInfo* foundInfo;
        
        /* bucket empty */
        if(head == NULL)  
            return NULL; 
        /* matches with head */
        else if(head->getName() == name){
            position = 1;
            return head;
        }     
        /* search internal infos */
        else {                
            SymbolInfo* prevInfo = prevSymbolInfo(name, head, position);
            
            /* not found */
            if(prevInfo == NULL)
                return NULL;
            /* found */
            else{
                position++;

                return prevInfo->getNext();
            }
        }
    }

    SymbolInfo* prevSymbolInfo(string name, SymbolInfo* head, int& position){
        SymbolInfo* prev;
        position = 0;

        do{
            prev = head;
            head = head->getNext();
            position++;

        } while (head != NULL && head->getName() != name);
        
        if(head == NULL) { // end of LL
            position = -1;

            return NULL;
        }      
        else
            return prev;
    }

    
    SymbolInfo* lastSymbolInfo(int index, int& position){
        SymbolInfo* currSymbolInfo = hashTable[index];
        position = 1;

        while(currSymbolInfo->getNext() != NULL){
            currSymbolInfo = currSymbolInfo->getNext();
            position++;
        }
            
        return currSymbolInfo;
    }

    void printChain(SymbolInfo* head){
        cout << "--> ";

        while(head != NULL) {
            cout << *head << " ";
            head = head->getNext();
        }
    }
    
    unsigned long long sdbmHash(const char* str){
        unsigned long long hash = 0;
        int c;

        while (c = *str++)
            hash = c + (hash << 6) + (hash << 16) - hash;

        return hash;
    }

};

#endif