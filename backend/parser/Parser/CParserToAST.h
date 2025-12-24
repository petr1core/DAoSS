//
// Created by пк on 17.11.2025.
//

#ifndef SIMPLE_CPP_PARSER_H
#define SIMPLE_CPP_PARSER_H

#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <cctype>
#include <algorithm>
#include "../Ast/Expr.h"
#include "../Ast/Stmt.h"
#include "../Scripts/Lexer.h"

class CParserToAST {
public:
    std::unique_ptr<Program> parse(const std::string &code) {
        Lexer lexer(code, LangType::LANG_C);
        tokens = lexer.getTokenList();
        
        std::cout << "[CParser::parse] Lexer created " << tokens.size() << " tokens" << std::endl;
        if (tokens.size() > 0) {
            std::cout << "[CParser::parse] First token: " << tokens[0].getType() 
                      << " = \"" << tokens[0].getValue() << "\"" << std::endl;
            if (tokens.size() > 10) {
                std::cout << "[CParser::parse] Token #10: " << tokens[10].getType() 
                          << " = \"" << tokens[10].getValue() << "\"" << std::endl;
            }
            if (tokens.size() > 50) {
                std::cout << "[CParser::parse] Token #50: " << tokens[50].getType() 
                          << " = \"" << tokens[50].getValue() << "\"" << std::endl;
            }
            std::cout << "[CParser::parse] Last token: " << tokens[tokens.size()-1].getType() 
                      << " = \"" << tokens[tokens.size()-1].getValue() << "\"" << std::endl;
        }
        
        // Включаем полный вывод первых 50 токенов для отладки
        std::cout << "[CParser::parse] First 50 tokens:" << std::endl;
        for (size_t i = 0; i < std::min(size_t(50), tokens.size()); i++) {
            std::cout << "  [" << i << "] " << tokens[i].getType() 
                      << " = \"" << tokens[i].getValue() << "\"" << std::endl;
        }
        if (tokens.size() > 50) {
            std::cout << "  ... and " << (tokens.size() - 50) << " more tokens" << std::endl;
        }
        
        // Фильтруем пробелы и комментарии, если они вдруг остались
        std::vector<Token> filtered;
        filtered.reserve(tokens.size());
        int filteredCount = 0;
        for (const auto &token: tokens) {
            if (token.getType() == "SPACE" || token.getType() == "COMMENT") {
                filteredCount++;
                continue;
            }
            filtered.push_back(token);
        }
        std::cout << "[CParser::parse] Filtered out " << filteredCount 
                  << " SPACE/COMMENT tokens, remaining: " << filtered.size() << std::endl;
        tokens.swap(filtered);
        current = 0;

        auto program = std::make_unique<Program>();
        program->name = "test_program_by_C";

        // Если токенов нет, возвращаем пустую программу
        if (tokens.empty()) {
            program->body = std::make_unique<Block>();
            return program;
        }

        program->body = parseTranslationUnit();
        return program;
    }

private:
    std::vector<Token> tokens;
    size_t current{0};
    std::unordered_set<std::string> knownStructs;
    std::unordered_set<std::string> knownTypedefs;

    std::unique_ptr<Stmt> parseStruct() {
        auto structDecl = std::make_unique<StructDecl>();

        bool isStruct = match("STRUCT");
        bool isUnion = match("UNION");

        if (check("IDENTIFIER")) {
            std::string structName = advance().getValue();
            structDecl->name = (isStruct ? "struct " : "union ") + structName;

            // Добавляем в knownStructs только если есть имя
            addStructType(structDecl->name);
            addStructType(structName); // Добавляем и короткое имя для Point p1;
        } else {
            structDecl->name = isStruct ? "struct" : "union";
            // Анонимные структуры не добавляем в knownStructs
        }


        // Парсим тело структуры если есть
        if (match("OPENCURLY")) {
            while (!isAtEnd() && !check("CLOSECURLY")) {
                // Парсим каждое поле структуры
                if (auto field = parseStructField()) {
                    structDecl->fields.push_back(std::move(field));
                } else {
                    // Если не удалось распарсить поле, пропускаем токен
                    advance();
                }
            }
            consume("CLOSECURLY", "Ожидался '}' после структуры");
        }
        if (!structDecl->name.empty()) {
            addStructType(structDecl->name);
        }
        return structDecl;
    }

    std::unique_ptr<VarDeclStmt> parseStructField() {
        size_t save = current;

        try {
            auto field = std::make_unique<VarDeclStmt>();
            field->typeName = parseTypeName();

            if (!check("IDENTIFIER")) {
                current = save;
                return nullptr;
            }

            field->name = advance().getValue();

            // Обработка массивов в полях структуры
            while (match("OPENBRACKET")) {
                field->typeName += "[";
                if (!check("CLOSEBRACKET")) {
                    if (auto sizeExpr = parseExpression()) {
                        if (auto intLit = dynamic_cast<IntLiteral *>(sizeExpr.get())) {
                            field->typeName += std::to_string(intLit->value);
                        }
                    }
                }
                consume("CLOSEBRACKET", "Ожидался ']'");
                field->typeName += "]";
            }

            consume("SEMICOLON", "Ожидался ';' после поля структуры");
            return field;

        } catch (...) {
            current = save;
            return nullptr;
        }
    }

    std::unique_ptr<Stmt> parseGlobalVar() {
        try {
            auto decl = std::make_unique<VarDeclStmt>();
            decl->typeName = parseTypeName();

            auto nameTok = consume("IDENTIFIER", "Ожидалось имя переменной");
            decl->name = nameTok.getValue();

            // Обработка массивов
            if (match("OPENBRACKET")) {
                decl->typeName += "[]";
                consume("CLOSEBRACKET", "Ожидался ']' после размера массива");
            }

            if (match("ASSIGN")) {
                decl->initializer = parseExpression();
            }

            consume("SEMICOLON", "Ожидался ';' после объявления");
            return decl;
        } catch (const std::exception &e) {
            std::cerr << "Error in parseGlobalVar: " << e.what() << std::endl;
            return nullptr;
        }
    }

    std::unique_ptr<Stmt> parseCase() {
        auto caseStmt = std::make_unique<CaseStmt>();

        // Парсим значение case (может быть выражением)
        if (!check("COLON")) {
            caseStmt->value = parseExpression();
        }

        consume("COLON", "Ожидалось ':' после case");

        // Парсим тело case до следующего case/default/}
        auto bodyBlock = std::make_unique<Block>();
        while (!isAtEnd() && !check("CASE") && !check("DEFAULT") && !check("CLOSECURLY")) {
            if (auto stmt = parseStatement()) {
                bodyBlock->statements.push_back(std::move(stmt));
            } else {
                advance();
            }
        }

        caseStmt->body = std::move(bodyBlock->statements);
        return caseStmt;
    }

    std::unique_ptr<Stmt> parseDefault() {
        auto defaultStmt = std::make_unique<DefaultStmt>();

        consume("COLON", "Ожидалось ':' после default");

        // Парсим тело default до следующего case/default/}
        auto bodyBlock = std::make_unique<Block>();
        while (!isAtEnd() && !check("CASE") && !check("DEFAULT") && !check("CLOSECURLY")) {
            if (auto stmt = parseStatement()) {
                bodyBlock->statements.push_back(std::move(stmt));
            } else {
                advance();
            }
        }

        defaultStmt->body = std::move(bodyBlock->statements);
        return defaultStmt;
    }

    std::unique_ptr<Stmt> parseSwitch() {
        consume("OPENPARENTHESES", "Ожидалась '(' после switch");
        auto condition = parseExpression();
        consume("CLOSEPARENTHESES", "Ожидалась ')' после условия switch");
        consume("OPENCURLY", "Ожидался '{' после switch");

        auto switchStmt = std::make_unique<SwitchStmt>();
        switchStmt->condition = std::move(condition);

        // Упрощенно: пропускаем содержимое switch
        while (!isAtEnd() && !check("CLOSECURLY")) {
            if (match("CASE")) {
                auto caseStmt = parseCase();
                switchStmt->cases.push_back(std::move(caseStmt));
            } else if (match("DEFAULT")) {
                auto defaultStmt = parseDefault();
                switchStmt->cases.push_back(std::move(defaultStmt));
            } else {
                advance();
            }
        }

        consume("CLOSECURLY", "Ожидался '}' после switch");
        return switchStmt;
    }

    std::unique_ptr<Block> parseTranslationUnit() {
        auto block = std::make_unique<Block>();

        std::cout << "[CParser] Starting parseTranslationUnit with " << tokens.size() << " tokens" << std::endl;
        
        if (tokens.empty()) {
            std::cerr << "[WARNING] [CParser] parseTranslationUnit: tokens list is empty!" << std::endl;
            return block;
        }

        int iterationCount = 0;
        const int MAX_ITERATIONS = 100000; // Защита от бесконечного цикла

        while (!isAtEnd() && current < tokens.size()) {
            if (iterationCount++ > MAX_ITERATIONS) {
                std::cerr << "[ERROR] [CParser] parseTranslationUnit: Too many iterations (> " 
                          << MAX_ITERATIONS << "), breaking loop" << std::endl;
                break;
            }
            
            std::string tmp = peekType();
            std::cout << "[CParser] Current token " << current << ": " << tmp 
                      << " [" << peek().getValue() << "]" << std::endl;

            // Пропускаем комментарии
            if (tmp == "COMMENT") {
                std::cout << "[CParser] Skipping: " << tmp << std::endl;
                if (current < tokens.size()) {
                    advance();
                } else {
                    break;
                }
                continue;
            }
            if (isPreprocessor(tmp)) {
                // Для каждой препроцессорной директивы создаем отдельный узел
                int ppCount = 0;
                const int MAX_PP_DIRECTIVES = 1000;
                while (!isAtEnd() && current < tokens.size() && isPreprocessor(peekType()) && ppCount++ < MAX_PP_DIRECTIVES) {
                    try {
                        auto pp = parsePreprocessor();
                        if (pp) {
                            block->statements.push_back(std::move(pp));
                            std::cout << "[CParser] Added preprocessor directive, total statements: " 
                                      << block->statements.size() << std::endl;
                        }
                    } catch (const std::exception& e) {
                        std::cerr << "[ERROR] [CParser] Error parsing preprocessor directive: " 
                                  << e.what() << std::endl;
                        if (current < tokens.size()) {
                            advance();
                        } else {
                            break;
                        }
                    }
                }
                continue;
            }
            // Функции (прототипы и определения)
            if (isFunctionStart()) {
                try {
                    std::cout << "[CParser] Detected function start at token " << current << std::endl;
                    auto func = parseFunction();

                    if (func) {
                        if (auto functionDecl = dynamic_cast<FunctionDecl *>(func.get())) {
                            std::cout << "[CParser] Parsed function: " << functionDecl->name 
                                      << ", returnType: " << functionDecl->returnType << std::endl;
                            if (functionDecl->name == "main") {
                                // Основная функция распарсена, остальной код - это другие функции
                                std::cout << "[CParser] Main function parsed, continuing with other functions..." << std::endl;
                            }
                        }
                        block->statements.push_back(std::move(func));
                        std::cout << "[CParser] Successfully added function to block. Current position: " 
                                  << current << ", total statements: " << block->statements.size() << std::endl;

                        if (isAtEnd() || current >= tokens.size()) {
                            std::cout << "[CParser] Reached end after parsing function" << std::endl;
                            break;
                        }
                        continue;
                    } else {
                        std::cerr << "[ERROR] [CParser] parseFunction() returned nullptr" << std::endl;
                    }
                } catch (const std::exception &e) {
                    std::cerr << "[ERROR] [CParser] Error parsing function: " << e.what() << std::endl;
                    if (!isAtEnd() && current < tokens.size()) {
                        advance();
                    } else {
                        break;
                    }
                }
                continue;
            }
            if (tmp == "STRUCT" || tmp == "UNION") {
                try {
                    auto structDecl = parseStruct();

                    if (current < tokens.size()) {
                        consume("SEMICOLON", "Ожидался ';' после struct");
                    }

                    block->statements.push_back(std::move(structDecl));
                    std::cout << "[CParser] Added struct/union, total statements: " 
                              << block->statements.size() << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "[ERROR] [CParser] Error parsing struct/union: " << e.what() << std::endl;
                    if (current < tokens.size()) {
                        advance();
                    } else {
                        break;
                    }
                }
                continue;
            }

            if (tmp == "TYPEDEF") {
                auto typedefDecl = std::make_unique<TypedefDecl>();
                advance(); // typedef

                // Сохраняем текущую позицию для отладки
                size_t typedefStart = current;

                try {
                    // Парсим базовый тип (может быть struct или простой тип)
                    if (check("STRUCT") || check("UNION")) {
                        // typedef struct Point Point;
                        auto structPart = parseStruct();
                        if (auto sd = dynamic_cast<StructDecl *>(structPart.get())) {
                            typedefDecl->typeName = sd->name;
                        }
                    } else {
                        // typedef int MyInt;
                        typedefDecl->typeName = parseTypeName();
                    }

                    // Парсим алиас
                    auto aliasTok = consume("IDENTIFIER", "Ожидалось имя для typedef");
                    typedefDecl->alias = aliasTok.getValue();

                    consume("SEMICOLON", "Ожидался ';' после typedef");

                    addTypedef(typedefDecl->alias);
                    block->statements.push_back(std::move(typedefDecl));

                } catch (const std::exception &e) {
                    std::cerr << "Error parsing typedef at token " << typedefStart << ": " << e.what() << std::endl;
                    // Пропускаем проблемный typedef
                    while (!isAtEnd() && !check("SEMICOLON")) {
                        advance();
                    }
                    if (!isAtEnd()) advance(); // ;
                }
                continue;
            }
            // Глобальные переменные
            if (isGlobalVarStart()) {
                try {
                    std::cout << "Attempting to parse global variable..." << std::endl;

                    // Обрабатываем const отдельно
                    // В parseTranslationUnit() замените блок обработки const:
                    if (match("CONST")) {
                        std::cout << "Parsing const variable declaration..." << std::endl;

                        auto varDecl = std::make_unique<VarDeclStmt>();

                        // Пропускаем const и парсим тип
                        advance(); // CONST
                        varDecl->typeName = "const " + parseTypeName();

                        auto nameTok = consume("IDENTIFIER", "Ожидалось имя const переменной");
                        varDecl->name = nameTok.getValue();

                        if (match("ASSIGN")) {
                            varDecl->initializer = parseExpression();
                        }

                        consume("SEMICOLON", "Ожидался ';' после const объявления");

                        // СОХРАНИМ имя ДО перемещения
                        std::string varName = varDecl->name;

                        block->statements.push_back(std::move(varDecl));
                        std::cout << "Successfully parsed const: " << varName
                                  << std::endl; // Используем сохраненное имя
                        continue;
                    } else {
                        // Обычная глобальная переменная
                        auto varDecl = parseGlobalVar();
                        if (varDecl) {
                            block->statements.push_back(std::move(varDecl));
                            std::cout << "Parsed global variable" << std::endl;
                            continue;
                        }
                    }
                } catch (const std::exception &e) {
                    std::cerr << "Error parsing global variable: " << e.what() << std::endl;
                    // Пропускаем проблемное объявление
                    while (!isAtEnd() && !check("SEMICOLON")) {
                        advance();
                    }
                    if (!isAtEnd()) advance();
                }
                continue;
            }

            // Попытка распознать как начало функции (fallback для случаев, когда isFunctionStart() не сработал)
            if (isTypeToken(tmp)) {
                size_t savePos = current;
                try {
                    std::cout << "[CParser] Trying to parse as function (fallback), type token: " << tmp << std::endl;
                    // Пробуем распарсить тип
                    std::string typeName = parseTypeName();
                    std::cout << "[CParser] Parsed type name: " << typeName << std::endl;
                    
                    // Проверяем, что следующий токен - идентификатор
                    if (check("IDENTIFIER")) {
                        std::string funcName = peek().getValue();
                        std::cout << "[CParser] Found identifier after type: " << funcName << std::endl;
                        
                        // Проверяем, что после идентификатора идет '('
                        size_t nextPos = current + 1;
                        if (nextPos < tokens.size() && tokens[nextPos].getType() == "OPENPARENTHESES") {
                            std::cout << "[CParser] Detected function signature, parsing function..." << std::endl;
                            current = savePos; // Откатываемся к началу
                            auto func = parseFunction();
                            if (func) {
                                block->statements.push_back(std::move(func));
                                std::cout << "[CParser] Successfully parsed function (fallback)" << std::endl;
                                continue;
                            }
                        }
                    }
                    // Если не получилось, откатываемся
                    current = savePos;
                } catch (const std::exception& e) {
                    std::cerr << "[ERROR] [CParser] Error in function fallback parsing: " << e.what() << std::endl;
                    current = savePos;
                }
            }
            
            // Если ничего не распарсилось, пропускаем токен
            std::cerr << "[WARNING] [CParser] No rule for token, skipping: " << tmp 
                      << " [" << peek().getValue() << "]" << std::endl;
            if (current < tokens.size()) {
                advance();
            } else {
                break;
            }
        }

        std::cout << "[CParser] Finished parseTranslationUnit. Total statements: " 
                  << block->statements.size() << ", final position: " << current 
                  << ", tokens.size(): " << tokens.size() << std::endl;
        return block;
    }

    std::unique_ptr<Stmt> parseStatement() {
        if (isAtEnd()) {
            return nullptr;
        }

        // Проверяем ключевые слова
        if (match("IF")) return parseIf();
        if (match("WHILE")) return parseWhile();
        if (match("FOR")) return parseFor();
        if (match("DO")) return parseDoWhile();
        if (match("SWITCH")) return parseSwitch();
        if (match("RETURN")) return parseReturn();
        if (match("SEMICOLON")) return nullptr; // пустой оператор
        if (match("OPENCURLY")) return parseBlockBody();
        if (match("BREAK")) {
            auto breakStmt = std::make_unique<BreakStmt>();
            consume("SEMICOLON", "Ожидался ';' после break");
            return breakStmt;
        }
        // Сохраняем позицию для отката
        size_t save = current;

        // Сначала пробуем как объявление структуры (Point p1;)
        if (check("IDENTIFIER") && isStructType(peek().getValue())) {
            if (auto structVar = parseStructVarDecl()) {
                return structVar;
            }
            current = save;
        }

        // Сначала пробуем как объявление переменной
        if (isTypeToken(peekType())) {
            if (auto varDecl = parseVarDecl()) {
                return varDecl;
            }
            // Если parseVarDecl вернул nullptr, восстанавливаем позицию
            current = save;
        }

        // Пробуем как присваивание
        if (auto assign = parseAssignmentStmt()) {
            return assign;
        }

        // Пробуем как выражение
        try {
            return parseExprStmt();
        } catch (const std::exception &e) {
            std::cerr << "Error parsing expression statement at token " << current << ": " << e.what() << std::endl;
            current = save;

            // Если ничего не получилось, пропускаем токен
            if (!isAtEnd()) {
                std::cout << "Skipping token in statement: " << peekType() << " [" << peek().getValue() << "]"
                          << std::endl;
                advance();
            }
            return nullptr;
        }
    }

    void addStructType(const std::string &name) {
        knownStructs.insert(name);
    }

    void addTypedef(const std::string &name) {
        knownTypedefs.insert(name);
    }

    bool isStructType(const std::string &name) {
        return knownStructs.count(name) > 0 || knownTypedefs.count(name) > 0;
    }

    std::unique_ptr<Stmt> parseFunction() {
        std::cout << "Starting to parse function at token " << current << std::endl;

        auto func = std::make_unique<FunctionDecl>();

        try {
            func->returnType = parseTypeName();
            std::cout << "Parsed return type: " << func->returnType << std::endl;

            auto nameTok = consume("IDENTIFIER", "Ожидалось имя функции");
            func->name = nameTok.getValue();
            std::cout << "Parsed function name: " << func->name << std::endl;

            consume("OPENPARENTHESES", "Ожидалась '(' после имени функции");
            std::cout << "Parsed opening parenthesis" << std::endl;

            func->params = parseParameters();
            std::cout << "Parsed " << func->params.size() << " parameters" << std::endl;

            consume("CLOSEPARENTHESES", "Ожидалась ')' после параметров");
            std::cout << "Parsed closing parenthesis" << std::endl;

            // Проверяем, это прототип или определение
            if (match("SEMICOLON")) {
                func->body = nullptr;
                std::cout << "Function prototype: " << func->name << " at position " << current << std::endl;
                return func;
            }

            if (!match("OPENCURLY")) {
                throw std::runtime_error("Ожидался блок тела функции");
            }
            std::cout << "Parsing function body..." << std::endl;

            func->body = parseBlockBody();
            std::cout << "Successfully parsed function definition: " << func->name << std::endl;

            return func;
        } catch (const std::exception &e) {
            std::cerr << "ERROR in parseFunction: " << e.what() << std::endl;
            throw;
        }
    }

    std::unique_ptr<Block> parseBlockBody() {
        auto block = std::make_unique<Block>();
        std::cout << "[CParser] Starting parseBlockBody at token " << current 
                  << ", tokens.size()=" << tokens.size() << std::endl;

        int statementCount = 0;
        const int MAX_STATEMENTS = 10000; // Защита от бесконечного цикла
        
        while (!isAtEnd() && current < tokens.size() && !check("CLOSECURLY")) {
            if (statementCount++ > MAX_STATEMENTS) {
                std::cerr << "[ERROR] [CParser] parseBlockBody: Too many statements (> " 
                          << MAX_STATEMENTS << "), breaking loop" << std::endl;
                break;
            }
            
            try {
                if (auto stmt = parseStatement()) {
                    block->statements.push_back(std::move(stmt));
                } else {
                    // Если не удалось распарсить оператор, пропускаем токен
                    if (!isAtEnd() && current < tokens.size()) {
                        std::cout << "[CParser] Skipping in block: " << peekType() 
                                  << " [" << peek().getValue() << "]" << std::endl;
                        advance();
                    } else {
                        break;
                    }
                }
            } catch (const std::exception &e) {
                std::cerr << "[ERROR] [CParser] Error in parseBlockBody: " << e.what() << std::endl;
                if (!isAtEnd() && current < tokens.size()) {
                    advance();
                } else {
                    break;
                }
            }
        }

        if (!isAtEnd() && current < tokens.size() && check("CLOSECURLY")) {
            consume("CLOSECURLY", "Ожидался символ '}'");
            std::cout << "[CParser] Closed block body, parsed " 
                      << block->statements.size() << " statements" << std::endl;
        } else {
            std::cerr << "[WARNING] [CParser] Block body not properly closed, current=" 
                      << current << ", tokens.size()=" << tokens.size() << std::endl;
        }

        return block;
    }

    std::unique_ptr<Stmt> parseIf() {
        consume("OPENPARENTHESES", "Ожидалась '(' после if");
        auto condition = parseExpression();
        if (!condition) {
            throw std::runtime_error("Условие if не может быть nullptr");
        }
        consume("CLOSEPARENTHESES", "Ожидалась ')' после условия if");

        auto thenBranch = parseStatement();
        if (!thenBranch) {
            throw std::runtime_error("Тело if не может быть nullptr");
        }
        std::unique_ptr<Stmt> elseBranch;
        if (match("ELSE")) {
            elseBranch = parseStatement();
        }

        auto stmt = std::make_unique<IfStmt>();
        stmt->condition = std::move(condition);
        stmt->thenBranch = std::move(thenBranch);
        stmt->elseBranch = std::move(elseBranch);
        return stmt;
    }

    std::unique_ptr<Stmt> parseWhile() {
        consume("OPENPARENTHESES", "Ожидалась '(' после while");
        auto condition = parseExpression();
        if (!condition) {
            throw std::runtime_error("Условие while не может быть nullptr");
        }
        consume("CLOSEPARENTHESES", "Ожидалась ')' после условия while");
        auto body = parseStatement();
        if (!body) {
            throw std::runtime_error("Тело while не может быть nullptr");
        }

        auto stmt = std::make_unique<WhileStmt>();
        stmt->condition = std::move(condition);
        stmt->body = std::move(body);
        return stmt;
    }

    bool isGlobalVarStart() {
        if (isAtEnd()) return false;

        size_t save = current;

        try {
            // Пробуем распарсить тип
            parseTypeName();

            // Должен быть идентификатор
            if (isAtEnd() || !check("IDENTIFIER")) {
                current = save;
                return false;
            }

            // После идентификатора должна быть ; или = или , или [
            size_t identPos = current + 1;
            if (identPos >= tokens.size()) {
                current = save;
                return false;
            }

            std::string nextType = tokens[identPos].getType();
            bool isGlobalVar = nextType == "SEMICOLON" || nextType == "ASSIGN" ||
                               nextType == "COMMA" || nextType == "OPENBRACKET";

            current = save;
            return isGlobalVar && !isFunctionStart(); // И это не функция
        } catch (...) {
            current = save;
            return false;
        }
    }

    std::unique_ptr<Stmt> parseFor() {
        consume("OPENPARENTHESES", "Ожидалась '(' после for");

        // Инициализация
        std::unique_ptr<Stmt> init;
        if (!check("SEMICOLON")) {
            if (isTypeToken(peekType())) {
                init = parseVarDecl(false);
            } else {
                // Парсим выражение присваивания
                if (auto assign = parseAssignmentStmt(false)) {
                    init = std::move(assign);
                } else {
                    // Или просто выражение
                    auto expr = parseExpression();
                    auto exprStmt = std::make_unique<ExprStmt>();
                    exprStmt->expression = std::move(expr);
                    init = std::move(exprStmt);
                }
            }
        }
        consume("SEMICOLON", "Ожидался ';' после инициализации for");

        // Условие
        std::unique_ptr<Expr> condition;
        if (!check("SEMICOLON")) {
            condition = parseExpression();
        }
        consume("SEMICOLON", "Ожидался ';' после условия for");

        // Инкремент
        std::unique_ptr<Expr> increment;
        if (!check("CLOSEPARENTHESES")) {
            increment = parseExpression();
        }
        consume("CLOSEPARENTHESES", "Ожидалась ')' после for");

        auto body = parseStatement();

        auto stmt = std::make_unique<ForStmt>();
        stmt->init = std::move(init);
        stmt->condition = std::move(condition);
        stmt->increment = std::move(increment);
        stmt->body = std::move(body);
        return stmt;
    }

    std::unique_ptr<Stmt> parseReturn() {
        auto stmt = std::make_unique<ReturnStmt>();
        if (!check("SEMICOLON")) {
            stmt->value = parseExpression();
        }
        consume("SEMICOLON", "Ожидался ';' после return");
        return stmt;
    }

    std::unique_ptr<Stmt> parseVarDecl(bool requireSemicolon = true) {
        std::cout << "Starting parseVarDecl at token " << current << " [" << peek().getValue() << "]" << std::endl;
        size_t save = current;

        try {
            auto firstDecl = std::make_unique<VarDeclStmt>();
            firstDecl->typeName = parseTypeName();
            if (firstDecl->typeName.find("struct") != std::string::npos ||
                firstDecl->typeName.find("union") != std::string::npos) {
                // Это объявление структуры - обрабатываем особо
                if (!check("IDENTIFIER")) {
                    // Анонимная структура или что-то не так
                    current = save;
                    return nullptr;
                }
            }
            // Проверяем, что следующий токен - идентификатор
            if (!check("IDENTIFIER")) {
                std::cout << "No identifier after type, not a variable declaration" << std::endl;
                current = save;
                return nullptr;
            }

            auto nameTok = advance();
            firstDecl->name = nameTok.getValue();

            // ИСПРАВЛЕННАЯ обработка массивов
            while (match("OPENBRACKET")) {
                firstDecl->typeName += "[";
                if (!check("CLOSEBRACKET")) {
                    // Парсим размер массива
                    if (auto sizeExpr = parseExpression()) {
                        if (auto intLit = dynamic_cast<IntLiteral *>(sizeExpr.get())) {
                            firstDecl->typeName += std::to_string(intLit->value);
                        }
                    }
                }
                consume("CLOSEBRACKET", "Ожидался ']'");
                firstDecl->typeName += "]";
            }

            // Обработка инициализатора
            if (match("ASSIGN")) {
                firstDecl->initializer = parseExpression();
            }

            // Если есть запятая, значит несколько переменных
            if (!match("COMMA")) {
                if (requireSemicolon) {
                    consume("SEMICOLON", "Ожидался ';' после объявления");
                }
                std::cout << "Successfully parsed single variable: " << firstDecl->name << std::endl;
                return firstDecl;
            }

            // Несколько переменных - создаем блок
            auto block = std::make_unique<Block>();
            block->statements.push_back(std::move(firstDecl));

            // Получаем тип из первой переменной
            std::string commonType;
            if (auto firstVar = dynamic_cast<VarDeclStmt *>(block->statements[0].get())) {
                commonType = firstVar->typeName;
            } else {
                throw std::runtime_error("Ожидалось объявление переменной");
            }

            // Обрабатываем остальные переменные
            do {
                auto decl = std::make_unique<VarDeclStmt>();
                decl->typeName = commonType;

                if (!check("IDENTIFIER")) {
                    throw std::runtime_error("Ожидалось имя переменной");
                }

                auto nameTok = advance();
                decl->name = nameTok.getValue();

                // Обработка массивов для последующих переменных
                while (match("OPENBRACKET")) {
                    decl->typeName += "[]";
                    if (!check("CLOSEBRACKET")) {
                        advance(); // пропускаем размер
                    }
                    if (!check("CLOSEBRACKET")) {
                        throw std::runtime_error("Ожидался ']'");
                    }
                    advance(); // consume ']'
                }

                if (match("ASSIGN")) {
                    decl->initializer = parseExpression();
                }

                block->statements.push_back(std::move(decl));
            } while (match("COMMA"));

            if (requireSemicolon) {
                consume("SEMICOLON", "Ожидался ';' после объявления");
            }

            std::cout << "Successfully parsed multiple variables" << std::endl;
            return block;

        } catch (const std::exception &e) {
            std::cerr << "Error in parseVarDecl at token " << save << ": " << e.what() << std::endl;
            current = save;
            return nullptr; // Возвращаем nullptr вместо throw
        }
    }

    std::unique_ptr<Stmt> parseDoWhile() {
        auto body = parseStatement();
        if (!body) throw std::runtime_error("Тело do-while не может быть nullptr");

        consume("WHILE", "Ожидалось 'while' после тела do");
        consume("OPENPARENTHESES", "Ожидалась '(' после while");
        auto condition = parseExpression();
        consume("CLOSEPARENTHESES", "Ожидалась ')' после условия");
        consume("SEMICOLON", "Ожидался ';' после do-while");

        auto stmt = std::make_unique<DoWhileStmt>();
        stmt->condition = std::move(condition);
        stmt->body = std::move(body);
        return stmt;
    }

    std::unique_ptr<Stmt> parseAssignmentStmt(bool requireSemicolon = true) {
        size_t save = current;

        try {
            // Парсим левую часть - может быть идентификатором или member access
            std::unique_ptr<Expr> targetExpr;

            // Проверяем разыменование (*ptr)
            if (match("MULTI")) {
                if (!check("IDENTIFIER")) {
                    current = save;
                    return nullptr;
                }
                auto unary = std::make_unique<UnaryOp>();
                unary->op = "*";
                unary->postfix = false;
                auto ident = std::make_unique<Identifier>();
                ident->name = advance().getValue();
                unary->operand = std::move(ident);
                targetExpr = std::move(unary);
            } else if (match("IDENTIFIER")) {
                targetExpr = std::make_unique<Identifier>();
                static_cast<Identifier *>(targetExpr.get())->name = previous().getValue();

                // Обрабатываем цепочку доступов (массивы, поля)
                while (true) {
                    if (match("OPENBRACKET")) {
                        auto arrayAccess = std::make_unique<ArrayAccessExpr>();
                        arrayAccess->array = std::move(targetExpr);
                        arrayAccess->index = parseExpression();
                        consume("CLOSEBRACKET", "Ожидался ']'");
                        targetExpr = std::move(arrayAccess);
                    } else if (match("DOT")) {
                        auto memberAccess = std::make_unique<MemberAccessExpr>();
                        memberAccess->object = std::move(targetExpr);
                        memberAccess->member = consume("IDENTIFIER", "Ожидалось имя поля").getValue();
                        memberAccess->isPointerAccess = false;
                        targetExpr = std::move(memberAccess);
                    } else {
                        break;
                    }
                }
            } else {
                return nullptr;
            }
            // Преобразуем выражение в строку для target (для обратной совместимости)
            std::string targetStr = exprToString(targetExpr.get());
            // Проверяем оператор присваивания
            if (match("ASSIGN") || match("PLUSASSIGN") || match("MINUSASSIGN") ||
                match("MULTIASSIGN") || match("DIVASSIGN") || match("MODASSIGN") ||
                match("INCREMENT") || match("DECREMENT")) {

                auto assign = std::make_unique<AssignStmt>();
                assign->target = targetStr;
                assign->op = previous().getType();

                // Для инкремента/декремента нет правой части
                if (assign->op == "INCREMENT" || assign->op == "DECREMENT") {
                    // Создаем унарную операцию для инкремента/декремента
                    auto unaryExpr = std::make_unique<UnaryOp>();
                    unaryExpr->op = (assign->op == "INCREMENT") ? "++" : "--";
                    unaryExpr->postfix = true;

                    unaryExpr->operand = std::move(targetExpr);
                    assign->value = std::move(unaryExpr);


                    if (requireSemicolon) {
                        consume("SEMICOLON", "Ожидался ';' после операции");
                    }

                    std::cout << "Successfully parsed increment/decrement: " << assign->target << std::endl;
                    return assign;
                }

                auto value = parseExpression();
                if (!value) {
                    throw std::runtime_error("Значение присваивания не может быть nullptr");
                }
                assign->value = std::move(value);

                if (requireSemicolon) {
                    consume("SEMICOLON", "Ожидался ';' после присваивания");
                }

                std::cout << "Successfully parsed assignment: " << assign->target << std::endl;
                return assign;
            }

            current = save;
            return nullptr;

        } catch (const std::exception &e) {
            std::cerr << "Error in parseAssignmentStmt: " << e.what() << std::endl;
            current = save;
            return nullptr;
        }
    }

    std::string exprToString(Expr *expr) {
        if (!expr) return "null";

        // Для Identifier
        if (auto ident = dynamic_cast<Identifier *>(expr)) {
            return ident->name;
        }
            // Для ArrayAccessExpr
        else if (auto arrayAccess = dynamic_cast<ArrayAccessExpr *>(expr)) {
            return exprToString(arrayAccess->array.get()) + "[" + exprToString(arrayAccess->index.get()) + "]";
        }
            // Для MemberAccessExpr
        else if (auto memberAccess = dynamic_cast<MemberAccessExpr *>(expr)) {
            std::string accessOp = memberAccess->isPointerAccess ? "->" : ".";
            return exprToString(memberAccess->object.get()) + accessOp + memberAccess->member;
        }
            // Для UnaryOp
        else if (auto unary = dynamic_cast<UnaryOp *>(expr)) {
            std::string op = unary->op;
            if (unary->postfix) {
                return exprToString(unary->operand.get()) + op;
            } else {
                return op + exprToString(unary->operand.get());
            }
        }
            // Для других типов выражений можно добавить обработку по необходимости
        else {
            return "expr";
        }
    }

    std::unique_ptr<Stmt> parseExprStmt(bool requireSemicolon = true) {
        auto expr = parseExpression();
        if (!expr) {
            throw std::runtime_error("Выражение не может быть nullptr");
        }
        if (requireSemicolon) {
            consume("SEMICOLON", "Ожидался ';' после выражения");
        }
        auto stmt = std::make_unique<ExprStmt>();
        stmt->expression = std::move(expr);
        return stmt;
    }

    std::vector<FunctionDecl::Parameter> parseParameters() {
        std::vector<FunctionDecl::Parameter> params;
        if (check("CLOSEPARENTHESES")) {
            return params;
        }
        if (check("VOID")) {
            advance();
            if (check("CLOSEPARENTHESES")) {
                return params;
            }
            // иначе void считается типом параметра
            current--;
        }
        do {
            FunctionDecl::Parameter param;
            param.typeName = parseTypeName();
            auto nameTok = consume("IDENTIFIER", "Ожидалось имя параметра");
            param.name = nameTok.getValue();
            params.push_back(param);
        } while (match("COMMA"));
        return params;
    }

    std::string parseTypeName() {
        std::string typeName;

        // Обрабатываем const в начале типа (const int, const float и т.д.)
        bool isConst = false;
        if (match("CONST")) {
            isConst = true;
            typeName = "const ";
        }

        // Базовый тип - читаем только один токен типа за раз (не цикл!)
        if (!isAtEnd() && isTypeToken(peekType())) {
            typeName += advance().getValue();
            
            // Модификаторы типа (static и т.д.) - но только если они идут сразу после типа
            // НЕ читаем несколько базовых типов подряд!
        } else if (isConst && !typeName.empty()) {
            // Если был const, но тип не распознан, возвращаем пустую строку
            // (это ошибка, но не ломаем парсинг)
            return "";
        }

        // Указатели и ссылки
        while (match("MULTI") || match("BITAND")) {
            if (previous().getType() == "MULTI") {
                typeName += "*";
            } else {
                typeName += "&";
            }

            // Константные указатели
            if (match("CONST")) {
                typeName += " const";
            }
        }

        // Массивы
        if (match("OPENBRACKET")) {
            typeName += "[]";
            consume("CLOSEBRACKET", "Ожидался ']'");
        }

        return typeName;
    }

    std::unique_ptr<Expr> parseExpression() {
        return parseTernary();
    }

    std::unique_ptr<Expr> parseTernary() {
        auto expr = parseLogicalOr();

        if (match("QUESTION")) {
            auto thenExpr = parseExpression();
            consume("COLON", "Ожидалось ':' в тернарном операторе");
            auto elseExpr = parseExpression();

            auto ternary = std::make_unique<TernaryOp>();
            ternary->condition = std::move(expr);
            ternary->thenExpr = std::move(thenExpr);
            ternary->elseExpr = std::move(elseExpr);
            return ternary;
        }

        return expr;
    }

    std::unique_ptr<Expr> parseLogicalOr() {
        auto expr = parseLogicalAnd();
        if (!expr) {
            throw std::runtime_error("Логическое выражение не может быть nullptr");
        }
        while (match("OR")) {
            auto right = parseLogicalAnd();
            expr = makeBinary(std::move(expr), BinOpKind::Or, std::move(right));
        }
        return expr;
    }

    std::unique_ptr<Expr> parseLogicalAnd() {
        auto expr = parseBitwiseOr();
        while (match("AND")) {
            auto right = parseBitwiseOr();
            expr = makeBinary(std::move(expr), BinOpKind::And, std::move(right));
        }
        return expr;
    }

    std::unique_ptr<Expr> parseBitwiseOr() {
        auto expr = parseBitwiseXor();
        while (match("BITOR")) {
            auto right = parseBitwiseXor();
            expr = makeBinary(std::move(expr), BinOpKind::BitOr, std::move(right));
        }
        return expr;
    }

    std::unique_ptr<Expr> parseBitwiseXor() {
        auto expr = parseBitwiseAnd();
        while (match("BITXOR")) {
            auto right = parseBitwiseAnd();
            expr = makeBinary(std::move(expr), BinOpKind::BitXor, std::move(right));
        }
        return expr;
    }

    std::unique_ptr<Expr> parseBitwiseAnd() {
        auto expr = parseEquality();
        while (match("BITAND")) {
            auto right = parseEquality();
            expr = makeBinary(std::move(expr), BinOpKind::BitAnd, std::move(right));
        }
        return expr;
    }

    std::unique_ptr<Expr> parseEquality() {
        auto expr = parseRelational();
        while (true) {
            if (match("JE")) {
                auto right = parseRelational();
                expr = makeBinary(std::move(expr), BinOpKind::Eq, std::move(right));
            } else if (match("JNE")) {
                auto right = parseRelational();
                expr = makeBinary(std::move(expr), BinOpKind::Ne, std::move(right));
            } else {
                break;
            }
        }
        return expr;
    }

    std::unique_ptr<Expr> parseRelational() {
        auto expr = parseShift();
        while (true) {
            if (match("JGE")) {
                auto right = parseShift();
                expr = makeBinary(std::move(expr), BinOpKind::Ge, std::move(right));
            } else if (match("JLE")) {
                auto right = parseShift();
                expr = makeBinary(std::move(expr), BinOpKind::Le, std::move(right));
            } else if (match("JG")) {
                auto right = parseShift();
                expr = makeBinary(std::move(expr), BinOpKind::Gt, std::move(right));
            } else if (match("JL")) {
                auto right = parseShift();
                expr = makeBinary(std::move(expr), BinOpKind::Lt, std::move(right));
            } else {
                break;
            }
        }
        return expr;
    }

    std::unique_ptr<Expr> parseShift() {
        auto expr = parseAdditive();
        while (true) {
            if (match("BITSHIFTLEFT")) {
                auto right = parseAdditive();
                expr = makeBinary(std::move(expr), BinOpKind::Shl, std::move(right));
            } else if (match("BITSHIFTRIGHT")) {
                auto right = parseAdditive();
                expr = makeBinary(std::move(expr), BinOpKind::Shr, std::move(right));
            } else {
                break;
            }
        }
        return expr;
    }

    std::unique_ptr<Expr> parseAdditive() {
        auto expr = parseMultiplicative();
        while (true) {
            if (match("PLUS")) {
                auto right = parseMultiplicative();
                expr = makeBinary(std::move(expr), BinOpKind::Add, std::move(right));
            } else if (match("MINUS")) {
                auto right = parseMultiplicative();
                expr = makeBinary(std::move(expr), BinOpKind::Sub, std::move(right));
            } else {
                break;
            }
        }
        return expr;
    }

    std::unique_ptr<Expr> parseMultiplicative() {
        auto expr = parseUnary();
        while (true) {
            if (match("MULTI")) {
                auto right = parseUnary();
                expr = makeBinary(std::move(expr), BinOpKind::Mul, std::move(right));
            } else if (match("DIV")) {
                auto right = parseUnary();
                expr = makeBinary(std::move(expr), BinOpKind::Div, std::move(right));
            } else if (match("MOD")) {
                auto right = parseUnary();
                expr = makeBinary(std::move(expr), BinOpKind::Mod, std::move(right));
            } else {
                break;
            }
        }
        return expr;
    }

    std::unique_ptr<Expr> parseType() {
        // Упрощенная версия - возвращаем идентификатор типа
        auto typeExpr = std::make_unique<Identifier>();
        typeExpr->name = parseTypeName();
        return typeExpr;
    }

    std::unique_ptr<Expr> parseUnary() {
        if (match("SIZEOF")) {
            auto sizeofExpr = std::make_unique<SizeofExpr>();

            if (match("OPENPARENTHESES")) {
                // sizeof(type)
                if (isTypeToken(peekType())) {
                    sizeofExpr->expression = parseType();
                    sizeofExpr->isType = true;
                } else {
                    //sizeof(expression)
                    sizeofExpr->expression = parseExpression();
                    sizeofExpr->isType = false;
                }
                consume("CLOSEPARENTHESES", "Ожидалась ')' после sizeof");
            } else {
                // sizeof expression (без скобок)
                sizeofExpr->expression = parseUnary();
                sizeofExpr->isType = false;
            }

            return sizeofExpr;
        }
        if (match("INCREMENT")) {
            auto operand = parseUnary();
            return makeUnary("++", std::move(operand), false);
        }
        if (match("DECREMENT")) {
            auto operand = parseUnary();
            return makeUnary("--", std::move(operand), false);
        }
        if (match("MULTI")) {
            auto operand = parseUnary();
            return makeUnary("*", std::move(operand), false);
        }

        if (match("BITAND")) {
            auto operand = parseUnary();
            return makeUnary("&", std::move(operand), false);
        }
        if (match("PLUS")) {
            auto operand = parseUnary();
            return operand;
        }
        if (match("MINUS")) {
            auto operand = parseUnary();
            return makeUnary("-", std::move(operand), false);
        }
        if (match("NOT")) {
            auto operand = parseUnary();
            return makeUnary("!", std::move(operand), false);
        }
        if (match("BITNOT")) {
            auto operand = parseUnary();
            return makeUnary("~", std::move(operand), false);
        }
        return parsePostfix();
    }

    std::unique_ptr<Expr> parsePostfix() {
        auto expr = parsePrimary();
        while (true) {
            if (match("INCREMENT")) {
                expr = makeUnary("++", std::move(expr), true);
            } else if (match("DECREMENT")) {
                expr = makeUnary("--", std::move(expr), true);
            } else {
                break;
            }
        }
        return expr;
    }

    std::unique_ptr<Expr> parsePrimary() {
        // Проверяем разыменование в начале выражения
        if (match("MULTI")) {
            auto operand = parsePrimary();
            return makeUnary("*", std::move(operand), false);
        }

        // Проверяем взятие адреса в начале выражения
        if (match("BITAND")) {
            auto operand = parsePrimary();
            return makeUnary("&", std::move(operand), false);
        }

        if (match("OPENPARENTHESES")) {
            auto expr = parseExpression();
            consume("CLOSEPARENTHESES", "Ожидалась ')'");
            return expr;
        }

        if (match("VALUESTRING")) {
            auto lit = std::make_unique<StringLiteral>();
            auto value = previous().getValue();
            if (!value.empty() && value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.size() - 2);
            }
            lit->value = value;
            return lit;
        }

        if (match("VALUECHAR")) {
            auto lit = std::make_unique<StringLiteral>();
            auto value = previous().getValue();
            if (value.size() >= 2 && value.front() == '\'' && value.back() == '\'') {
                value = value.substr(1, value.size() - 2);
            }
            lit->value = value;
            return lit;
        }

        if (match("VALUEFLOAT") || match("VALUEDOUBLE")) {
            auto lit = std::make_unique<RealLiteral>();
            lit->value = std::stod(previous().getValue());
            return lit;
        }

        if (match("VALUEHEX")) {
            auto lit = std::make_unique<IntLiteral>();
            lit->value = std::stoll(previous().getValue(), nullptr, 16);
            return lit;
        }

        if (match("VALUEOCTAL")) {
            auto lit = std::make_unique<IntLiteral>();
            lit->value = std::stoll(previous().getValue(), nullptr, 8);
            return lit;
        }

        if (match("VALUEINTEGER")) {
            auto lit = std::make_unique<IntLiteral>();
            lit->value = std::stoll(previous().getValue(), nullptr, 10);
            return lit;
        }

        if (match("IDENTIFIER")) {
            std::unique_ptr<Expr> ident = std::make_unique<Identifier>();
            static_cast<Identifier *>(ident.get())->name = previous().getValue();
            // Проверяем member access (p1.x)
            while (true) {
                // Доступ к массиву
                if (match("OPENBRACKET")) {
                    auto arrayAccess = std::make_unique<ArrayAccessExpr>();
                    arrayAccess->array = std::move(ident);
                    arrayAccess->index = parseExpression();
                    consume("CLOSEBRACKET", "Ожидался ']'");
                    ident = std::move(arrayAccess);
                }
                    // Доступ к полю
                else if (match("DOT")) {
                    auto memberAccess = std::make_unique<MemberAccessExpr>();
                    memberAccess->object = std::move(ident);
                    memberAccess->member = consume("IDENTIFIER", "Ожидалось имя поля").getValue();
                    memberAccess->isPointerAccess = false;
                    ident = std::move(memberAccess);
                }
                    // Доступ через указатель (->)
                else if (match("PTRACCESS")) {
                    auto memberAccess = std::make_unique<MemberAccessExpr>();
                    memberAccess->object = std::move(ident);
                    memberAccess->member = consume("IDENTIFIER", "Ожидалось имя поля").getValue();
                    memberAccess->isPointerAccess = true;
                    ident = std::move(memberAccess);
                } else {
                    break;
                }
            }

            if (match("OPENPARENTHESES")) {
                auto call = std::make_unique<CallExpr>();
                // Получаем имя функции из выражения
                std::string calleeName;
                if (auto ident2 = dynamic_cast<Identifier *>(ident.get())) {
                    calleeName = ident2->name;
                } else if (auto memberAccess = dynamic_cast<MemberAccessExpr *>(ident.get())) {
                    calleeName = memberAccess->member;
                } else {
                    calleeName = "unknown";
                }
                call->callee = calleeName;
                if (!check("CLOSEPARENTHESES")) {
                    do {
                        call->arguments.push_back(parseExpression());
                    } while (match("COMMA"));
                }
                consume("CLOSEPARENTHESES", "Ожидалась ')' после аргументов");
                return call;
            }
            return ident;
        }

        throw std::runtime_error("Неожиданное выражение");
    }

    std::unique_ptr<Expr> makeUnary(const std::string &op, std::unique_ptr<Expr> operand, bool postfix) {
        if (!operand) {
            throw std::runtime_error("Операнд унарной операции не может быть nullptr");
        }
        auto node = std::make_unique<UnaryOp>();
        node->op = op;
        node->postfix = postfix;
        node->operand = std::move(operand);
        return node;
    }

    std::unique_ptr<Expr> makeBinary(std::unique_ptr<Expr> left, BinOpKind kind, std::unique_ptr<Expr> right) {
        if (!left || !right) {
            throw std::runtime_error("Операнды бинарной операции не могут быть nullptr");
        }
        auto node = std::make_unique<BinaryOp>();
        node->op = kind;
        node->left = std::move(left);
        node->right = std::move(right);
        return node;
    }

    bool isFunctionStart() {
        if (isAtEnd() || tokens.empty() || current >= tokens.size()) {
            return false;
        }

        size_t save = current;
        const size_t MAX_LOOKAHEAD = 100; // Защита от чрезмерного lookahead

        try {
            // Парсим тип возвращаемого значения
            std::string type = parseTypeName();
            
            if (current >= tokens.size()) {
                current = save;
                return false;
            }

            // Должен быть идентификатор
            if (isAtEnd() || current >= tokens.size() || !check("IDENTIFIER")) {
                current = save;
                return false;
            }

            // Должны быть скобки
            size_t next = current + 1;
            if (next >= tokens.size() || tokens[next].getType() != "OPENPARENTHESES") {
                current = save;
                return false;
            }

            // Проверяем, что это не вызов функции (после скобок должна быть { или ;)
            size_t afterParen = next + 1;
            int parenDepth = 1;
            int searchCount = 0;
            while (afterParen < tokens.size() && parenDepth > 0 && searchCount++ < MAX_LOOKAHEAD) {
                std::string tokenType = tokens[afterParen].getType();
                if (tokenType == "OPENPARENTHESES") {
                    parenDepth++;
                } else if (tokenType == "CLOSEPARENTHESES") {
                    parenDepth--;
                }
                if (parenDepth > 0) {
                    afterParen++;
                }
            }
            
            if (searchCount >= MAX_LOOKAHEAD) {
                // Не удалось найти закрывающую скобку в пределах разумного
                current = save;
                return false;
            }
            
            if (afterParen + 1 < tokens.size()) {
                std::string afterType = tokens[afterParen + 1].getType();
                if (afterType != "OPENCURLY" && afterType != "SEMICOLON") {
                    current = save;
                    return false;
                }
            }

            current = save;
            return true;
        } catch (const std::exception &e) {
            current = save;
            return false;
        }
    }

    bool isTypeToken(const std::string &type) {
        static const std::vector<std::string> typeTokens = {
                "VOID", "INT", "FLOAT", "DOUBLE", "CHAR", "SHORT", "LONG",
                "SIGNED", "UNSIGNED", "STRUCT", "CONST", "STATIC", "UNION"
        };
        return std::find(typeTokens.begin(), typeTokens.end(), type) != typeTokens.end();
    }

    bool isPreprocessor(const std::string &type) {
        return type == "INCLUDE" || type == "DEFINE" || type == "PREPROCESSOR";
    }

    bool match(const std::string &type) {
        if (check(type)) {
            std::cout << "Matched: " << type << " [" << peek().getValue() << "]" << std::endl;
            advance();
            return true;
        }
        return false;
    }

    bool check(const std::string &type) {
        if (tokens.empty() || current >= tokens.size()) {
            return false;
        }
        // isAtEnd() проверяет current >= tokens.size(), так что эта проверка избыточна, но оставляем для ясности
        return tokens[current].getType() == type;
    }

    Token advance() {
        if (tokens.empty() || current >= tokens.size()) {
            std::cerr << "[ERROR] [CParser] advance() called with invalid state: current=" 
                      << current << ", tokens.size()=" << tokens.size() << std::endl;
            static const Token dummy;
            return dummy;
        }
        if (isAtEnd()) {
            std::cerr << "[WARNING] [CParser] advance() called at end, current=" 
                      << current << ", tokens.size()=" << tokens.size() << std::endl;
            static const Token dummy;
            return dummy;
        }
        Token result = tokens[current++];
        return result;
    }

    Token &peek() {
        static Token dummy;
        if (tokens.empty() || current >= tokens.size()) {
            return dummy;
        }
        if (current >= tokens.size()) {
            std::cerr << "ERROR: current out of bounds in peek()" << std::endl;
            return dummy;
        }
        return tokens[current];
    }

    std::string peekType() {
        if (tokens.empty() || current >= tokens.size()) {
            static const std::string empty;
            return empty;
        }
        return tokens[current].getType();
    }

    Token previous() {
        static const Token dummy;
        if (tokens.empty() || current == 0) {
            return dummy;
        }
        return tokens[current - 1];
    }

    bool isAtEnd() const {
        bool atEnd = current >= tokens.size();
        if (atEnd) {
            std::cout << "Reached end of tokens at position " << current << std::endl;
        }
        return atEnd;
    }

    Token consume(const std::string &type, const std::string &message) {
        if (tokens.empty() || current >= tokens.size()) {
            throw std::runtime_error(message + " (reached end of tokens, current=" + 
                                   std::to_string(current) + ", tokens.size()=" + 
                                   std::to_string(tokens.size()) + ")");
        }
        if (check(type)) {
            return advance();
        }
        std::string actualType = current < tokens.size() ? tokens[current].getType() : "EOF";
        std::string actualValue = current < tokens.size() ? tokens[current].getValue() : "";
        throw std::runtime_error(message + " (got " + actualType + " [" + actualValue + 
                               "] at token " + std::to_string(current) + ")");
    }

    std::unique_ptr<Stmt> parseStructVarDecl() {
        size_t save = current;

        try {
            // Просто проверяем, что это идентификатор типа
            if (check("IDENTIFIER") && isStructType(peek().getValue())) {
                auto typeName = advance().getValue();

                // Должен быть идентификатор переменной
                if (!check("IDENTIFIER")) {
                    current = save;
                    return nullptr;
                }

                auto varDecl = std::make_unique<VarDeclStmt>();
                varDecl->typeName = typeName;
                varDecl->name = advance().getValue();

                // Проверяем массив
                while (match("OPENBRACKET")) {
                    varDecl->typeName += "[]";
                    if (!check("CLOSEBRACKET")) {
                        // Пропускаем размер массива
                        advance();
                    }
                    consume("CLOSEBRACKET", "Ожидался ']'");
                }

                if (match("ASSIGN")) {
                    varDecl->initializer = parseExpression();
                }

                consume("SEMICOLON", "Ожидался ';' после объявления структуры");
                return varDecl;
            }
            current = save;
            return nullptr;

        } catch (...) {
            current = save;
            return nullptr;
        }
    }

    std::unique_ptr<Stmt> parsePreprocessor() {
        if (isAtEnd()) return nullptr;

        auto pp = std::make_unique<PreprocessorDirective>();
        Token directiveToken = advance();  // Токен типа INCLUDE/DEFINE/PREPROCESSOR

        // Сохраняем полный текст директивы
        pp->directive = directiveToken.getValue();

        std::string tokenType = directiveToken.getType();

        if (tokenType == "INCLUDE") {
            // Обработка #include (оставляем как есть - работает)
            std::string fullValue = directiveToken.getValue();

            if (fullValue.find("<") != std::string::npos) {
                size_t start = fullValue.find("<");
                size_t end = fullValue.find(">");
                if (end != std::string::npos) {
                    pp->value = fullValue.substr(start + 1, end - start - 1);
                }
            } else if (fullValue.find("\"") != std::string::npos) {
                size_t start = fullValue.find("\"");
                size_t end = fullValue.find("\"", start + 1);
                if (end != std::string::npos) {
                    pp->value = fullValue.substr(start + 1, end - start - 1);
                }
            }
        } else if (tokenType == "DEFINE") {
            std::string macroName;
            std::string macroValue;

            // 1. Берём ИМЯ макроса (следующий токен)
            if (!isAtEnd() && check("IDENTIFIER")) {
                macroName = advance().getValue();

                // 2. Берём ЗНАЧЕНИЕ макроса (если есть)
                if (!isAtEnd() && !isPreprocessor(peekType()) &&
                    !isStatementStart(peekType())) {
                    macroValue = advance().getValue();
                }
            }

            // Формируем результат: "PI 3.1415926"
            pp->value = macroName;
            if (!macroValue.empty()) {
                pp->value += " " + macroValue;
            }

            std::cout << "DEBUG: #define " << macroName << " = " << macroValue << std::endl;
        } else {
            // Для других препроцессорных директив
            std::string content;

            while (!isAtEnd()) {
                std::string nextType = peekType();
                if (isPreprocessor(nextType)) {
                    break;
                }

                if (!content.empty()) {
                    content += " ";
                }
                content += advance().getValue();
            }

            pp->value = content;
        }

        std::cout << "Parsed preprocessor: " << pp->directive << " -> " << pp->value << std::endl;
        return pp;
    }

    bool isStatementStart(const std::string &type) {
        static const std::vector<std::string> statementStarts = {
                "INT", "VOID", "FLOAT", "DOUBLE", "CHAR", "SHORT", "LONG",
                "STRUCT", "TYPEDEF", "CONST", "STATIC",
                "IF", "WHILE", "FOR", "DO", "SWITCH", "RETURN", "BREAK",
                "IDENTIFIER", "OPENCURLY", "SEMICOLON"
        };
        return std::find(statementStarts.begin(), statementStarts.end(), type) != statementStarts.end();
    }
};

#endif // SIMPLE_CPP_PARSER_H