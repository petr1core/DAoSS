//
// Created by shuri on 11.05.2024.
//

#ifndef CASEOF_H
#define CASEOF_H
#include <vector>
#include "Expression.h"
#include "../Token.h"
static int c=0;
class CaseOf: public Expression {
private:
    int c1;
    Token value;
    vector<std::pair<vector<Token>,vector<Expression*>>> body;
    int globalPosCase;
public:
    CaseOf(){
        c1=++c;
        value=Token();};
    CaseOf(int pos, vector<Token>list){
        c1=++c;
        doSwitch(pos,list);
    }
    Token getVal(){return value;}
    vector<std::pair<vector<Token>,vector<Expression*>>> getBody(){return body;}
    void doSwitch(int pos, vector<Token>list){
        globalPosCase=pos;
        globalPosCase++;
        value=list[globalPosCase];
        if(value.getType()=="VALUESTRING"||value.getType()=="VALUEREAL"){
            throw std::runtime_error{"invalid argument for switch"};}
        globalPosCase++;
        globalPosCase++;
        globalPosCase++;
        vector<Expression*>expressionList; //для хранения действий при определённом кейсе
        vector<Token>localList;
        vector<Token>argueList;//условия вхождения в кейс
        while(list[globalPosCase].getType()!="ENDofCycle")
        {
            while(list[globalPosCase].getType()!="COLON")
            {
                if(list[globalPosCase].getType()!="COMMA")
                {
                    argueList.push_back(list[globalPosCase]);
                }
                globalPosCase++;
            }
            globalPosCase++;
            globalPosCase++;
            while(list[globalPosCase].getType()!="ENDofCycle"){
                if(((list[globalPosCase].getType()=="CONDITION"))||
                   (list[globalPosCase].getType()=="CYCLEFOR")||
                   (list[globalPosCase].getType()=="CYCLEWHILE")||
                   (list[globalPosCase].getType()=="CYCLEDOWHILE"))
                {
                    auto* cx =new ConditionExpression(globalPosCase,list);
                    globalPosCase=ConditionExpression::getGlobalPos();
                    expressionList.push_back(cx);
                }
                else{
                    while(list[globalPosCase].getType()!="SEMICOLON"){
                        localList.push_back(list[globalPosCase]);
                        globalPosCase++;
                    }
                    auto* rx= new StatementExpression(localList);
                    expressionList.push_back(rx);
                    localList.clear();
                    globalPosCase++;
                }
            }
            std::pair t{argueList,expressionList};
            body.push_back(t);
            expressionList.clear();
            argueList.clear();
            globalPosCase++;
        }
        globalPosCase++;
    }
    void print(int tab)override{
        for(int j=0;j<tab;j++){
            cout<<"   ";
        }
        std::cout<<"CaseOf "<<c1<<" of "<<value.getValue()<<" =";
        std::cout<<endl;

        if(!body.empty())
        {
            ++tab;
            for(auto token2:body)
            {
                for(int j=0;j<tab;j++){
                    std::cout<<"   ";
                }
                std::cout<<"in case:";
                for(auto token3:token2.first){
                    std::cout<<" "<< token3.getValue();
                }
                std::cout<<std::endl;
                for(int j=0;j<tab;j++){
                    std::cout<<"   ";
                }
                std::cout<<"corresponds to:";
                for(auto token3:token2.second){
                    std::cout<<"   ";
                    token3->print(tab);
                }
                std::cout<<std::endl;
            }
        }
        --tab;
    }
    int getPos(){return globalPosCase;}
};


#endif //CASEOF_H
