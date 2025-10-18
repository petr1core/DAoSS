//
// Created by egorm on 21.04.2024.
//
#ifndef PARSER_H
#define PARSER_H
#include <bits/stdc++.h>
#include "stdexcept"
#include "Lexer.h"
#include "Token.h"
#include "HierarchyList.h"
#include "Expression/Expression.h"
#include "Expression/StatementExpression.h"
#include "Expression/ConditionExpression.h"
#include "Expression/CaseOf.h"
#include "Expression/Function.h"
#include "Expression/Procedure.h"
using namespace std;
int number=0;
class AgeException: public std::exception
{
public:
    AgeException(const std::string& message): message(message){}
    std::string getMessage() const {std::cout<<message; return message;}
private:
    std::string message;
};
class Parser {
private:
    TPostfixCalc calc;
    HierarchyList<string,Expression*>hierarchyList;
    std::vector<Token> tokenList;
    std::vector<std::pair<Expression*,string>> expressionList;
    std::vector<Token> localList;
    int currentPos = 0;
public:
    Parser(Lexer lexer) {
        this->tokenList = lexer.getTokenList();
    }
    Parser(vector<Token> t){this->tokenList=t;}
    void parse() {
        initDeclaration();
        tohierarchy();
        vector<vector<Token>> copyIf;
//        for(auto item: expressionList){
//            cout<<(item.first)<<endl;
//        }
        for(auto item:expressionList){
            if (auto statementExpr = dynamic_cast<StatementExpression*>(item.first))
            {
                calc.ChangeEquation(*statementExpr); // Вызов метода для StatementExpression
            }
            else if (auto conditionExpr = dynamic_cast<ConditionExpression*>(item.first))
            {
                if((*conditionExpr).getCondition().front().getValue()=="if"){
                    copyIf.push_back((*conditionExpr).getCondition());}
                if((*conditionExpr).getCondition().front().getValue()=="else"){
                    vector<Token>newCon;
                    newCon.push_back((*conditionExpr).getCondition().front());
                    for(auto item:copyIf.back()){
                        newCon.push_back(item);
                    }
                    auto iter=copyIf.end();
                    copyIf.erase(iter);
                    (*conditionExpr).setCondition(newCon);}
                calc.ChangeEquation(*conditionExpr); // Вызов метода для ConditionExpression
            }
            else if  (auto CaseExpr = dynamic_cast<CaseOf*>(item.first)) {
                calc.ChangeEquation(*CaseExpr);
            }
            else{ continue;}
        } cout<<endl;
        //calc.getTable().root->print();  cout<<endl;
        //hierarchyList.print(hierarchyList.getRoot()); cout<<endl;
        //this->print();
    }
    void tohierarchy(){
        for( const auto& item:expressionList){
            hierarchyList.toAddNext(item.first,item.second);
        }
    }
    void initDeclaration(){
        if(isTypeToken("TITLE")){
            while(!isTypeToken("SEMICOLON"))
            {currentPos++;} //скип названия, чтобы дойти до разделов Const или Var
            currentPos++;
        }       //есть вариант вообще название не добавлять уже на стадии Lexer, но мб пригодится
        if(isTypeToken("FUNCTION")){
            Function* sw= new Function(currentPos,tokenList);
            currentPos=sw->getPos();
            std::pair t{sw,"Var"};
            calc.add(sw);
            expressionList.push_back(t);
        }
        if(isTypeToken("PROCEDURE")){
            Procedure* sw= new Procedure(currentPos,tokenList);
            currentPos=sw->getPos();
            std::pair t{sw,"Var"};
            calc.add(sw);
            expressionList.push_back(t);
        }
        if(isTypeToken("CONST")){
            currentPos++;
            while (!isTypeToken("VAR")){
                initRowStatement("Const");
            }
        }
        if(isTypeToken("VAR")){
            currentPos++;
            while (!isTypeToken("BEGIN")){
                initRowStatement("Var");
            }
            currentPos++;
        }
        initBegin();
        return;
    }
    void print(){
        for(auto item:expressionList)
        {    item.first->print(number);      }
    }
    void initRowStatement(string chapter){//метод чтобы строчку кода (не условие и не цикл) переводить в StatementExpression
        while(!isTypeToken("SEMICOLON")){
            localList.push_back( tokenList[currentPos]);
            currentPos++; }
        StatementExpression* rx=new StatementExpression(localList);
        std::pair t={rx,chapter};
        expressionList.push_back(t);
        localList.clear();
        currentPos++;
        return;
    }
    void initBegin() {
        std::vector<Token> condition;
        while (!isTypeToken("ENDofPROGRAM")) {
            if(isTypeToken("SWITCH"))
            {
                CaseOf* sw= new CaseOf(currentPos,tokenList);
                currentPos=sw->getPos();
                std::pair t{sw,"Body"};
                expressionList.push_back(t);
                continue;
            }
            if ((isTypeToken("CONDITION"))||(isTypeToken("UNCONDITION"))||(isTypeToken("CYCLEFOR"))||
                (isTypeToken("CYCLEWHILE"))||(isTypeToken("CYCLEDOWHILE")))
            {
                ConditionExpression *cx = new ConditionExpression(currentPos, tokenList);
                currentPos = cx->getGlobalPos();
                std::pair t={cx,"Body"};
                expressionList.push_back(t);
                continue;

            }
            else
            {
                initRowStatement("Body");
            }
        }
        return;
    }
    bool isTypeToken(const string &typeToken) {
        return tokenList[currentPos].getType() == typeToken;
    }
};
#endif //PARSER_H
