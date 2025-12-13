//
// Created by shuri on 08.05.2024.
//

#ifndef FUNCTION_H
#define FUNCTION_H

#include "Expression.h"
#include "../Scripts/Token.h"
#include "ConditionExpression.h"
#include "StatementExpression.h"
#include <utility>
#include <vector>
#include <iostream>

static int f = 0;

class Function : public Expression {
private:
    int f1;
    Token name;
    std::vector<Expression *> expressionList;
    std::vector<Token> declaration;
    std::vector<Token> localList;
    int globalPosFun;
public:
    Function(int pos, std::vector<Token> list) {
        f1 = ++f;
        globalPosFun = pos;
        doFunction(std::move(list));
    }

    Function(const Function &ex) {
        f1 = ++f;
        this->declaration = ex.declaration;
        this->expressionList = ex.expressionList;
    }

    void doFunction(std::vector<Token> list) {
        while (list[globalPosFun].getType() != "SEMICOLON") {
            declaration.push_back(list[globalPosFun]);
            globalPosFun++;
        }
        name = declaration[1];
        globalPosFun++;
        globalPosFun++;
        while (list[globalPosFun].getType() != "ENDofCycle") {
            if (((list[globalPosFun].getType() == "CONDITION")) || //если хоть какую-то в нем вложенность находим
                (list[globalPosFun].getType() == "CYCLEFOR") ||
                // хоть вложенное условие, хоть вложенный цикл, то создаём новый объект
                (list[globalPosFun].getType() == "CYCLEWHILE") ||
                // не забываем про static переменную, она указывает новое место где мы окажемся
                (list[globalPosFun].getType() == "CYCLEDOWHILE"))//поднявшись обратно наверх от вложенного объекта
            {
                auto *cx = new ConditionExpression(globalPosFun, list);
                globalPosFun = cx->getGlobalPos();
                expressionList.push_back(cx);
            } else {
                while (list[globalPosFun].getType() !=
                       "SEMICOLON") { //если вложенности нет или мы с ней уже закончили, то формируем обычные выражения
                    localList.push_back(list[globalPosFun]);
                    globalPosFun++;
                }
                auto *rx = new StatementExpression(localList);
                expressionList.push_back(rx);
                localList.clear();
                globalPosFun++;
            }
        }
        globalPosFun++;
        return;
    }

    Token getName() { return name; }

    std::vector<Expression *> getBody() { return expressionList; }

    std::vector<Token> getHead() { return declaration; }

    void print(int tab) override {
        for (int j = 0; j < tab; j++) {
            std::cout << "   ";
        }
        std::cout << "Function " << f1 << " = ";
        for (auto token: declaration) {
            if (token.getValue() != "function") {
                std::cout << token.getValue() << " ";
            }
        }
        std::cout << std::endl;

        if (!expressionList.empty()) {
            ++tab;
            for (auto token2: expressionList) {
                std::cout << "   ";
                token2->print(tab);
            }
        }
        --tab;
    }

    int getPos() const { return globalPosFun; }
};


#endif //FUNCTION_H
