//
// Created by shuri on 26.04.2024.
//
#ifndef CONDITIONEXPRESSION_H
#define CONDITIONEXPRESSION_H
#include <vector>
#include "Expression.h"
#include <bits/stdc++.h>
#include "stdexcept"
#include "../Token.h"
static int y=0;
static int posofEndofIf=0; //только для вложенных случаев нужен
class ConditionExpression: public Expression{
private:
    std::vector<Expression*> expressionList;
    std::vector<Token> condition;
    std::vector<Token> localList;
public:
    static int getGlobalPos(){return posofEndofIf;}
    ConditionExpression(int pos, vector<Token>list){
        doCondition(pos,list);
    }
    ConditionExpression( const ConditionExpression& ex){
        this->condition=ex.condition;
        this->expressionList=ex.expressionList;
    }
    vector<Token> getCondition(){return condition;}
    void setCondition(vector<Token> con){this->condition=con;}
    std::pair<vector<Token>, vector<Expression*>> getBody() {
        std::pair<vector<Token>, vector<Expression*>> condAndList(condition, expressionList);
        return condAndList;
    }
    void doCondition(int pos, vector<Token>list){
        posofEndofIf=pos;
        if(list[posofEndofIf].getType()=="CONDITION")
        {
            while(list[posofEndofIf].getType()!="BEGIN")
            {
                condition.push_back(list[posofEndofIf]);
                posofEndofIf++;
            }
            posofEndofIf++;
            while(list[posofEndofIf].getType()!="ENDofIF"){ //пока не дойдём до конца тела текущего if
                if(((list[posofEndofIf].getType()=="CONDITION"))|| //если хоть какую-то в нем вложенность находим
                   (list[posofEndofIf].getType()=="CYCLEFOR")|| // хоть вложенное условие, хоть вложенный цикл, то создаём новый объект
                   (list[posofEndofIf].getType()=="CYCLEWHILE")|| // не забываем про static переменную, она указывает новое место где мы окажемся
                   (list[posofEndofIf].getType()=="CYCLEDOWHILE"))//поднявшись обратно наверх от вложенного объекта
                {
                        ConditionExpression* cx = new ConditionExpression(posofEndofIf,list);
                        posofEndofIf=cx->getGlobalPos();
                        expressionList.push_back(cx);
                }
                else
                {
                    while(list[posofEndofIf].getType()!="SEMICOLON"){ //если вложенности нет или мы с ней уже закончили, то формируем обычные выражения
                        localList.push_back(list[posofEndofIf]);
                        posofEndofIf++;}
                    StatementExpression* rx= new StatementExpression(localList);
                    expressionList.push_back(rx);
                    localList.clear();
                    posofEndofIf++;
                }
            }
            posofEndofIf++;
            return;
            //после ENDofIF перескакивать не надо, чтобы было разделение на отдельные объекты у if и else
        }
        if(list[posofEndofIf].getType()=="UNCONDITION"){
            condition.push_back(list[posofEndofIf]);
            while(list[posofEndofIf].getType()!="BEGIN")
            { posofEndofIf++;} posofEndofIf++;
            // те же шаги, что и при condition, но уже в цикле до ENDofCycle
            while(list[posofEndofIf].getType()!="ENDofCycle"){
                if(((list[posofEndofIf].getType()=="CONDITION"))||
                   (list[posofEndofIf].getType()=="CYCLEFOR")||
                   (list[posofEndofIf].getType()=="CYCLEWHILE")||
                   (list[posofEndofIf].getType()=="CYCLEDOWHILE"))
                    {
                    ConditionExpression* cx =new ConditionExpression(posofEndofIf,list);
                    posofEndofIf=cx->getGlobalPos();
                    expressionList.push_back(cx);}
               else{
                   while(list[posofEndofIf].getType()!="SEMICOLON")
                   {
                       localList.push_back(list[posofEndofIf]);
                       posofEndofIf++;
                   }
                   StatementExpression* rx= new StatementExpression(localList);
                   expressionList.push_back(rx);
                   localList.clear();
                   posofEndofIf++;
               }
            }
            posofEndofIf++;
            return;
        }
        if((list[posofEndofIf].getType()=="CYCLEFOR")||(list[posofEndofIf].getType()=="CYCLEWHILE")){
            while(list[posofEndofIf].getType()!="BEGIN"){
                condition.push_back(list[posofEndofIf]);
                posofEndofIf++;  }
            posofEndofIf++;
            // те же шаги, что и при uncondition
            while(list[posofEndofIf].getType()!="ENDofCycle"){
                if((list[posofEndofIf].getType()=="CONDITION")||
                   (list[posofEndofIf].getType()=="CYCLEFOR")||
                   (list[posofEndofIf].getType()=="CYCLEWHILE")||
                   (list[posofEndofIf].getType()=="CYCLEDOWHILE")){
                    ConditionExpression* cx= new ConditionExpression(posofEndofIf,list);
                    posofEndofIf=cx->getGlobalPos();
                    expressionList.push_back(cx);}
               else{
                   while(list[posofEndofIf].getType()!="SEMICOLON"){
                       localList.push_back(list[posofEndofIf]);
                       posofEndofIf++;}
                   StatementExpression* rx=new StatementExpression(localList);
                   expressionList.push_back(rx);
                   localList.clear();
                   posofEndofIf++;
               }
            }
            posofEndofIf++;
            return;

       }
        if(list[posofEndofIf].getType()=="CYCLEDOWHILE"){
            posofEndofIf++;
            posofEndofIf++;
            // те же шаги, что и при uncondition
            while(list[posofEndofIf].getType()!="ENDofCycle"){
                if((list[posofEndofIf].getType()=="CONDITION")||
                   (list[posofEndofIf].getType()=="CYCLEFOR")||
                   (list[posofEndofIf].getType()=="CYCLEWHILE")||
                   (list[posofEndofIf].getType()=="CYCLEDOWHILE")){
                    ConditionExpression* cx= new ConditionExpression(posofEndofIf,list);
                    posofEndofIf=cx->getGlobalPos();
                    expressionList.push_back(cx);}
               else{
                   while(list[posofEndofIf].getType()!="SEMICOLON"){
                       localList.push_back(list[posofEndofIf]);
                       posofEndofIf++;}
                   StatementExpression* rx=new StatementExpression(localList);
                   expressionList.push_back(rx);
                   localList.clear();
                   posofEndofIf++;
               }
            }
            posofEndofIf++;
            while(list[posofEndofIf].getType()!="SEMICOLON")
            {
                condition.push_back(list[posofEndofIf]);
                posofEndofIf++;
            }
            posofEndofIf++;
            return;
        }
    }
    ConditionExpression& operator=(const ConditionExpression& other) {
        this->condition = other.condition;
        this->expressionList = other.expressionList;
        return *this;
    }
    void print(int tab) override{
        for(int j=0;j<tab;j++){
            cout<<"   ";
        }
       std::cout<<"ConditionExpression "<<++y<<" = ";
       if(condition[0].getValue()!="else"){
           for(auto token:condition)
           { std::cout<<token.getValue()<<" "; } std::cout<<endl;}
       else{std::cout<<"else"<<endl;}
       if(!expressionList.empty())
       {
           ++tab;
           for(auto token2:expressionList)
           {
               std::cout<<"   ";token2->print(tab);
           }
       }
       --tab;
    }
};


#endif //CONDITIONEXPRESSION_H
