//
// Created by egorm on 21.04.2024.
//
#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include <utility>
#include <stdexcept>
#include <iostream>
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
    std::vector<std::pair<Expression *, std::string>> expressionList;
    std::vector<Expression *> expressionOnly;
    std::vector<Token> localList;
    int currentPos = 0;
    LangType type;
public:
    PascalParserToExpression(Lexer lexer, LangType type) {
        this->tokenList = lexer.getTokenList();
        this->type=type;
    }
    void setType(LangType t){
        this->type=t;
    }
    // Парсинг без выполнения - только структура
    void parseOnly() {
        try {
            initDeclaration();
        } catch (const std::exception& e) {
            std::cerr << "[PARSER] Exception in initDeclaration: " << e.what() << std::endl;
            throw;
        }
        // Не вызываем calc.ChangeEquation - только парсим структуру
    }
    
    void parse() {
        initDeclaration();
        std::vector<std::vector<Token>> copyIf;
        for (const auto& item: expressionList) {
            if (auto statementExpr = dynamic_cast<StatementExpression *>(item.first)) {
                calc.ChangeEquation(*statementExpr); // Вызов метода для StatementExpression
            } else if (auto conditionExpr = dynamic_cast<ConditionExpression *>(item.first)) {
                if ((*conditionExpr).getCondition().front().getValue() == "if") {
                    copyIf.push_back((*conditionExpr).getCondition());
                }
                if ((*conditionExpr).getCondition().front().getValue() == "else") {
                    std::vector<Token> newCon;
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
            // Извлекаем только имя программы из токена "program <name>"
            std::string titleValue = tokenList[currentPos].getValue();
            // Убираем "program " в начале, оставляем только имя
            if (titleValue.length() > 8 && titleValue.substr(0, 8) == "program ") {
                setTitle(titleValue.substr(8)); // Пропускаем "program "
            } else {
                setTitle(titleValue); // Fallback: если формат неожиданный
            }
            // Пропускаем токен TITLE и точку с запятой
            currentPos++; // Пропускаем токен TITLE
            if (isTypeToken("SEMICOLON")) {
                currentPos++; // Пропускаем SEMICOLON после названия программы
            }
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
            while (!isTypeToken("VAR") && !isTypeToken("BEGIN") && !isTypeToken("ENDofPROGRAM")) {
                initRowStatement("Const");
                if (currentPos >= tokenList.size()) break;
            }
        }
        if (isTypeToken("VAR")) {
            currentPos++;
            while (!isTypeToken("BEGIN") && !isTypeToken("ENDofPROGRAM")) {
                initRowStatement("Var");
                if (currentPos >= tokenList.size()) break;
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

    void initRowStatement(const std::string& chapter) {//метод чтобы строчку кода (не условие и не цикл) переводить в StatementExpression
        // Проверяем, не пустое ли тело (сразу END)
        if (isTypeToken("ENDofPROGRAM")) {
            return;
        }
        
        while (!isTypeToken("SEMICOLON") && !isTypeToken("ENDofPROGRAM")) {
            if (currentPos >= tokenList.size()) {
                throw std::runtime_error("Reached end of tokens without finding SEMICOLON or ENDofPROGRAM");
            }
            localList.push_back(tokenList[currentPos]);
            currentPos++;
        }
        
        // Если localList пустой (не было операторов), не создаем StatementExpression
        if (!localList.empty()) {
            auto *rx = new StatementExpression(localList);
            std::pair t = {rx, chapter};
            expressionList.emplace_back(t);
        }
        
        localList.clear();
        
        // Пропускаем SEMICOLON, если он есть
        if (isTypeToken("SEMICOLON")) {
            currentPos++;
        }
        
        return;
    }

    void initBegin() {
        std::vector<Token> condition;
        int iterations = 0;
        const int MAX_ITERATIONS = 100; // Уменьшено с 1000 для быстрого обнаружения проблем
        
        while (!isTypeToken("ENDofPROGRAM")) {
            // Ранняя проверка: если вышли за границы токенов, сразу выбрасываем ошибку
            if (currentPos >= tokenList.size()) {
                throw std::runtime_error("Reached end of tokens without finding ENDofPROGRAM");
            }
            
            iterations++;
            if (iterations > MAX_ITERATIONS) {
                throw std::runtime_error("Too many iterations in initBegin loop - possible infinite loop or invalid syntax");
            }
            
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

    bool isTypeToken(const std::string &typeToken) {
        if (currentPos >= tokenList.size()) {
            return false;
        }
        return tokenList[currentPos].getType() == typeToken;
    }
};

#endif //PARSER_H
