//
// Created by shuri on 11.05.2024.
//

#ifndef PROCEDURE_H
#define PROCEDURE_H

#include "Expression.h"
#include "../Token.h"
#include "ConditionExpression.h"
#include "StatementExpression.h"
#include <utility>
#include <vector>

static int p = 0;

class Procedure : public Expression {
private:
    int p1;
    Token name;
    std::vector<Expression *> expressionList;
    std::vector<Token> declaration;
    std::vector<Token> localList;
    int globalPosProc;
public:
    Procedure(int pos, vector<Token> list) {
        p1 = ++p;
        doProcedure(pos, list);
    }

    Procedure(const Procedure &ex) {
        p1 = ++p;
        this->declaration = ex.declaration;
        this->expressionList = ex.expressionList;
    }

    void doProcedure(int pos, vector<Token> list) {
        globalPosProc = pos;
        while (list[globalPosProc].getType() != "SEMICOLON") {
            declaration.push_back(list[globalPosProc]);
            globalPosProc++;
        }
        name = declaration[1];
        globalPosProc++;
        globalPosProc++;
        while (list[globalPosProc].getType() != "ENDofCycle") {
            if (((list[globalPosProc].getType() == "CONDITION")) || //если хоть какую-то в нем вложенность находим
                (list[globalPosProc].getType() == "CYCLEFOR") ||
                // хоть вложенное условие, хоть вложенный цикл, то создаём новый объект
                (list[globalPosProc].getType() == "CYCLEWHILE") ||
                // не забываем про static переменную, она указывает новое место где мы окажемся
                (list[globalPosProc].getType() == "CYCLEDOWHILE"))//поднявшись обратно наверх от вложенного объекта
            {
                ConditionExpression *cx = new ConditionExpression(globalPosProc, list);
                globalPosProc = cx->getGlobalPos();
                expressionList.push_back(cx);
            } else {
                while (list[globalPosProc].getType() != "SEMICOLON") { //если вложенности нет или мы с ней уже закончили, то формируем обычные выражения
                    localList.push_back(list[globalPosProc]);
                    globalPosProc++;
                }
                auto *rx = new StatementExpression(localList);
                expressionList.push_back(rx);
                localList.clear();
                globalPosProc++;
            }
        }
        globalPosProc++;
        return;
    }

    vector<Expression *> getBody() { return expressionList; }

    vector<Token> getHead() { return declaration; }

    Token getName() { return name; }

    void print(int tab) override {
        for (int j = 0; j < tab; j++) {
            cout << "   ";
        }
        std::cout << "Procedure " << p1 << " = ";
        for (auto token: declaration) {
            if (token.getValue() != "procedure") {
                std::cout << token.getValue() << " ";
            }
        }
        std::cout << endl;

        if (!expressionList.empty()) {
            ++tab;
            for (auto token2: expressionList) {
                std::cout << "   ";
                token2->print(tab);
            }
        }
        --tab;
    }

    int getPos() { return globalPosProc; }
};


#endif //PROCEDURE_H
