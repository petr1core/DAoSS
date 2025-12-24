//
// Created by egorm on 21.04.2024.
//
#ifndef PARSER_H
#define PARSER_H


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
    LangType type;

public:
    PascalParserToExpression(Lexer lexer, LangType type)
    {
        this->tokenList = lexer.getTokenList();
        this->type=type;
    }
    void setType(LangType t){
        this->type=t;
    }
    // Парсинг без выполнения - только структура
    void parseOnly() {
        std::cout << "[HYPOTHESIS 3] parseOnly: Starting, tokenList size: " << tokenList.size() << std::endl;
        try {
            initDeclaration();
            std::cout << "[HYPOTHESIS 3] parseOnly: initDeclaration completed successfully" << std::endl;
        } catch (const std::bad_alloc& e) {
            std::cerr << "[HYPOTHESIS 3] parseOnly: bad_alloc in initDeclaration: " << e.what() << std::endl;
            throw;
        } catch (const std::exception& e) {
            std::cerr << "[HYPOTHESIS 3] parseOnly: exception in initDeclaration: " << e.what() << std::endl;
            throw;
        }
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
        std::cout << "[HYPOTHESIS 3] initDeclaration: Starting, currentPos=" << currentPos 
                  << ", tokenList.size()=" << tokenList.size() << std::endl;
        
        if (isTypeToken("TITLE")) {
            std::cout << "[HYPOTHESIS 3] initDeclaration: Processing TITLE" << std::endl;
            setTitle(tokenList[currentPos].getValue());
            while (!isTypeToken("SEMICOLON")) { currentPos++; } //скип названия, чтобы дойти до разделов Const или Var
            currentPos++;
        }
        
        // ИСПРАВЛЕНО: обрабатываем все функции/процедуры в цикле до CONST/VAR/BEGIN
        // В Pascal функции могут идти ДО секций const/var
        std::cout << "[HYPOTHESIS 3] initDeclaration: Starting function/procedure loop" << std::endl;
        int funcCount = 0;
        while (currentPos < tokenList.size() && 
               (isTypeToken("FUNCTION") || isTypeToken("PROCEDURE"))) {
            funcCount++;
            std::cout << "[HYPOTHESIS 3] initDeclaration: Processing function/procedure #" << funcCount 
                      << " at pos=" << currentPos << std::endl;
            
            try {
                if (isTypeToken("FUNCTION")) {
                    std::cout << "[HYPOTHESIS 3] initDeclaration: Creating Function object" << std::endl;
                    auto *sw = new Function(currentPos, tokenList);
                    currentPos = sw->getPos();
                    std::pair t{sw, "Var"};
                    calc.add(sw);
                    expressionList.emplace_back(t);
                    std::cout << "[HYPOTHESIS 3] initDeclaration: Function created, new pos=" << currentPos << std::endl;
                } else if (isTypeToken("PROCEDURE")) {
                    std::cout << "[HYPOTHESIS 3] initDeclaration: Creating Procedure object" << std::endl;
                    auto *sw = new Procedure(currentPos, tokenList);
                    currentPos = sw->getPos();
                    std::pair t{sw, "Var"};
                    calc.add(sw);
                    expressionList.emplace_back(t);
                    std::cout << "[HYPOTHESIS 3] initDeclaration: Procedure created, new pos=" << currentPos << std::endl;
                }
            } catch (const std::bad_alloc& e) {
                std::cerr << "[ERROR] [HYPOTHESIS 3] initDeclaration: bad_alloc at function/procedure #" << funcCount 
                          << ", pos=" << currentPos << ": " << e.what() << std::endl;
                throw;
            } catch (const std::exception& e) {
                std::cerr << "[ERROR] [HYPOTHESIS 3] initDeclaration: exception at function/procedure #" << funcCount 
                          << ", pos=" << currentPos << ": " << e.what() << std::endl;
                throw;
            }
        }
        
        std::cout << "[HYPOTHESIS 3] initDeclaration: Functions/procedures processed: " << funcCount << std::endl;
        std::cout << "[HYPOTHESIS 3] initDeclaration: Processing CONST section, currentPos=" << currentPos << std::endl;
        if (isTypeToken("CONST")) {
            std::cout << "[HYPOTHESIS 3] initDeclaration: Processing CONST section" << std::endl;
            currentPos++;
            int constCount = 0;
            while (currentPos < tokenList.size() && !isTypeToken("VAR") && !isTypeToken("BEGIN")) {
                constCount++;
                std::cout << "[HYPOTHESIS 3] initDeclaration: Processing CONST statement #" << constCount 
                          << " at pos=" << currentPos << std::endl;
                try {
                    initRowStatement("Const");
                } catch (const std::bad_alloc& e) {
                    std::cerr << "[ERROR] [HYPOTHESIS 3] initDeclaration: bad_alloc in CONST statement #" << constCount 
                              << ": " << e.what() << std::endl;
                    throw;
                }
            }
            std::cout << "[HYPOTHESIS 3] initDeclaration: CONST statements processed: " << constCount << std::endl;
        }
        if (isTypeToken("VAR")) {
            std::cout << "[HYPOTHESIS 3] initDeclaration: Processing VAR section, currentPos=" << currentPos << std::endl;
            currentPos++;
            int varCount = 0;
            while (currentPos < tokenList.size() && !isTypeToken("BEGIN")) {
                varCount++;
                std::cout << "[HYPOTHESIS 3] initDeclaration: Processing VAR statement #" << varCount 
                          << " at pos=" << currentPos << std::endl;
                try {
                    initRowStatement("Var");
                } catch (const std::bad_alloc& e) {
                    std::cerr << "[ERROR] [HYPOTHESIS 3] initDeclaration: bad_alloc in VAR statement #" << varCount 
                              << ": " << e.what() << std::endl;
                    throw;
                }
            }
            std::cout << "[HYPOTHESIS 3] initDeclaration: VAR statements processed: " << varCount << std::endl;
            if (currentPos < tokenList.size() && isTypeToken("BEGIN")) {
                currentPos++;
            }
        }
        std::cout << "[HYPOTHESIS 3] initDeclaration: Starting initBegin(), currentPos=" << currentPos << std::endl;
        try {
            initBegin();
        } catch (const std::bad_alloc& e) {
            std::cerr << "[ERROR] [HYPOTHESIS 3] initDeclaration: bad_alloc in initBegin: " << e.what() << std::endl;
            throw;
        }
        std::cout << "[HYPOTHESIS 3] initDeclaration: Completed successfully" << std::endl;
        return;
    }

    void print() {
        for (auto item: expressionList) {
            item.first->print(0);
        }
    }

    void initRowStatement(const string& chapter) {//метод чтобы строчку кода (не условие и не цикл) переводить в StatementExpression
        size_t startPos = currentPos;
        int tokenCount = 0;
        
        while (!isTypeToken("SEMICOLON")) {
            if (currentPos >= tokenList.size()) {
                std::cerr << "[ERROR] [HYPOTHESIS 3] initRowStatement: Reached end of tokenList before SEMICOLON at pos=" 
                          << currentPos << ", chapter=" << chapter << std::endl;
                throw std::runtime_error("Unexpected end of tokens in initRowStatement");
            }
            localList.push_back(tokenList[currentPos]);
            tokenCount++;
            currentPos++;
            
            // Защита от бесконечного цикла
            if (tokenCount > 10000) {
                std::cerr << "[ERROR] [HYPOTHESIS 3] initRowStatement: Too many tokens without SEMICOLON! pos=" 
                          << currentPos << ", chapter=" << chapter << std::endl;
                throw std::runtime_error("Too many tokens in statement");
            }
        }
        
        try {
            // ИСПРАВЛЕНО: используем move для передачи localList
            auto *rx = new StatementExpression(std::move(localList));
            std::pair t = {rx, chapter};
            expressionList.emplace_back(t);
        } catch (const std::bad_alloc& e) {
            std::cerr << "[ERROR] [HYPOTHESIS 3] initRowStatement: bad_alloc creating StatementExpression" 
                      << ", chapter=" << chapter << ", tokens=" << tokenCount 
                      << ", startPos=" << startPos << ": " << e.what() << std::endl;
            throw;
        }
        
        localList.clear();
        currentPos++;
        return;
    }

    void initBegin() {
        std::cout << "[HYPOTHESIS 3] initBegin: Starting, currentPos=" << currentPos 
                  << ", tokenList.size()=" << tokenList.size() << std::endl;
        
        std::vector<Token> condition;
        int statementCount = 0;
        while (!isTypeToken("ENDofPROGRAM")) {
            statementCount++;
            if (statementCount % 10 == 0) {
                std::cout << "[HYPOTHESIS 3] initBegin: Processed " << statementCount 
                          << " statements, currentPos=" << currentPos << std::endl;
            }
            
            try {
                if (isTypeToken("SWITCH")) {
                    std::cout << "[HYPOTHESIS 3] initBegin: Creating CaseOf at pos=" << currentPos << std::endl;
                    auto *sw = new CaseOf(currentPos, tokenList);
                    currentPos = sw->getPos();
                    std::pair t{sw, "Body"};
                    expressionList.push_back(t);
                    std::cout << "[HYPOTHESIS 3] initBegin: CaseOf created, new pos=" << currentPos << std::endl;
                    continue;
                }
                if ((isTypeToken("CONDITION")) || (isTypeToken("UNCONDITION")) || (isTypeToken("CYCLEFOR")) ||
                    (isTypeToken("CYCLEWHILE")) || (isTypeToken("CYCLEDOWHILE"))) {
                    std::cout << "[HYPOTHESIS 3] initBegin: Creating ConditionExpression at pos=" << currentPos 
                              << ", type=" << tokenList[currentPos].getType() << std::endl;
                    auto *cx = new ConditionExpression(currentPos, tokenList);
                    currentPos = cx->getGlobalPos();
                    std::pair t = {cx, "Body"};
                    expressionList.push_back(t);
                    std::cout << "[HYPOTHESIS 3] initBegin: ConditionExpression created, new pos=" << currentPos << std::endl;
                    continue;

                } else {
                    if (statementCount <= 5 || statementCount % 20 == 0) {
                        std::cout << "[HYPOTHESIS 3] initBegin: Processing row statement #" << statementCount 
                                  << " at pos=" << currentPos << std::endl;
                    }
                    initRowStatement("Body");
                }
            } catch (const std::bad_alloc& e) {
                std::cerr << "[ERROR] [HYPOTHESIS 3] initBegin: bad_alloc at statement #" << statementCount 
                          << ", pos=" << currentPos << ", token type=" << tokenList[currentPos].getType() 
                          << ", token value=\"" << tokenList[currentPos].getValue() << "\": " << e.what() << std::endl;
                throw;
            } catch (const std::exception& e) {
                std::cerr << "[ERROR] [HYPOTHESIS 3] initBegin: exception at statement #" << statementCount 
                          << ", pos=" << currentPos << ": " << e.what() << std::endl;
                throw;
            }
        }

        std::cout << "[HYPOTHESIS 3] initBegin: Completed, processed " << statementCount << " statements total" << std::endl;
        return;
    }

    bool isTypeToken(const string &typeToken) {
        return tokenList[currentPos].getType() == typeToken;
    }
};

#endif //PARSER_H