//
// Created by shuri on 11.05.2024.
//

#ifndef CASEOF_H
#define CASEOF_H
#include <vector>
#include "Expression.h"
#include "../Token.h"
class CaseOf: public Expression {
private:
    Token value;
    vector<std::pair<vector<Token>,vector<Expression*>>> body;
    int globalPosCase;
public:
    CaseOf(){value=Token();};
    CaseOf(int pos, vector<Token>list){
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
                    ConditionExpression* cx =new ConditionExpression(globalPosCase,list);
                    globalPosCase=cx->getGlobalPos();
                    expressionList.push_back(cx);
                }
                else{
                    while(list[globalPosCase].getType()!="SEMICOLON"){
                        localList.push_back(list[globalPosCase]);
                        globalPosCase++;
                    }
                    StatementExpression* rx= new StatementExpression(localList);
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
    void print(int tab)override{}
    int getPos(){return globalPosCase;}
};


#endif //CASEOF_H
