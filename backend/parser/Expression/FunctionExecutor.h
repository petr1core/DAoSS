//
// Created by shuri on 11.05.2024.
//
#ifndef FUNCTIONALLOCATOR_H
#define FUNCTIONALLOCATOR_H
#include <utility>
#include <vector>
#include "../Token.h"
#include "Expression.h"
#include "Function.h"
#include "Procedure.h"
class FunctionExecutor: public Expression{
private:
    vector<Expression*> expressionList;
    string ans;
public:
    FunctionExecutor(vector<Token> argue, vector<Token> head, SearchTreeTable<string,string>t){
        ParseFunction(argue, head, t);
    }
    void ParseFunction(vector<Token> argue, vector<Token> head,SearchTreeTable<string,string>t){
        Token returnType = head[head.size()-1];
        int i=0;
        int count =0; int interval=0;
        vector<Token>lists;
        while (head[i].getValue()!="("){ i++; } i++;
        while(head[i].getValue()!=")"){
            if(head[i].getType()=="TYPEINTEGER"||head[i].getType()=="TYPEREAL"
            ||head[i].getType()=="TYPESTRING"||head[i].getType()=="TYPECHAR"){
                while(count!=(static_cast<int>(lists.size()-1))){
                    t.Change(lists[count].getValue(),argue[count+interval].getValue(),head[i].getType());
                    count++;
                }
                interval+=count;
                lists.clear();
                count=0;
            }
            if(head[i].getType()=="VARIABLE"){
                lists.push_back(head[i]);
            }
            i++;
        }
    }
    void print(int tab) override{

    }
};


#endif //FUNCTIONALLOCATOR_H
