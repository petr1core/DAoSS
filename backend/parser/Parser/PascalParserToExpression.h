//
// Created by egorm on 21.04.2024.
//
#ifndef PARSER_H
#define PARSER_H

#include <bits/stdc++.h>

#include <utility>
#include "stdexcept"
#include "../Scripts/Lexer.h"
#include "../Scripts/Token.h"
#include "../Scripts/HierarchyList.h"
#include "../Expression/Expression.h"
#include "../Expression/StatementExpression.h"
#include "../Expression/ConditionExpression.h"
#include "../Expression/CaseOf.h"
#include "../Expression/Function.h"
#include "../Expression/Procedure.h"


class PascalParserToExpression {
private:
    std::string title;
    TPostfixCalc calc;
    std::vector<Token> tokenList;
    std::vector<std::pair<Expression *, string>> expressionList;
    std::vector<Expression *> expressionOnly;
    std::vector<Token> localList;
    int currentPos = 0;
    types type;
public:
    PascalParserToExpression(Lexer lexer, types type) {
        this->tokenList = lexer.getTokenList();
        this->type=type;
    }
    void setType(types t){
        this->type=t;
    }
    // Парсинг без выполнения - только структура
    void parseOnly() {
        initDeclaration();
        // Не вызываем calc.ChangeEquation - только парсим структуру
    }
    
    void parse() {
        initDeclaration();
        vector<vector<Token>> copyIf;
        for (const auto& item: expressionList) {
            if (auto statementExpr = dynamic_cast<StatementExpression *>(item.first)) {
                calc.ChangeEquation(*statementExpr); // Вызов метода для StatementExpression
            } else if (auto conditionExpr = dynamic_cast<ConditionExpression *>(item.first)) {
                if ((*conditionExpr).getCondition().front().getValue() == "if") {
                    copyIf.push_back((*conditionExpr).getCondition());
                }
                if ((*conditionExpr).getCondition().front().getValue() == "else") {
                    vector<Token> newCon;
                    newCon.push_back((*conditionExpr).getCondition().front());
                    for (auto item: copyIf.back()) {
                        newCon.push_back(item);
                    }
                    auto iter = copyIf.end();
                    copyIf.erase(iter);
                    (*conditionExpr).setCondition(newCon);
                }
                calc.ChangeEquation(*conditionExpr); // Вызов метода для ConditionExpression
            } else if (auto CaseExpr = dynamic_cast<CaseOf *>(item.first)) {
                calc.ChangeEquation(*CaseExpr);
            } else { continue; }
        }
//        cout << endl;
//        calc.getTable().root->print();
//        cout << endl;
        //hierarchyList.print(hierarchyList.getRoot()); cout<<endl;
        //this->print();
    }

    [[nodiscard]] const std::vector<std::pair<Expression *, std::string>> &getExpressions() const {
        return expressionList;
    }

    std::vector<Expression *> &getExpressionsOnly() {
        for (const auto &tmp: expressionList)
            expressionOnly.push_back(tmp.first);
        return expressionOnly;
    }

    void setTitle(std::string t) {
        this->title = std::move(t);
    }

    std::string getTitle() { return title; }

    void initDeclaration() {
        if (isTypeToken("TITLE")) {
            setTitle(tokenList[currentPos].getValue());
            while (!isTypeToken("SEMICOLON")) { currentPos++; } //скип названия, чтобы дойти до разделов Const или Var
            currentPos++;
        }
        if (isTypeToken("FUNCTION")) {
            auto *sw = new Function(currentPos, tokenList);
            currentPos = sw->getPos();
            std::pair t{sw, "Var"};
            calc.add(sw);
            expressionList.emplace_back(t);
        }
        if (isTypeToken("PROCEDURE")) {
            auto *sw = new Procedure(currentPos, tokenList);
            currentPos = sw->getPos();
            std::pair t{sw, "Var"};
            calc.add(sw);
            expressionList.emplace_back(t);
        }
        if (isTypeToken("CONST")) {
            currentPos++;
            while (!isTypeToken("VAR")) {
                initRowStatement("Const");
            }
        }
        if (isTypeToken("VAR")) {
            currentPos++;
            while (!isTypeToken("BEGIN")) {
                initRowStatement("Var");
            }
            currentPos++;
        }
        initBegin();
        return;
    }

    void print() {
        for (auto item: expressionList) {
            item.first->print(0);
        }
    }

    void initRowStatement(const string& chapter) {//метод чтобы строчку кода (не условие и не цикл) переводить в StatementExpression
        while (!isTypeToken("SEMICOLON")) {
            localList.push_back(tokenList[currentPos]);
            currentPos++;
        }
        auto *rx = new StatementExpression(localList);
        std::pair t = {rx, chapter};
        expressionList.emplace_back(t);
        localList.clear();
        currentPos++;
        return;
    }

    void initBegin() {
        std::vector<Token> condition;
        while (!isTypeToken("ENDofPROGRAM")) {
            if (isTypeToken("SWITCH")) {
                auto *sw = new CaseOf(currentPos, tokenList);
                currentPos = sw->getPos();
                std::pair t{sw, "Body"};
                expressionList.push_back(t);
                continue;
            }
            if ((isTypeToken("CONDITION")) || (isTypeToken("UNCONDITION")) || (isTypeToken("CYCLEFOR")) ||
                (isTypeToken("CYCLEWHILE")) || (isTypeToken("CYCLEDOWHILE"))) {
                auto *cx = new ConditionExpression(currentPos, tokenList);
                currentPos = cx->getGlobalPos();
                std::pair t = {cx, "Body"};
                expressionList.push_back(t);
                continue;

            } else {
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
