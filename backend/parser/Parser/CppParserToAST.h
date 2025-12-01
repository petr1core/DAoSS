//
// Created by пк on 19.11.2025.
//

#ifndef AISDLAB_CPPPARSERTOAST_H
#define AISDLAB_CPPPARSERTOAST_H

#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <iostream>
#include <unordered_set>
#include <unordered_map>
#include "../CPPAst/CppAst.h"
#include "../CPPAst/CppExpr.h"
#include "../CPPAst/CppStmt.h"
#include "../CPPAst/CppDecl.h"
#include "../CPPAst/CppVisitor.h"
#include "../CPPAst/CppExprAcceptImpl.h"
#include "../Scripts/Lexer.h"
#include "../Scripts/Token.h"

class CppParserToAST {
public:
    std::unique_ptr<CppProgram> parse(const std::string &code) {
        Lexer lexer(code, types::CPP);
        tokens = lexer.getTokenList();
        lexer.printTokenList();

        // Фильтруем пробелы и комментарии
        std::vector<Token> filtered;
        filtered.reserve(tokens.size());
        for (const auto &token: tokens) {
            if (token.getType() == "SPACE" || token.getType() == "COMMENT") {
                continue;
            }
            filtered.push_back(token);
        }
        tokens.swap(filtered);
        current = 0;

        auto program = std::make_unique<CppProgram>();
        program->name = "test_program_by_CPP";

        if (tokens.empty()) {
            program->body = std::make_unique<CppCompoundStmt>();
            return program;
        }

        program->body = parseTranslationUnit();
        return program;
    }

private:
    std::vector<Token> tokens;
    size_t current{0};
    size_t recursionDepth{0};
    static const size_t MAX_RECURSION_DEPTH = 100;
    // Контекст парсера
    std::vector<std::string> currentNamespace;
    std::vector<CppTemplateParameter> currentTemplateParams;
    CppClassDecl *currentClass{nullptr};
    bool inTemplate{false};
    std::unordered_set<std::string> knownClasses;
    std::unordered_set<std::string> knownTypedefs;
    std::unordered_set<std::string> currentAccessScope;

    void enterRecursion() {
        if (++recursionDepth > MAX_RECURSION_DEPTH) {
            throw std::runtime_error("Maximum recursion depth exceeded");
        }
    }

    void exitRecursion() {
        --recursionDepth;
    }

    // ============ ОСНОВНОЙ ПАРСИНГ ============

    std::unique_ptr<CppCompoundStmt> parseTranslationUnit() {
        auto block = std::make_unique<CppCompoundStmt>();

        while (!isAtEnd()) {
            std::string tokenType = peekType();

            std::cout << "DEBUG: TranslationUnit at: " << tokenType
                      << " [" << peek().getValue() << "]"
                      << " (current=" << current << "/" << tokens.size() << ")" << std::endl;



            // 🔥 ПЕРЕМЕСТИТЕ ЭТУ ПРОВЕРКУ СЮДА - перед isGlobalVarStart()
            if (check("IDENTIFIER") && knownClasses.count(peek().getValue()) > 0) {
                std::cout << "DEBUG: Found user type declaration: " << peek().getValue() << std::endl;
                auto var = parseGlobalVar();
                if (var) {
                    block->statements.push_back(std::move(var));
                    std::cout << "DEBUG: Successfully parsed user type var" << std::endl;
                    continue;
                }
            }

            // Пропускаем проблемные токены в начале
            if (!isDeclarationStart(tokenType) && !isPreprocessor(tokenType)) {
                std::cout << "Skipping non-declaration token: " << tokenType
                          << " [" << peek().getValue() << "]" << std::endl;
                advance();
                continue;
            }

            // Препроцессор
            if (isPreprocessor(tokenType)) {
                auto pp = parsePreprocessor();
                if (pp) block->statements.push_back(std::move(pp));
                continue;
            }

            // Пространства имен
            if (match("NAMESPACE")) {
                auto ns = parseNamespace();
                if (ns) block->statements.push_back(std::move(ns));
                continue;
            }

            // Шаблоны - пропускаем для упрощения
            if (match("TEMPLATE")) {
                std::cout << "Skipping template" << std::endl;
                skipUntilSemicolonOrBrace();
                continue;
            }

            // Классы и структуры
            if (match("CLASS") || match("STRUCT")) {
                std::cout << "DEBUG: Starting to parse class/struct at position " << current << std::endl;
                auto classDecl = parseClass();
                if (classDecl) {
                    std::string className = classDecl->name;
                    block->statements.push_back(std::move(classDecl));
                    knownClasses.insert(className);
                    std::cout << "DEBUG: Successfully added class to AST" << std::endl;
                } else {
                    std::cout << "DEBUG: Failed to parse class/struct" << std::endl;
                }
                continue;
            }

            // Перечисления
            if (match("ENUM")) {
                auto enumDecl = parseEnum();
                if (enumDecl) block->statements.push_back(std::move(enumDecl));
                continue;
            }

            if (match("UNION")) {
                auto unionDecl = parseUnion();
                if (unionDecl) block->statements.push_back(std::move(unionDecl));
                continue;
            }

            // typedef и using
            if (match("TYPEDEF")) {
                auto typedefDecl = parseTypedef();
                if (typedefDecl) block->statements.push_back(std::move(typedefDecl));
                continue;
            }

            if (match("USING")) {
                auto usingDecl = parseUsing();
                if (usingDecl) block->statements.push_back(std::move(usingDecl));
                continue;
            }

            // Глобальные переменные
            if (isGlobalVarStart()) {
                std::cout << "DEBUG: Found global var start: " << peek().getValue() << std::endl;
                auto var = parseGlobalVar();
                if (var) {
                    block->statements.push_back(std::move(var));
                    std::cout << "DEBUG: Successfully parsed global var" << std::endl;
                } else {
                    std::cout << "DEBUG: Failed to parse global var, skipping..." << std::endl;
                    skipUntilSemicolon();
                }
                continue;
            }

            // Функции
            if (isFunctionStart()) {
                auto func = parseFunction();
                if (func) block->statements.push_back(std::move(func));
                continue;
            }

            // Если ничего не распарсилось, пропускаем токен
            std::cout << "Skipping token in translation unit: " << tokenType
                      << " [" << peek().getValue() << "]" << std::endl;
            advance();
        }

        std::cout << "DEBUG: Finished parsing translation unit successfully!" << std::endl;
        return block;
    }

    void skipUntilSemicolon() {
        int braceLevel = 0;
        int parenLevel = 0;

        while (!isAtEnd()) {
            if (check("OPENCURLY")) braceLevel++;
            else if (check("CLOSECURLY")) {
                if (braceLevel == 0) break;
                braceLevel--;
            }
            else if (check("OPENPARENTHESES")) parenLevel++;
            else if (check("CLOSEPARENTHESES")) {
                if (parenLevel == 0) break;
                parenLevel--;
            }
            else if (check("SEMICOLON") && braceLevel == 0 && parenLevel == 0) {
                advance(); // пропускаем точку с запятой
                return;
            }

            std::cout << "Skipping in skipUntilSemicolon: " << peek().getValue() << std::endl;
            advance();
        }
    }

    void skipUntilSemicolonOrBrace() {
        int braceLevel = 0;
        int parenLevel = 0;

        while (!isAtEnd()) {
            if (check("OPENCURLY")) {
                braceLevel++;
            } else if (check("CLOSECURLY")) {
                if (braceLevel == 0) {
                    std::cout << "DEBUG: Reached closing brace, stopping skip" << std::endl;
                    return; // НЕ потребляем закрывающую скобку!
                }
                braceLevel--;
            }else if (check("OPENPARENTHESES")) {
                parenLevel++;
            } else if (check("CLOSEPARENTHESES")) {
                if (parenLevel > 0) parenLevel--;
            }  else if (check("SEMICOLON") && braceLevel == 0 && parenLevel == 0) {
                std::cout << "DEBUG: Reached semicolon, stopping skip" << std::endl;
                advance(); // пропускаем точку с запятой
                return;
            }
            std::cout << "Skipping in skipUntilSemicolonOrBrace: " << peek().getValue() << std::endl;
            advance();
        }
    }

    Token lookAhead(size_t offset = 1) {
        size_t pos = current + offset;
        return pos < tokens.size() ? tokens[pos] : Token();
    }

    std::unique_ptr<CppDecl> parseDeclaration() {
        if (isAtEnd()) return nullptr;

        // Препроцессор
        if (isPreprocessor(peekType())) {
            return parsePreprocessor();
        }

        // Пространства имен
        if (match("NAMESPACE")) {
            return parseNamespace();
        }

        // Шаблоны
        if (match("TEMPLATE")) {
            return parseTemplate();
        }

        // Классы и структуры
        if (match("CLASS") || match("STRUCT")) {
            return parseClass();
        }

        // Перечисления
        if (match("ENUM")) {
            return parseEnum();
        }

        // typedef и using
        if (match("TYPEDEF")) {
            return parseTypedef();
        }

        if (match("USING")) {
            return parseUsing();
        }

        // Глобальные переменные
        if (isGlobalVarStart()) {
            return parseGlobalVarAsDecl();
        }


        // Функции
        if (isFunctionStart()) {
            return parseFunction();
        }

        // Если ничего не распарсилось, пробуем как выражение
        try {
            auto expr = parseExpression();
            if (expr && match("SEMICOLON")) {
                // Для выражений возвращаем nullptr, так как они не являются объявлениями
                return nullptr;
            }
        } catch (...) {
            // Игнорируем ошибки выражений
        }

        // Пропускаем проблемный токен
        std::cout << "Skipping token in declaration: " << peek().getValue() << std::endl;
        advance();
        return nullptr;
    }

    std::unique_ptr<CppDecl> parseGlobalVarAsDecl() {
        try {
            auto decl = std::make_unique<CppVarDecl>();
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

            return std::unique_ptr<CppDecl>(dynamic_cast<CppDecl *>(decl.release()));

        } catch (const std::exception &e) {
            std::cerr << "Error in parseGlobalVarAsDecl: " << e.what() << std::endl;
            return nullptr;
        }
    }


    std::unique_ptr<CppTypedefDecl> parseTypedef() {
        auto typedefDecl = std::make_unique<CppTypedefDecl>();
        // Упрощенная реализация
        typedefDecl->typeName = parseTypeName();
        if (check("IDENTIFIER")) {
            typedefDecl->alias = advance().getValue();
        }
        consume("SEMICOLON", "Expected ';' after typedef");
        return typedefDecl;
    }

    std::unique_ptr<CppUsingDecl> parseUsing() {
        auto usingDecl = std::make_unique<CppUsingDecl>();
        // Упрощенная реализация
        if (match("NAMESPACE")) {
            usingDecl->isNamespace = true;
            if (check("IDENTIFIER")) {
                usingDecl->target = advance().getValue();
            }
        } else {
            usingDecl->name = parseTypeName();
        }
        consume("SEMICOLON", "Expected ';' after using");
        return usingDecl;
    }

    // ============ ПРЕПРОЦЕССОР ============

    std::unique_ptr<CppPreprocessorDirective> parsePreprocessor() {
        auto pp = std::make_unique<CppPreprocessorDirective>();
        Token directive = advance();
        pp->directive = directive.getValue();

        std::string content;
        while (!isAtEnd() && !isPreprocessor(peekType()) &&
               !isDeclarationStart(peekType())) {
            if (!content.empty()) content += " ";
            content += advance().getValue();
        }
        pp->value = content;

        return pp;
    }

    // ============ ОБЪЯВЛЕНИЯ ============

    std::unique_ptr<CppNamespaceDecl> parseNamespace() {
        auto ns = std::make_unique<CppNamespaceDecl>();

        if (check("IDENTIFIER")) {
            ns->name = advance().getValue();
        } else {
            ns->name = ""; // anonymous namespace
        }

        consume("OPENCURLY", "Expected '{' after namespace");
        ns->body = std::unique_ptr<CppCompoundStmt>(static_cast<CppCompoundStmt *>(parseCompoundStmt().release()));
        return ns;
    }

    std::unique_ptr<CppTemplateDecl> parseTemplate() {
        auto templ = std::make_unique<CppTemplateDecl>();

        consume("JL", "Expected '<' after template");

        // Упрощенный парсинг параметров шаблона
        while (!isAtEnd() && !check("JG")) {
            if (match("TYPENAME") || match("CLASS")) {
                CppTemplateParameter param;
                param.kind = "type";
                if (check("IDENTIFIER")) {
                    param.name = advance().getValue();
                }
                templ->parameters.push_back(param);
            }

            if (match("COMMA")) continue;
            if (check("JG")) break;
            advance(); // пропускаем неизвестные токены
        }

        if (check("CLOSEANGLE")) {
            advance();
        }

        consume("JG", "Expected '>' after template parameters");

        // Парсим объявление после шаблона
        if (isDeclarationStart(peekType())) {
            templ->declaration = parseDeclaration();
        }

        return templ;
    }

    std::unique_ptr<CppClassDecl> parseClass() {

        std::cout << "DEBUG: parseClass() called at position " << current
                  << ", token: " << peek().getValue()
                  << ", currentClass: " << std::endl;

        auto classDecl = std::make_unique<CppClassDecl>();
        classDecl->isStruct = previous().getType() == "STRUCT";

        if (check("IDENTIFIER")) {
            classDecl->name = advance().getValue();
            knownClasses.insert(classDecl->name);
            std::cout << "DEBUG: Parsing class/struct: " << classDecl->name << std::endl;
        }
        // 🔥 ДОБАВЛЯЕМ ОБРАБОТКУ НАСЛЕДОВАНИЯ
        if (match("COLON")) {
            std::cout << "DEBUG: Found inheritance list" << std::endl;
            do {
                // Спецификатор доступа наследования
                CppAccessSpecifier access = CppAccessSpecifier::Private;
                if (match("PUBLIC")) {
                    access = CppAccessSpecifier::Public;
                } else if (match("PROTECTED")) {
                    access = CppAccessSpecifier::Protected;
                } else if (match("PRIVATE")) {
                    access = CppAccessSpecifier::Private;
                }

                // Базовый класс
                std::string baseClass = parseTypeName();
                classDecl->baseClasses.push_back(baseClass);
                classDecl->baseAccess.push_back(access);

                std::cout << "DEBUG: Added base class: " << baseClass
                          << " with access: " << (access == CppAccessSpecifier::Public ? "public" :
                                                  access == CppAccessSpecifier::Protected ? "protected" : "private")
                          << std::endl;

            } while (match("COMMA"));
        }
        if (!match("OPENCURLY")) {
            throw std::runtime_error("Expected '{' after class");
        }

        CppClassDecl *previousClass = currentClass;
        currentClass = classDecl.get();
        CppAccessSpecifier currentAccess = classDecl->isStruct ?
                                           CppAccessSpecifier::Public : CppAccessSpecifier::Private;

        while (!isAtEnd() && !check("CLOSECURLY")) {
            if (isAtEnd()) break;

            std::cout << "DEBUG: Class parsing at: " << peek().getValue() << " (" << peek().getType() << ")"
                      << std::endl;

            // Спецификаторы доступа
            if (match("PUBLIC") || match("PROTECTED") || match("PRIVATE")) {
                if (previous().getType() == "PUBLIC") currentAccess = CppAccessSpecifier::Public;
                else if (previous().getType() == "PROTECTED") currentAccess = CppAccessSpecifier::Protected;
                else currentAccess = CppAccessSpecifier::Private;
                consume("COLON", "Expected ':' after access specifier");
                continue;
            }
            // 🔥 ПЕРВОЕ: проверяем деструкторы
            if (check("BITNOT") && lookAhead(1).getType() == "IDENTIFIER" &&
                currentClass && lookAhead(1).getValue() == currentClass->name) {
                std::cout << "DEBUG: Trying to parse destructor: ~" << lookAhead(1).getValue() << std::endl;
                try {
                    auto dtor = parseDestructor();
                    if (dtor) {
                        auto method = std::make_unique<CppMethodDecl>();
                        method->returnType = dtor->returnType;
                        method->name = dtor->name;
                        method->parameters = std::move(dtor->parameters);
                        method->body = std::move(dtor->body);
                        method->access = currentAccess;
                        method->isVirtual = dtor->isVirtual;
                        method->isOverride = dtor->isOverride;
                        method->isConst = dtor->isConst;
                        classDecl->members.push_back(std::unique_ptr<CppDecl>(method.release()));
                        std::cout << "DEBUG: Successfully parsed destructor" << std::endl;
                        continue;
                    }
                } catch (const std::exception &e) {
                    std::cout << "DEBUG: Destructor parsing failed: " << e.what() << std::endl;
                    // Продолжаем парсинг вместо пропуска
                }
            }
            // 🔥 ВТОРОЕ: проверяем конструкторы
            if (check("IDENTIFIER") && currentClass && peek().getValue() == currentClass->name) {
                std::cout << "DEBUG: Parsing constructor: " << peek().getValue() << std::endl;
                try {
                    auto ctor = parseConstructor();
                    if (ctor) {
                        // 🔥 ПРАВИЛЬНОЕ ПРЕОБРАЗОВАНИЕ В CppConstructorDecl
                        if (auto funcDecl = dynamic_cast<CppFunctionDecl*>(ctor.get())) {
                            auto constructor = std::make_unique<CppConstructorDecl>();
                            constructor->name = funcDecl->name;
                            constructor->parameters = std::move(funcDecl->parameters);
                            constructor->body = std::move(funcDecl->body);
                            constructor->access = currentAccess;
                            std::cout<<"TODO: добавить парсинг списка инициализации";

                            classDecl->members.push_back(std::unique_ptr<CppDecl>(constructor.release()));
                            std::cout << "DEBUG: Successfully converted to CppConstructorDecl" << std::endl;
                        } else {
                            classDecl->members.push_back(std::move(ctor));
                        }
                        std::cout << "DEBUG: Successfully parsed constructor" << std::endl;
                        continue;
                    }
                } catch (const std::exception &e) {
                    std::cout << "DEBUG: Constructor parsing failed: " << e.what() << std::endl;
                    // Продолжаем парсинг вместо пропуска
                }
            }
            // 🔥 ТРЕТЬЕ: проверяем статические методы
            if (match("STATIC")) {
                std::cout << "DEBUG: Found static, trying to parse method" << std::endl;
                if (isFunctionStart()) {
                    try {
                        auto func = parseFunction();
                        if (func) {
                            auto method = std::make_unique<CppMethodDecl>();
                            method->returnType = func->returnType;
                            method->name = func->name;
                            method->parameters = std::move(func->parameters);
                            method->body = std::move(func->body);
                            method->isStatic = true;
                            method->access = currentAccess;
                            method->isVirtual = func->isVirtual;
                            method->isOverride = func->isOverride;
                            method->isConst = func->isConst;
                            classDecl->members.push_back(std::unique_ptr<CppDecl>(method.release()));
                            std::cout << "DEBUG: Successfully parsed static method: " << method->name << std::endl;
                            continue;
                        }
                    } catch (const std::exception &e) {
                        std::cout << "DEBUG: Static method parsing failed: " << e.what() << std::endl;
                        current--; // откатываем STATIC
                    }
                } else {// Если не функция, возможно статическое поле
                    current--; // откатываемся назад
                }
            }
            // 🔥 ЧЕТВЕРТОЕ: проверяем обычные методы и операторы
            if (isFunctionStart()) {
                std::cout << "DEBUG: Trying to parse method/operator: " << peek().getValue() << std::endl;
                size_t funcSave = current;
                try {
                    auto func = parseFunction();
                    if (func) {
                        std::cout << "DEBUG: Successfully parsed function: " << func->name << std::endl;
                        if (func->name.find("operator") == 0) {
                            // Это оператор
                            std::cout << "DEBUG: Converting to CppOperatorDecl: " << func->name << std::endl;
                            auto op = std::make_unique<CppOperatorDecl>();
                            op->returnType = func->returnType;
                            op->operatorSymbol = func->name.substr(8); // убираем "operator"
                            op->parameters = std::move(func->parameters);
                            op->body = std::move(func->body);
                            op->isConst = func->isConst;
                            op->access = currentAccess;
                            std::cout << "DEBUG: Created operator: " << op->operatorSymbol << std::endl;
                            classDecl->members.push_back(std::unique_ptr<CppDecl>(op.release()));
                            std::cout << "DEBUG: Successfully parsed operator: " << op->operatorSymbol << std::endl;
                        } else {
                            // Обычный метод
                            std::cout << "DEBUG: Converting to CppMethodDecl: " << func->name << std::endl;
                            auto method = std::make_unique<CppMethodDecl>();
                            method->returnType = func->returnType;
                            method->name = func->name;
                            method->parameters = std::move(func->parameters);
                            method->body = std::move(func->body);
                            method->isVirtual = func->isVirtual;
                            method->isOverride = func->isOverride;
                            method->isConst = func->isConst;
                            method->access = currentAccess;
                            std::cout << "DEBUG: Created method: " << method->name << std::endl;
                            std::unique_ptr<CppDecl> methodAsDecl(method.release());
                            classDecl->members.push_back(std::move(methodAsDecl));
                            std::cout << "DEBUG: Successfully added method to class" << std::endl;
                        }
                        continue;
                    }
                } catch (const std::exception &e) {
                    current = funcSave;
                    std::cout << "DEBUG: Method/operator parsing failed: " << e.what() << std::endl;
                }
            }

            // 🔥 ПЯТОЕ: дружественные функции (пропускаем для упрощения)
            if (match("FRIEND")) {
                std::cout << "DEBUG: Skipping friend declaration: " << peek().getValue() << std::endl;
                skipUntilSemicolon();
                continue;
            }
            // 🔥 ШЕСТОЕ: специальная обработка операторов (они могут не иметь возвращаемого типа)
            if (check("OPERATOR")) {
                std::cout << "DEBUG: Temporarily skipping operator: " << peek().getValue() << std::endl;
                skipUntilSemicolonOrBrace();
                continue;
            }
            /* if (check("OPERATOR")) {
                std::cout << "DEBUG: Found operator keyword, trying to parse operator" << std::endl;
                size_t opSave = current;

                try {
                    // Пропускаем "operator" и парсим оператор
                    advance(); // operator

                    // Собираем символ оператора
                    std::string opSymbol;
                    if (check("ASSIGN") || check("PLUS") || check("MINUS") || check("MULTI") ||
                        check("DIV") || check("MOD") || check("JE") || check("JNE") ||
                        check("JL") || check("JG") || check("JLE") || check("JGE") ||
                        check("INCREMENT") || check("DECREMENT") || check("BITSHIFTLEFT") ||
                        check("BITSHIFTRIGHT") || check("BITAND") || check("BITOR") ||
                        check("BITXOR") || check("BITNOT") || check("AND") || check("OR") ||
                        check("OPENBRACKET") || check("CLOSEBRACKET") || check("OPENPARENTHESES") ||
                        check("CLOSEPARENTHESES") || check("MEMBERACCESS") || check("PTRACCESS") ||
                        check("NEW") || check("DELETE")) {

                        opSymbol = advance().getValue();
                    } else {
                        throw std::runtime_error("Unsupported operator");
                    }

                    // Проверяем, что дальше идет список параметров
                    if (!check("OPENPARENTHESES")) {
                        throw std::runtime_error("Expected '(' after operator");
                    }

                    // Парсим как обычную функцию с пустым возвращаемым типом
                    auto op = std::make_unique<CppOperatorDecl>();
                    op->returnType = ""; // операторы могут иметь разный возвращаемый тип
                    op->operatorSymbol = opSymbol;
                    op->access = currentAccess;

                    // Параметры
                    consume("OPENPARENTHESES", "Expected '(' after operator");
                    op->parameters = parseParameters();
                    consume("CLOSEPARENTHESES", "Expected ')' after parameters");

                    // Квалификаторы
                    if (match("CONST")) {
                        op->isConst = true;
                    }

                    // Тело оператора
                    if (match("OPENCURLY")) {
                        op->body = std::unique_ptr<CppCompoundStmt>(
                                static_cast<CppCompoundStmt*>(parseCompoundStmt().release()));
                    } else if (match("SEMICOLON")) {
                        // Объявление оператора без тела
                    } else {
                        throw std::runtime_error("Expected operator body or ';'");
                    }

                    classDecl->members.push_back(std::unique_ptr<CppDecl>(op.release()));
                    std::cout << "DEBUG: Successfully parsed operator: " << opSymbol << std::endl;
                    continue;

                } catch (const std::exception& e) {
                    current = opSave;
                    std::cout << "DEBUG: Operator parsing failed: " << e.what() << std::endl;
                }
            }*/
            // 🔥 СЕДЬМОЕ: пробуем распарсить поле (ОСНОВНОЙ ПУТЬ)
            size_t save = current;
            try {
                auto fieldOrBlock = parseFieldDecl();
                if (fieldOrBlock) {
                    // 🔥 ОБРАБАТЫВАЕМ КАК ОДИНОЧНОЕ ПОЛЕ ИЛИ БЛОК
                    if (auto singleField = dynamic_cast<CppFieldDecl *>(fieldOrBlock.get())) {
                        singleField->access = currentAccess;
                        classDecl->members.push_back(std::move(fieldOrBlock));
                        std::cout << "DEBUG: Added single field: " << singleField->name << std::endl;
                    } else if (auto block = dynamic_cast<CppCompoundStmt *>(fieldOrBlock.get())) {
                        // 🔥 ДОБАВЛЯЕМ ВСЕ ПОЛЯ ИЗ БЛОКА
                        for (auto &stmt: block->statements) {
                            if (auto field = dynamic_cast<CppFieldDecl *>(stmt.get())) {
                                auto fieldCopy = std::make_unique<CppFieldDecl>();
                                fieldCopy->typeName = field->typeName;
                                fieldCopy->name = field->name;
                                fieldCopy->access = currentAccess;
                                classDecl->members.push_back(std::move(fieldCopy));
                                std::cout << "DEBUG: Added field from multiple: " << field->name << std::endl;
                            }
                        }
                    }
                    continue;
                }
            } catch (const std::exception &e) {
                current = save;
                std::cout << "DEBUG: Field parsing failed: " << e.what() << std::endl;
            }

            //если не поле и не конструктор, пропускаем ОДИН токен
            std::cout << "DEBUG: Skipping single token: " << peek().getValue() << std::endl;
            advance();
        }

        if (isAtEnd()) {
            throw std::runtime_error("Unexpected end of file while parsing class");
        }

        std::cout << "DEBUG: Before consuming '}' - current: " << current
                  << ", token: " << peek().getValue() << std::endl;

        consume("CLOSECURLY", "Expected '}' after class body");

        std::cout << "DEBUG: After consuming '}' - current: " << current
                  << ", token: " << (isAtEnd() ? "END" : peek().getValue()) << std::endl;

        // Безопасная проверка точки с запятой после класса
        if (check("SEMICOLON")) {
            advance();
            std::cout << "DEBUG: Consumed semicolon after class" << std::endl;
        } else {
            std::cout << "DEBUG: WARNING: No semicolon after class, but continuing" << std::endl;
            // Не бросаем исключение, а просто продолжаем
        }
        std::cout << "DEBUG: After consuming ';' - current: " << current
                  << ", token: " << (isAtEnd() ? "END" : peek().getValue()) << std::endl;

        std::cout << "DEBUG: Finished parsing class: " << classDecl->name
                  << " with " << classDecl->members.size() << " fields" << std::endl;

        currentClass = previousClass;

        return classDecl;
    }

    void skipComplexMember() {
        if (isAtEnd()) return;

        std::cout << "DEBUG: Skipping complex class member: " << peek().getValue() << std::endl;

        int braceLevel = 0;
        int parenLevel = 0;


        while (!isAtEnd()) {

            std::string tokenType = peekType();
            std::string tokenValue = peek().getValue();

            // 🔥 КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ: останавливаемся на ЛЮБОЙ закрывающей скобке на верхнем уровне
            if (tokenType == "CLOSECURLY" && braceLevel == 0 && parenLevel == 0) {
                std::cout << "DEBUG: Reached class closing brace, stopping skip (IMPORTANT!)" << std::endl;
                return; // НЕ потребляем закрывающую скобку!
            }
            // 🔥 ИЛИ если видим ";" на верхнем уровне - СТОП
            if (tokenType == "SEMICOLON" && braceLevel == 0 && parenLevel == 0) {
                std::cout << "DEBUG: SIMPLE skip - reached semicolon" << std::endl;
                advance();
                return;
            }


            // Отслеживаем уровни вложенности
            if (tokenType == "OPENPARENTHESES") {
                parenLevel++;
            } else if (tokenType == "CLOSEPARENTHESES") {
                if (parenLevel > 0) parenLevel--;
            } else if (tokenType == "OPENCURLY") {
                braceLevel++;
            } else if (tokenType == "CLOSECURLY") {
                if (braceLevel > 0) braceLevel--;
            } else if (tokenType == "SEMICOLON" && braceLevel == 0 && parenLevel == 0) {
                std::cout << "DEBUG: Reached semicolon, stopping skip" << std::endl;
                advance();
                return;
            }

            std::cout << "Skipping: " << tokenValue << " (braceLevel=" << braceLevel
                      << ", parenLevel=" << parenLevel << ")" << std::endl;
            advance();
        }
    }

    Token previous() {
        static Token dummy;
        if (current == 0 || tokens.empty()) return dummy;
        return tokens[current - 1];
    }

    std::unique_ptr<CppFunctionDecl> parseConstructor() {
        auto func = std::make_unique<CppFunctionDecl>();
        func->name = advance().getValue(); // имя конструктора
        func->returnType = ""; // конструкторы не имеют возвращаемого типа

        // Параметры
        if (!match("OPENPARENTHESES")) {
            throw std::runtime_error("Expected '(' after constructor name");
        }

        func->parameters = parseParameters();

        if (!match("CLOSEPARENTHESES")) {
            throw std::runtime_error("Expected ')' after parameters");
        }

        // 🔥 ОБРАБОТКА СПИСКА ИНИЦИАЛИЗАЦИИ
        if (match("COLON")) {
            std::cout << "DEBUG: Parsing initializer list" << std::endl;
            do {
                if (check("IDENTIFIER")) {
                    std::string memberName = advance().getValue();

                    if (match("OPENPARENTHESES")) {
                        // Пропускаем аргументы инициализации для упрощения
                        while (!isAtEnd() && !check("CLOSEPARENTHESES")) {
                            advance();
                        }
                        consume("CLOSEPARENTHESES", "Expected ')' after initializer");
                    }
                }
            } while (match("COMMA"));
        }

        // Тело конструктора
        if (match("OPENCURLY")) {
            func->body = std::unique_ptr<CppCompoundStmt>(
                    static_cast<CppCompoundStmt *>(parseCompoundStmt().release()));
        } else if (match("SEMICOLON")) {
            // Объявление конструктора без тела
        } else {
            throw std::runtime_error("Expected constructor body or ';'");
        }

        return func;
    }

    std::unique_ptr<CppFunctionDecl> parseDestructor() {
        auto func = std::make_unique<CppFunctionDecl>();
        match("BITNOT"); // пропускаем ~
        func->name = "~" + advance().getValue(); // имя деструктора
        func->returnType = ""; // деструкторы не имеют возвращаемого типа

        std::cout << "DEBUG: Parsing destructor parameters for: " << func->name << std::endl;

        // Параметры (у деструктора нет параметров)
        if (!match("OPENPARENTHESES")) {
            throw std::runtime_error("Expected '(' after destructor name");
        }

        // У деструктора могут быть пустые параметры или с void
        if (!check("CLOSEPARENTHESES")) {
            // Если есть параметры, пропускаем их для упрощения
            skipUntilCommaOrParen();
        }

        if (!match("CLOSEPARENTHESES")) {
            throw std::runtime_error("Expected ')' after destructor parameters");
        }

        // Квалификаторы
        if (match("OVERRIDE")) {
            func->isOverride = true;
        }
        if (match("FINAL")) {
            // final qualifier
        }

        std::cout << "DEBUG: Parsing destructor body for: " << func->name << std::endl;


        // Тело деструктора
        if (match("OPENCURLY")) {
            func->body = std::unique_ptr<CppCompoundStmt>(
                    static_cast<CppCompoundStmt *>(parseCompoundStmt().release()));
        } else if (match("SEMICOLON")) {
            // Объявление деструктора без тела
        } else {
            throw std::runtime_error("Expected destructor body or ';'");
        }
        std::cout << "DEBUG: Successfully parsed destructor: " << func->name << std::endl;

        return func;
    }

    std::unique_ptr<CppDecl> parseFieldDecl() {  // 🔥 ИЗМЕНИТЬ ВОЗВРАЩАЕМЫЙ ТИП
        size_t save = current;

        try {
            // Парсим общий тип для всех объявлений
            std::string commonType = parseTypeName();

            if (!check("IDENTIFIER")) {
                current = save;
                return nullptr;
            }

            // Первое объявление
            auto firstField = std::make_unique<CppFieldDecl>();
            firstField->typeName = commonType;
            firstField->name = advance().getValue();

            // 🔥 ОБРАБАТЫВАЕМ ДОПОЛНИТЕЛЬНЫЕ ОБЪЯВЛЕНИЯ ЧЕРЕЗ ЗАПЯТУЮ
            while (match("COMMA")) {
                if (!check("IDENTIFIER")) break;

                // Пропускаем дополнительные имена, но не создаем для них поля
                // (для упрощения сохраняем только первое поле)
                advance(); // пропускаем имя
            }

            // Пропускаем инициализаторы если есть
            if (match("ASSIGN")) {
                skipUntilCommaOrSemicolon();
            }

            if (!match("SEMICOLON")) {
                throw std::runtime_error("Expected ';' after field declaration");
            }

            // 🔥 ВОЗВРАЩАЕМ КАК CppDecl (корректный тип)
            return std::unique_ptr<CppDecl>(firstField.release());

        } catch (const std::exception &e) {
            current = save;
            return nullptr;
        }
    }

    std::unique_ptr<CppFunctionDecl> parseFunction() {

        std::cout << "DEBUG: Starting parseFunction at token: " << peek().getValue() << std::endl;

        auto func = std::make_unique<CppFunctionDecl>();

        // Возвращаемый тип
        try {
            func->returnType = parseTypeName();
            std::cout << "DEBUG: Parsed return type: " << func->returnType << std::endl;
        } catch (const std::exception &e) {
            std::cout << "DEBUG: Failed to parse return type: " << e.what() << std::endl;
            func->returnType = "";
        }

        // Имя функции
        if (check("IDENTIFIER") || check("OPERATOR")) {
            func->name = advance().getValue();
            std::cout << "DEBUG: Function name: " << func->name << std::endl;
        } else {
            std::cout << "DEBUG: Expected function name, got: " << peek().getValue() << std::endl;
            return nullptr;
        }


//        // Шаблонные специализации
//        if (match("OPENANGLE")) {
//            // Пропускаем template arguments для упрощения
//            while (!isAtEnd() && !check("CLOSEANGLE")) {
//                advance();
//            }
//            consume("CLOSEANGLE", "Expected '>'");
//        }

        // Параметры
        if (!match("OPENPARENTHESES")) {
            std::cout << "DEBUG: Expected '(', got: " << peek().getValue() << std::endl;
            throw std::runtime_error("Expected '(' after function name");
        }


        std::cout << "DEBUG: Parsing parameters..." << std::endl;

        func->parameters = parseParameters();

        if (!match("CLOSEPARENTHESES")) {
            std::cout << "DEBUG: Expected ')', got: " << peek().getValue() << std::endl;
            throw std::runtime_error("Expected ')' after parameters");
        }

        // Квалификаторы
        if (match("CONST")) {
            func->isConst = true;
        }
        if (match("VOLATILE")) {
            // volatile qualifier
        }
        if (match("NOEXCEPT")) {
            //func->isNoexcept = true;
            if (match("OPENPARENTHESES")) {
                // Пропускаем condition
                parseExpression();
                consume("CLOSEPARENTHESES", "Expected ')' after noexcept");
            }
        }
        if (match("OVERRIDE")) {
            func->isOverride = true;
        }
        if (match("FINAL")) {
            //func->isFinal = true;
        }

        // Тело функции
//        if (match("ASSIGN")) {
//            if (match("ZERO")) { // = 0 (pure virtual)
//                // pure virtual function
//            } else if (match("DEFAULT")) { // = default
//                // defaulted function
//            } else if (match("DELETE")) { // = delete
//                // deleted function
//            }
//            consume("SEMICOLON", "Expected ';'");
//        } else
        if (match("OPENCURLY")) {
            func->body = std::unique_ptr<CppCompoundStmt>(
                    static_cast<CppCompoundStmt *>(parseCompoundStmt().release()));
        } else if (match("SEMICOLON")) {
            // Прототип функции
        } else {
            throw std::runtime_error("Expected function body or ';'");
        }

        return func;
    }

    std::unique_ptr<CppEnumDecl> parseEnum() {
        auto enumDecl = std::make_unique<CppEnumDecl>();

        if (match("CLASS") || match("STRUCT")) {
            enumDecl->isScoped = true;
        }

        if (check("IDENTIFIER")) {
            enumDecl->name = advance().getValue();
        }

        // Базовый тип
        if (match("COLON")) {
            // Парсим тип, а не просто токен
            std::string baseType;
            while (isTypeToken(peekType()) || check("IDENTIFIER")) {
                if (!baseType.empty()) baseType += " ";
                baseType += advance().getValue();
            }
            enumDecl->underlyingType = baseType;
        }

        if (!match("OPENCURLY")) {
            std::cout << "DEBUG: Expected '{', got: " << peek().getValue() << std::endl;
            throw std::runtime_error("Expected '{' after enum");
        }

        // Перечислители
        while (!isAtEnd() && !check("CLOSECURLY")) {
            if (check("IDENTIFIER")) {
                std::string enumerator = advance().getValue();
                std::unique_ptr<CppExpr> value;

                if (match("ASSIGN")) {
                    value = parseExpression();
                }

                enumDecl->enumerators.emplace_back(enumerator, std::move(value));
            }

            if (!match("COMMA")) {
                break;
            }
        }

        consume("CLOSECURLY", "Expected '}' after enum");
        consume("SEMICOLON", "Expected ';' after enum");
        return enumDecl;
    }

    // ============ ВЫРАЖЕНИЯ ============

    std::unique_ptr<CppExpr> parseExpression() {
        return parseAssignment();
    }

    std::unique_ptr<CppExpr> parseAssignment() {
        auto expr = parseTernary();

        if (match("ASSIGN") || match("PLUSASSIGN") || match("MINUSASSIGN") ||
            match("MULTIASSIGN") || match("DIVASSIGN") || match("MODASSIGN") ||
            match("BITSHIFTLEFTASSIGN") || match("BITSHIFTRIGHTASSIGN")) {

            auto assign = std::make_unique<CppBinaryOp>();
            assign->left = std::move(expr);

            std::string op = previous().getValue();
            if (op == "=") assign->op = CppBinOpKind::Assign;
            else if (op == "+=") assign->op = CppBinOpKind::AddAssign;
            else if (op == "-=") assign->op = CppBinOpKind::SubAssign;
            else if (op == "*=") assign->op = CppBinOpKind::MulAssign;
            else if (op == "/=") assign->op = CppBinOpKind::DivAssign;
            else if (op == "%=") assign->op = CppBinOpKind::ModAssign;
            else if (op == "<<=") assign->op = CppBinOpKind::ShlAssign;
            else if (op == ">>=") assign->op = CppBinOpKind::ShrAssign;

            assign->right = parseAssignment();
            return assign;
        }

        return expr;
    }

    std::unique_ptr<CppExpr> parseTernary() {
        auto expr = parseLogicalOr();

        if (match("QUESTION")) {
            auto ternary = std::make_unique<CppTernaryOp>();
            ternary->condition = std::move(expr);
            ternary->thenExpr = parseExpression();
            consume("COLON", "Expected ':' in ternary operator");
            ternary->elseExpr = parseExpression();
            return ternary;
        }

        return expr;
    }

    std::unique_ptr<CppExpr> parseLogicalOr() {
        auto expr = parseLogicalAnd();
        while (match("OR")) {
            auto binOp = std::make_unique<CppBinaryOp>();
            binOp->op = CppBinOpKind::Or;
            binOp->left = std::move(expr);
            binOp->right = parseLogicalAnd();
            expr = std::move(binOp);
        }
        return expr;
    }

    std::unique_ptr<CppExpr> parseLogicalAnd() {
        auto expr = parseBitwiseOr();
        while (match("AND")) {
            auto binOp = std::make_unique<CppBinaryOp>();
            binOp->op = CppBinOpKind::And;
            binOp->left = std::move(expr);
            binOp->right = parseBitwiseOr();
            expr = std::move(binOp);
        }
        return expr;
    }

    std::unique_ptr<CppExpr> parseBitwiseOr() {
        auto expr = parseBitwiseXor();
        while (match("BITOR")) {
            auto right = parseBitwiseXor();
            expr = makeBinary(std::move(expr), CppBinOpKind::BitOr, std::move(right));
        }
        return expr;
    }

    std::unique_ptr<CppExpr> parseBitwiseXor() {
        auto expr = parseBitwiseAnd();
        while (match("BITXOR")) {
            auto right = parseBitwiseAnd();
            expr = makeBinary(std::move(expr), CppBinOpKind::BitXor, std::move(right));
        }
        return expr;
    }

    std::unique_ptr<CppExpr> parseBitwiseAnd() {
        auto expr = parseEquality();
        while (match("BITAND")) {
            auto right = parseEquality();
            expr = makeBinary(std::move(expr), CppBinOpKind::BitAnd, std::move(right));
        }
        return expr;
    }

    std::unique_ptr<CppExpr> parseEquality() {
        auto expr = parseRelational();
        while (true) {
            if (match("JE")) {
                auto right = parseRelational();
                expr = makeBinary(std::move(expr), CppBinOpKind::Eq, std::move(right));
            } else if (match("JNE")) {
                auto right = parseRelational();
                expr = makeBinary(std::move(expr), CppBinOpKind::Ne, std::move(right));
            } else {
                break;
            }
        }
        return expr;
    }

    std::unique_ptr<CppExpr> parseRelational() {
        auto expr = parseShift();
        while (true) {
            if (match("JGE")) {
                auto right = parseShift();
                expr = makeBinary(std::move(expr), CppBinOpKind::Ge, std::move(right));
            } else if (match("JLE")) {
                auto right = parseShift();
                expr = makeBinary(std::move(expr), CppBinOpKind::Le, std::move(right));
            } else if (match("JG")) {
                auto right = parseShift();
                expr = makeBinary(std::move(expr), CppBinOpKind::Gt, std::move(right));
            } else if (match("JL")) {
                auto right = parseShift();
                expr = makeBinary(std::move(expr), CppBinOpKind::Lt, std::move(right));
            } else {
                break;
            }
        }
        return expr;
    }

    std::unique_ptr<CppExpr> parseShift() {
        auto expr = parseAdditive();
        while (true) {
            if (match("BITSHIFTLEFT")) {
                // Проверяем, это вывод в поток или битовый сдвиг
                // Упрощенно - всегда считаем битовым сдвигом
                auto right = parseAdditive();
                expr = makeBinary(std::move(expr), CppBinOpKind::Shl, std::move(right));
            } else if (match("BITSHIFTRIGHT")) {
                auto right = parseAdditive();
                expr = makeBinary(std::move(expr), CppBinOpKind::Shr, std::move(right));
            } else {
                break;
            }
        }
        return expr;
    }

    std::unique_ptr<CppExpr> parseAdditive() {
        auto expr = parseMultiplicative();
        while (true) {
            if (match("PLUS")) {
                auto right = parseMultiplicative();
                expr = makeBinary(std::move(expr), CppBinOpKind::Add, std::move(right));
            } else if (match("MINUS")) {
                auto right = parseMultiplicative();
                expr = makeBinary(std::move(expr), CppBinOpKind::Sub, std::move(right));
            } else {
                break;
            }
        }
        return expr;
    }

    std::unique_ptr<CppExpr> parseMultiplicative() {
        auto expr = parseUnary();
        while (true) {
            if (match("MULTI")) {
                auto right = parseUnary();
                expr = makeBinary(std::move(expr), CppBinOpKind::Mul, std::move(right));
            } else if (match("DIV")) {
                auto right = parseUnary();
                expr = makeBinary(std::move(expr), CppBinOpKind::Div, std::move(right));
            } else if (match("MOD")) {
                auto right = parseUnary();
                expr = makeBinary(std::move(expr), CppBinOpKind::Mod, std::move(right));
            } else {
                break;
            }
        }
        return expr;
    }

    std::unique_ptr<CppExpr> parseSizeofExpr() {
        auto sizeofExpr = std::make_unique<CppSizeofExpr>();

        if (match("OPENPARENTHESES")) {
            if (isTypeToken(peekType())) {
                sizeofExpr->expression = parseType();
                sizeofExpr->isType = true;
            } else {
                sizeofExpr->expression = parseExpression();
                sizeofExpr->isType = false;
            }
            consume("CLOSEPARENTHESES", "Expected ')' after sizeof");
        } else {
            sizeofExpr->expression = parseUnary();
            sizeofExpr->isType = false;
        }

        return sizeofExpr;
    }

    std::unique_ptr<CppExpr> parseLiteral() {
        Token token = previous();

        if (token.getType() == "VALUESTRING" || token.getType() == "VALUECHAR") {
            auto lit = std::make_unique<CppStringLiteral>();
            lit->value = token.getValue();
            return lit;
        } else if (token.getType() == "VALUEBOOL") {
            auto lit = std::make_unique<CppBoolLiteral>();
            lit->value = (token.getValue() == "true");
            return lit;
        } else if (token.getType() == "VALUEINTEGER") {
            auto lit = std::make_unique<CppIntLiteral>();
            try {
                lit->value = std::stoll(token.getValue());
            } catch (...) {
                lit->value = 0;
            }
            return lit;
        } else if (token.getType() == "VALUEFLOAT") {
            auto lit = std::make_unique<CppRealLiteral>();
            try {
                lit->value = std::stod(token.getValue());
            } catch (...) {
                lit->value = 0.0;
            }
            return lit;
        }

        return nullptr;
    }

    CppStmtPtr parseTryStmt() {
        auto tryStmt = std::make_unique<CppTryStmt>();

        tryStmt->tryBlock = std::unique_ptr<CppCompoundStmt>(
                static_cast<CppCompoundStmt *>(parseCompoundStmt().release())
        );

        while (match("CATCH")) {
            auto catchStmt = std::make_unique<CppCatchStmt>();
            consume("OPENPARENTHESES", "Expected '(' after catch");

            if (isTypeToken(peekType())) {
                catchStmt->exceptionType = parseTypeName();
                if (check("IDENTIFIER")) {
                    catchStmt->exceptionName = advance().getValue();
                }
            }

            consume("CLOSEPARENTHESES", "Expected ')' after catch parameters");
            catchStmt->body = std::unique_ptr<CppCompoundStmt>(
                    static_cast<CppCompoundStmt *>(parseCompoundStmt().release())
            );

            tryStmt->handlers.push_back(std::move(catchStmt));
        }

        return tryStmt;
    }

    CppExprPtr parseType() {
        // Упрощенная версия - возвращаем идентификатор типа
        auto typeExpr = std::make_unique<CppIdentifier>();
        typeExpr->name = parseTypeName();
        return typeExpr;
    }

    std::unique_ptr<CppExpr> makeUnary(const std::string &op, std::unique_ptr<CppExpr> operand, bool postfix) {
        if (!operand) {
            throw std::runtime_error("Операнд унарной операции не может быть nullptr");
        }
        auto node = std::make_unique<CppUnaryOp>();
        node->op = op;
        node->postfix = postfix;
        node->operand = std::move(operand);
        return node;
    }

    std::unique_ptr<CppExpr>
    makeBinary(std::unique_ptr<CppExpr> left, CppBinOpKind kind, std::unique_ptr<CppExpr> right) {
        if (!left || !right) {
            throw std::runtime_error("Операнды бинарной операции не могут быть nullptr");
        }
        auto node = std::make_unique<CppBinaryOp>();
        node->op = kind;
        node->left = std::move(left);
        node->right = std::move(right);
        return node;
    }

    CppStmtPtr parseGlobalVar() {
        size_t save = current;
        return parseVarDecl(true);
    }

    std::unique_ptr<CppExpr> parseUnary() {
//        if (match("SIZEOF")) {
//            auto sizeofExpr = std::make_unique<SizeofExpr>();
//
//            if (match("OPENPARENTHESES")) {
//                // sizeof(type)
//                if (isTypeToken(peekType())) {
//                    sizeofExpr->expression = parseType();
//                    sizeofExpr->isType = true;
//                } else {
//                    //sizeof(expression)
//                    sizeofExpr->expression = parseExpression();
//                    sizeofExpr->isType = false;
//                }
//                consume("CLOSEPARENTHESES", "Ожидалась ')' после sizeof");
//            } else {
//                // sizeof expression (без скобок)
//                sizeofExpr->expression = parseUnary();
//                sizeofExpr->isType = false;
//            }
//
//            return sizeofExpr;
//        }
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

    std::unique_ptr<CppDecl> parseUnion() {
        auto unionDecl = std::make_unique<CppClassDecl>(); // или специальный UnionDecl
        unionDecl->isStruct = true; // обрабатываем union аналогично struct
        unionDecl->isUnion = true;

        if (check("IDENTIFIER")) {
            unionDecl->name = advance().getValue();
            knownClasses.insert(unionDecl->name);
        }

        consume("OPENCURLY", "Ожидалась '{' после union");

        // Парсим поля
        while (!isAtEnd() && !check("CLOSECURLY")) {
            if (isTypeToken(peekType())) {
                auto field = parseFieldDecl();
                if (field) {
                    if (auto fieldDecl = dynamic_cast<CppFieldDecl *>(field.get())) {
                        fieldDecl->access = CppAccessSpecifier::Public; // в union все public
                        unionDecl->members.push_back(std::move(field));
                    }
                }
            } else {
                advance();
            }
        }

        consume("CLOSECURLY", "Ожидалась '}' после union");
        consume("SEMICOLON", "Ожидался ';' после union");

        return unionDecl;
    }

    std::unique_ptr<CppExpr> parsePostfix() {
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

    std::unique_ptr<CppExpr> parsePrimary() {
        if (match("THIS")) {
            return std::make_unique<CppThisExpr>();
        }

        if (match("NULLPTR")) {
            return std::make_unique<CppNullptrLiteral>();
        }

        if (match("TRUE")) {
            auto lit = std::make_unique<CppBoolLiteral>();
            lit->value = true;
            return lit;
        }

        if (match("FALSE")) {
            auto lit = std::make_unique<CppBoolLiteral>();
            lit->value = false;
            return lit;
        }

        if (match("NEW")) {
            return parseNewExpr();
        }

        if (match("DELETE")) {
            return parseDeleteExpr();
        }

        if (match("SIZEOF")) {
            return parseSizeofExpr();
        }

        // Лямбда-выражения
        if (match("LAMBDA")) {
            return parseLambdaExpr();
        }

        // Базовые случаи (как в C парсере)
        if (match("OPENPARENTHESES")) {
            auto expr = parseExpression();
            consume("CLOSEPARENTHESES", "Expected ')'");
            return expr;
        }

        if (match("IDENTIFIER")) {
            return parseIdentifierExpr();
        }

        // Литералы
        if (match("VALUESTRING") || match("VALUECHAR") ||
            match("VALUEINTEGER") || match("VALUEFLOAT") || match("VALUEBOOL")) {
            return parseLiteral();
        }

        throw std::runtime_error("Unexpected token in expression");
    }

    CppExprPtr parseIdentifierExpr() {
        auto ident = std::make_unique<CppIdentifier>();
        ident->name = previous().getValue();

        CppExprPtr expr = std::move(ident);


        // Обработка квалифицированных имен (std::cout)
        while (match("SCOPE")) {
            auto qualified = std::make_unique<CppMemberAccessExpr>();
            qualified->object = std::move(expr);
            if (check("IDENTIFIER")) {
                qualified->member = advance().getValue();
                qualified->isPointerAccess = false;
            }
            expr = std::move(qualified);
        }

        // Обработка вызовов функций, доступ к массивам и полям
        while (true) {
            // Доступ к массиву
            if (match("OPENBRACKET")) {
                auto arrayAccess = std::make_unique<CppArrayAccessExpr>();
                arrayAccess->array = std::move(expr);
                arrayAccess->index = parseExpression();
                consume("CLOSEBRACKET", "Expected ']'");
                expr = std::move(arrayAccess);
            }
                // Доступ к полю
            else if (match("MEMBERACCESS") || match("PTRACCESS")) {
                auto memberAccess = std::make_unique<CppMemberAccessExpr>();
                memberAccess->object = std::move(expr);
                memberAccess->member = consume("IDENTIFIER", "Expected member name").getValue();
                memberAccess->isPointerAccess = (previous().getType() == "PTRACCESS");
                expr = std::move(memberAccess);
            }
                // Вызов функции
            else if (match("OPENPARENTHESES")) {
                auto call = std::make_unique<CppCallExpr>();
                call->callee = std::move(expr);
                if (!check("CLOSEPARENTHESES")) {
                    do {
                        call->arguments.push_back(parseExpression());
                    } while (match("COMMA"));
                }
                consume("CLOSEPARENTHESES", "Expected ')' after arguments");
                expr = std::move(call);
            } else {
                break;
            }
        }

        return expr;
    }

    CppExprPtr parseNewExpr() {
        auto newExpr = std::make_unique<CppNewExpr>();

        if (match("OPENBRACKET")) {
            newExpr->isArray = true;
            consume("CLOSEBRACKET", "Expected ']'");
        }

        newExpr->typeName = parseTypeName();

        if (match("OPENPARENTHESES")) {
            if (!check("CLOSEPARENTHESES")) {
                do {
                    newExpr->arguments.push_back(parseExpression());
                } while (match("COMMA"));
            }
            consume("CLOSEPARENTHESES", "Expected ')'");
        }

        return newExpr;
    }

    CppExprPtr parseDeleteExpr() {
        auto deleteExpr = std::make_unique<CppDeleteExpr>();

        if (match("OPENBRACKET")) {
            deleteExpr->isArray = true;
            consume("CLOSEBRACKET", "Expected ']'");
        }

        deleteExpr->operand = parseExpression();
        return deleteExpr;
    }


    std::unique_ptr<CppExpr> parseLambdaExpr() {
        auto lambda = std::make_unique<CppLambdaExpr>();

        // Захваты
        if (match("LAMBDA_CAPTURE")) {
            // Упрощенная обработка захватов
            std::string capture = previous().getValue();
            // ... парсинг capture list
        }

        // Упрощенная обработка параметров - конвертируем CppParameter в unique_ptr<CppParameter>
        if (match("OPENPARENTHESES")) {
            auto params = parseParameters();
            for (auto &param: params) {
                auto paramPtr = std::make_unique<CppParameter>();
                *paramPtr = std::move(param);
                lambda->parameters.push_back(std::move(paramPtr));
            }
            consume("CLOSEPARENTHESES", "Expected ')'");
        }

        // Возвращаемый тип - создаем идентификатор
        if (match("ARROW")) {
            auto returnType = std::make_unique<CppIdentifier>();
            returnType->name = parseTypeName();
            lambda->returnType = std::move(returnType);
        }

        // Тело
        if (match("OPENCURLY")) {
            lambda->body = std::unique_ptr<CppCompoundStmt>(
                    static_cast<CppCompoundStmt *>(parseCompoundStmt().release()));
        } else {
            // Expression lambda
            auto body = std::make_unique<CppCompoundStmt>();
            auto returnStmt = std::make_unique<CppReturnStmt>();
            returnStmt->value = parseExpression();
            body->statements.push_back(std::move(returnStmt));
            lambda->body = std::move(body);
        }

        return lambda;
    }

    // ============ ВСПОМОГАТЕЛЬНЫЕ МЕТОДЫ ============

    std::vector<CppParameter> parseParameters() {
        std::vector<CppParameter> params;

        if (check("CLOSEPARENTHESES")) {
            std::cout << "DEBUG: Empty parameter list" << std::endl;
            return params;
        }

        do {
            CppParameter param;
            size_t paramStart = current;

            try {
                // 🔥 УПРОЩЕННЫЙ ПАРСИНГ ТИПА - избегаем рекурсивных вызовов
                param.typeName = parseSimpleTypeName();

                // Имя параметра (может отсутствовать)
                if (check("IDENTIFIER")) {
                    param.name = advance().getValue();
                }

                // Значение по умолчанию - пропускаем для упрощения
                if (match("ASSIGN")) {
                    std::cout << "DEBUG: Skipping default value for parameter" << std::endl;
                    skipUntilCommaOrParen();
                }

                params.push_back(param);
                std::cout << "DEBUG: Added parameter: " << param.typeName << " " << param.name << std::endl;
            } catch (const std::exception &e) {
                // Если не удалось распарсить параметр, пропускаем его
                std::cout << "DEBUG: Parameter parsing failed: " << e.what() << std::endl;
                current = paramStart;
                skipUntilCommaOrParen();
            }
        } while (match("COMMA"));
        std::cout << "DEBUG: Finished parsing " << params.size() << " parameters" << std::endl;

        return params;
    }

    void skipUntilCommaOrSemicolon() {
        int parenLevel = 0;
        while (!isAtEnd()) {
            if (check("OPENPARENTHESES")) {
                parenLevel++;
            } else if (check("CLOSEPARENTHESES")) {
                if (parenLevel == 0) break;
                parenLevel--;
            } else if ((check("COMMA") || check("SEMICOLON")) && parenLevel == 0) {
                break;
            }
            advance();
        }
    }

    std::string parseSimpleTypeName() {
        std::string typeName;
        size_t start = current;
        try{
        // Только базовые квалификаторы
        while (match("CONST") || match("VOLATILE")) {
            if (!typeName.empty()) typeName += " ";
            typeName += previous().getValue();
        }

        // Базовый тип
        if (isTypeToken(peekType()) || check("IDENTIFIER")) {
            if (!typeName.empty()) typeName += " ";
            typeName += advance().getValue();
        } else {
            throw std::runtime_error("Expected type in parameter");
        }

        // Простые указатели/ссылки
        while (match("MULTI") || match("BITAND") || match("BITANDAND")) {
            if (previous().getType() == "MULTI") typeName += "*";
            else if (previous().getType() == "BITAND") typeName += "&";
            else if (previous().getType() == "BITANDAND") typeName += "&&";
        }
            if (typeName.empty()) {
                throw std::runtime_error("Failed to parse type");
            }

            std::cout << "DEBUG: Successfully parsed simple type: " << typeName << std::endl;

            return typeName;
        }catch (const std::exception& e) {
            current = start;
            std::cout << "DEBUG: Failed to parse simple type: " << e.what() << std::endl;
            throw;
        }
    }

    void skipUntilCommaOrParen() {
        while (!isAtEnd() && !check("COMMA") && !check("CLOSEPARENTHESES")) {
            advance();
        }
    }

    std::string parseTypeName() {
        std::string typeName;
        size_t start = current;

        try {
            // Собираем квалификаторы
            while (match("CONST") || match("VOLATILE") || match("STATIC") ||
                   match("EXTERN") || match("MUTABLE")) {
                if (!typeName.empty()) typeName += " ";
                typeName += previous().getValue();
            }

            // 🔥 КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ: идентификатор МОЖЕТ быть типом
            if (isTypeToken(peekType()) || check("IDENTIFIER")) {
                if (!typeName.empty()) typeName += " ";
                typeName += advance().getValue();

                // Квалифицированные имена
                while (match("SCOPE")) {
                    typeName += "::";
                    if (check("IDENTIFIER") || isTypeToken(peekType())) {
                        typeName += advance().getValue();
                    } else {
                        throw std::runtime_error("Ожидался идентификатор после ::");
                    }
                }
            } else {
                throw std::runtime_error("Ожидался тип");
            }

            // Указатели и ссылки
            while (match("MULTI") || match("BITAND") || match("BITANDAND")) {
                if (previous().getType() == "MULTI") typeName += "*";
                else if (previous().getType() == "BITAND") typeName += "&";
                else if (previous().getType() == "BITANDAND") typeName += "&&";
            }

            if (typeName.empty()) {
                throw std::runtime_error("Не удалось распарсить тип");
            }

            std::cout << "DEBUG: Successfully parsed type: " << typeName << std::endl;
            return typeName;

        } catch (const std::exception &e) {
            current = start;
            std::cout << "DEBUG: Failed to parse type: " << e.what()
                      << " at token: " << (isAtEnd() ? "END" : peek().getValue()) << std::endl;
            throw;
        }
    }

    CppStmtPtr parseThrowStmt() {
        auto throwExpr = parseExpression();
        consume("SEMICOLON", "Expected ';' after throw");

        auto stmt = std::make_unique<CppExprStmt>();
        stmt->expression = std::move(throwExpr);
        return stmt;
    }

    std::unique_ptr<CppStmt> parseStatement() {
        if (isAtEnd()) {
            std::cout << "DEBUG parseStatement: at end" << std::endl;
            return nullptr;
        }
        // Отладочный вывод
        std::cout << "DEBUG parseStatement: " << peek().getValue() << " (" << peek().getType() << ")" << std::endl;


        if (match("IF")) return parseIfStmt();
        if (match("WHILE")) return parseWhileStmt();
        if (match("FOR")) return parseForStmt();
        if (match("DO")) return parseDoWhileStmt();
        if (match("SWITCH")) return parseSwitchStmt();
        if (match("RETURN")) return parseReturnStmt();
        if (match("BREAK")) return parseBreakStmt();
        if (match("CONTINUE")) return parseContinueStmt();
        if (match("TRY")) return parseTryStmt();
        if (match("THROW")) return parseThrowStmt();
        if (match("OPENCURLY")) return parseCompoundStmt();
        if (match("SEMICOLON")) return nullptr; // empty statement

        // Объявления переменных
        if (isTypeToken(peekType()) || check("AUTO")) {
            return parseVarDecl();
        }

        // Выражения
        try {
            auto expr = parseExpression();
            if (expr) {
                auto stmt = std::make_unique<CppExprStmt>();
                stmt->expression = std::move(expr);
                if (match("SEMICOLON")) {
                    return stmt;
                } else {
                    throw std::runtime_error("Expected ';' after expression");
                }
            }
        } catch (const std::exception &e) {
            std::cout << "Error parsing expression: " << e.what() << std::endl;
        }

        // Если ничего не распарсилось, пропускаем токен
        std::cout << "Skipping token in statement: " << peek().getValue() << std::endl;
        advance();
        return nullptr;
    }

    CppStmtPtr parseIfStmt() {
        consume("OPENPARENTHESES", "Expected '(' after if");
        auto condition = parseExpression();
        consume("CLOSEPARENTHESES", "Expected ')' after condition");

        auto thenBranch = parseStatement();
        CppStmtPtr elseBranch;
        if (match("ELSE")) {
            elseBranch = parseStatement();
        }

        auto stmt = std::make_unique<CppIfStmt>();
        stmt->condition = std::move(condition);
        stmt->thenBranch = std::move(thenBranch);
        stmt->elseBranch = std::move(elseBranch);
        return stmt;
    }

    CppStmtPtr parseWhileStmt() {
        consume("OPENPARENTHESES", "Expected '(' after while");
        auto condition = parseExpression();
        if (!condition) {
            throw std::runtime_error("Условие while не может быть nullptr");
        }
        consume("CLOSEPARENTHESES", "Expected ')' after condition");
        auto body = parseStatement();

        auto stmt = std::make_unique<CppWhileStmt>();
        stmt->condition = std::move(condition);
        stmt->body = std::move(body);
        return stmt;
    }

    CppStmtPtr parseForStmt() {
        consume("OPENPARENTHESES", "Expected '(' after for");

        // Инициализация
        CppStmtPtr init;
        if (!check("SEMICOLON")) {
            if (isTypeToken(peekType()) || check("AUTO")) {
                init = parseVarDecl(false); // без точки с запятой
            } else {
                init = parseExprStmt(false); // без точки с запятой
            }
        }

        if (!match("SEMICOLON")) {
            throw std::runtime_error("Expected ';' after initialization");
        }

        // Условие
        CppExprPtr condition;
        if (!check("SEMICOLON")) {
            condition = parseExpression();
        }

        if (!match("SEMICOLON")) {
            throw std::runtime_error("Expected ';' after condition");
        }
        // Инкремент
        CppExprPtr increment;
        if (!check("CLOSEPARENTHESES")) {
            increment = parseExpression();
        }
        consume("CLOSEPARENTHESES", "Expected ')' after for");

        auto body = parseStatement();

        auto stmt = std::make_unique<CppForStmt>();
        stmt->init = std::move(init);
        stmt->condition = std::move(condition);
        stmt->increment = std::move(increment);
        stmt->body = std::move(body);
        return stmt;
    }

    CppStmtPtr parseDoWhileStmt() {
        auto stmt = std::make_unique<CppDoWhileStmt>();

        // Парсим тело do
        stmt->body = parseStatement();
        if (!stmt->body) {
            throw std::runtime_error("Expected statement after 'do'");
        }

        // Ожидаем 'while'
        if (!match("WHILE")) {
            throw std::runtime_error("Expected 'while' after do body");
        }

        // Условие в скобках
        consume("OPENPARENTHESES", "Expected '(' after while");
        stmt->condition = parseExpression();
        consume("CLOSEPARENTHESES", "Expected ')' after condition");
        consume("SEMICOLON", "Expected ';' after do-while");

        return stmt;
    }

    CppStmtPtr parseCase() {
        auto caseStmt = std::make_unique<CppCaseStmt>();

        // Парсим значение case (может быть выражением)
        if (!check("COLON")) {
            caseStmt->value = parseExpression();
        }

        consume("COLON", "Ожидалось ':' после case");

        // Парсим тело case до следующего case/default/}
        auto bodyBlock = std::make_unique<CppCompoundStmt>();
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

    CppStmtPtr parseDefault() {
        auto defaultStmt = std::make_unique<CppDefaultStmt>();

        consume("COLON", "Ожидалось ':' после default");

        // Парсим тело default до следующего case/default/}
        auto bodyBlock = std::make_unique<CppCompoundStmt>();
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

    CppStmtPtr parseSwitchStmt() {
        consume("OPENPARENTHESES", "Ожидалась '(' после switch");
        auto condition = parseExpression();
        consume("CLOSEPARENTHESES", "Ожидалась ')' после условия switch");
        consume("OPENCURLY", "Ожидался '{' после switch");

        auto switchStmt = std::make_unique<CppSwitchStmt>();
        switchStmt->condition = std::move(condition);

        // Упрощенно: пропускаем содержимое switch
        while (!isAtEnd() && !check("CLOSECURLY")) {
            if (match("CASE")) {
                auto caseStmt = parseCase();
                switchStmt->cases.push_back(
                        std::unique_ptr<CppCaseStmt>(static_cast<CppCaseStmt *>(caseStmt.release())));
            } else if (match("DEFAULT")) {
                auto defaultStmt = parseDefault();
                switchStmt->cases.push_back(
                        std::unique_ptr<CppCaseStmt>(static_cast<CppCaseStmt *>(defaultStmt.release())));
            } else {
                advance();
            }
        }

        consume("CLOSECURLY", "Ожидался '}' после switch");
        return switchStmt;
    }

    CppStmtPtr parseReturnStmt() {
        auto stmt = std::make_unique<CppReturnStmt>();
        if (!check("SEMICOLON")) {
            stmt->value = parseExpression();
        }
        consume("SEMICOLON", "Expected ';' after return");
        return stmt;
    }

    CppStmtPtr parseBreakStmt() {
        auto stmt = std::make_unique<CppBreakStmt>();
        consume("SEMICOLON", "Expected ';' after break");
        return stmt;
    }

    CppStmtPtr parseContinueStmt() {
        auto stmt = std::make_unique<CppContinueStmt>();
        consume("SEMICOLON", "Expected ';' after continue");
        return stmt;
    }

    CppStmtPtr parseExprStmt(bool requireSemicolon = true) {
        auto expr = parseExpression();
        if (requireSemicolon) {
            consume("SEMICOLON", "Expected ';' after expression");
        }
        auto stmt = std::make_unique<CppExprStmt>();
        stmt->expression = std::move(expr);
        return stmt;
    }

    CppStmtPtr parseVarDecl(bool requireSemicolon = true) {
        size_t save = current;

        try {
            std::string typeName = parseTypeName();

            if (!check("IDENTIFIER")) {
                current = save;
                return nullptr;
            }

            // Если это множественное объявление, создаем блок
            auto firstDecl = std::make_unique<CppVarDecl>();
            firstDecl->typeName = typeName;
            firstDecl->name = advance().getValue();

            // Инициализатор для первой переменной
            if (match("ASSIGN")) {
                firstDecl->initializer = parseExpression();
            }

            std::vector<std::unique_ptr<CppStmt>> declarations;
            declarations.push_back(std::move(firstDecl));

            // Обрабатываем дополнительные объявления через запятую
            while (match("COMMA")) {
                if (!check("IDENTIFIER")) break;

                auto additionalDecl = std::make_unique<CppVarDecl>();
                additionalDecl->typeName = typeName; // тот же тип
                additionalDecl->name = advance().getValue();

                if (match("ASSIGN")) {
                    additionalDecl->initializer = parseExpression();
                }

                declarations.push_back(std::move(additionalDecl));
            }

            if (requireSemicolon) {
                consume("SEMICOLON", "Expected ';' after declaration");
            }

            // Возвращаем одиночное объявление или блок
            if (declarations.size() == 1) {
                return std::move(declarations[0]);
            } else {
                auto block = std::make_unique<CppCompoundStmt>();
                for (auto &decl: declarations) {
                    block->statements.push_back(std::move(decl));
                }
                return block;
            }

        } catch (const std::exception &e) {
            current = save;
            return nullptr;
        }
    }

    CppStmtPtr parseCompoundStmt() {
        std::cout << "DEBUG: Starting parseCompoundStmt" << std::endl;
        auto block = std::make_unique<CppCompoundStmt>();

        while (!isAtEnd() && !check("CLOSECURLY")) {
            if (auto stmt = parseStatement()) {
                block->statements.push_back(std::move(stmt));
            } else {
                std::cout << "DEBUG: Skipping token in compound stmt: " << peek().getValue() << std::endl;
                advance();
            }
        }
        if (isAtEnd()) {
            throw std::runtime_error("Unexpected end of file while parsing compound statement");
        }

        consume("CLOSECURLY", "Expected '}'");
        std::cout << "DEBUG: Finished parseCompoundStmt with " << block->statements.size() << " statements" << std::endl;

        return block;
    }

    // ============ УТИЛИТЫ ============
    bool isPreprocessor(const std::string &type) {
        return type == "INCLUDE" || type == "DEFINE" ||
               type == "IFDEF" || type == "IFNDEF" || type == "ENDIF";
    }

    bool isTypeToken(const std::string &type) {
        static const std::vector<std::string> typeTokens = {
                "VOID", "INT", "FLOAT", "DOUBLE", "CHAR", "BOOL", "SHORT", "LONG",
                "SIGNED", "UNSIGNED", "AUTO", "CONST", "VOLATILE", "STATIC",
                "CLASS", "STRUCT", "ENUM", "TYPENAME"
        };
        return std::find(typeTokens.begin(), typeTokens.end(), type) != typeTokens.end();
    }

    bool isFunctionStart() {
        size_t save = current;

        try {
            // Для методов может не быть возвращаемого типа (конструкторы/деструкторы)
            // или он может быть сложным
            std::string typeName = parseTypeName();

            if (!check("IDENTIFIER") && !check("OPERATOR")) {
                current = save;
                return false;
            }

            std::string name = peek().getValue();
            advance(); // имя

            bool isFunc = check("OPENPARENTHESES");
            current = save;
            return isFunc;

        } catch (...) {
            current = save;
            // Проверяем случай оператора
            if (check("OPERATOR")) {
                size_t opSave = current;
                advance(); // operator

                // Проверяем различные типы операторов
                if (check("ASSIGN") || check("PLUS") || check("MINUS") || check("MULTI") ||
                    check("DIV") || check("MOD") || check("JE") || check("JNE") ||
                    check("JL") || check("JG") || check("JLE") || check("JGE") ||
                    check("INCREMENT") || check("DECREMENT") || check("BITSHIFTLEFT") ||
                    check("BITSHIFTRIGHT") || check("BITAND") || check("BITOR") ||
                    check("BITXOR") || check("BITNOT") || check("AND") || check("OR") ||
                    check("OPENBRACKET") || check("CLOSEBRACKET") || check("OPENPARENTHESES") ||
                    check("CLOSEPARENTHESES") || check("MEMBERACCESS") || check("PTRACCESS") ||
                    check("NEW") || check("DELETE")) {

                    advance(); // оператор
                    bool isOpFunc = check("OPENPARENTHESES");
                    current = opSave;
                    return isOpFunc;
                }
                current = opSave;
            }

            return false;
        }
    }

    bool isGlobalVarStart() {
        size_t save = current;

        try {
            // Пытаемся распарсить тип
            std::string typeName = parseTypeName();

            if (!check("IDENTIFIER")) {
                current = save;
                return false;
            }

            // Продвигаемся для анализа контекста
            advance(); // идентификатор переменной

            // Это объявление переменной если не функция
            bool isVar = !check("OPENPARENTHESES");

            current = save;
            return isVar;

        } catch (const std::exception &e) {
            // Если не распарсился как известный тип, проверяем пользовательский тип
            current = save;

            if (check("IDENTIFIER")) {
                std::string potentialType = peek().getValue();

                // Проверяем, известен ли этот идентификатор как тип
                if (knownClasses.count(potentialType) > 0) {
                    advance(); // потребляем идентификатор типа

                    // Проверяем, что следующий токен - идентификатор переменной
                    if (check("IDENTIFIER")) {
                        current = save; // восстанавливаем позицию
                        return true;
                    }
                }
            }
            current = save;
            return false;
        }
    }

    bool isDeclarationStart(const std::string &type) {

        return isTypeToken(type) || type == "CLASS" || type == "STRUCT" ||
               type == "ENUM" || type == "TYPEDEF" || type == "USING" ||
               type == "TEMPLATE" || type == "NAMESPACE" || type == "UNION";
    }

    // Token matching utilities (аналогично C парсеру)
    bool match(const std::string &type) {
        if (check(type)) {
            advance();
            return true;
        }
        return false;
    }

    bool check(const std::string &type) {
        return !isAtEnd() && tokens[current].getType() == type;
    }

    Token advance() {
        return isAtEnd() ? Token() : tokens[current++];
    }

    Token &peek() {
        static Token dummy;
        return isAtEnd() ? dummy : tokens[current];
    }

    std::string peekType() {
        return isAtEnd() ? "" : tokens[current].getType();
    }

    bool isAtEnd() {
        return current >= tokens.size();
    }

    Token consume(const std::string &type, const std::string &message) {
        if (check(type)) return advance();
        throw std::runtime_error(message);
    }

    std::unique_ptr<CppDecl> parseClassMember(CppAccessSpecifier access) {
        enterRecursion();

        try {
            // 🔥 САМОЕ ПРОСТОЕ РЕШЕНИЕ: пропускаем сложные члены классов
            if (check("IDENTIFIER") && currentClass && peek().getValue() == currentClass->name) {
                // Это конструктор - пропускаем для упрощения
                std::cout << "DEBUG: Skipping constructor: " << peek().getValue() << std::endl;
                skipUntilSemicolonOrBrace();
                exitRecursion();
                return nullptr;
            }

            // Только простые поля
            if (isTypeToken(peekType())) {
                size_t save = current;
                try {
                    auto field = parseFieldDecl();
                    if (field) {
                        if (auto fieldDecl = dynamic_cast<CppFieldDecl *>(field.get())) {
                            fieldDecl->access = access;
                        }
                    }
                    exitRecursion();
                    return field;
                } catch (...) {
                    current = save;
                }
            }

            // Пропускаем все остальное (функции, конструкторы и т.д.)
            std::cout << "DEBUG: Skipping complex class member: " << peek().getValue() << std::endl;
            skipUntilSemicolonOrBrace();

            exitRecursion();
            return nullptr;

        } catch (...) {
            exitRecursion();
            throw;
        }
    }

    template<typename To, typename From>
    std::unique_ptr<To> safe_cast(std::unique_ptr<From> from) {
        if (!from) return nullptr;

        // Проверяем, что To является наследником From
        static_assert(std::is_base_of_v<From, To>, "To must be derived from From");

        To *ptr = dynamic_cast<To *>(from.get());
        if (!ptr) {
            return nullptr;
        }
        from.release(); // освобождаем владение из исходного unique_ptr
        return std::unique_ptr<To>(ptr);
    }

    std::unique_ptr<CppDecl> stmtToDecl(std::unique_ptr<CppStmt> stmt) {
        if (!stmt) return nullptr;

        // Используем dynamic_cast для безопасного преобразования
        if (auto varDecl = dynamic_cast<CppVarDecl *>(stmt.get())) {
            stmt.release();
            return std::unique_ptr<CppDecl>(dynamic_cast<CppDecl *>(varDecl));
        }
        if (auto funcDecl = dynamic_cast<CppFunctionDecl *>(stmt.get())) {
            stmt.release();
            return std::unique_ptr<CppDecl>(dynamic_cast<CppDecl *>(funcDecl));
        }
        if (auto classDecl = dynamic_cast<CppClassDecl *>(stmt.get())) {
            stmt.release();
            return std::unique_ptr<CppDecl>(dynamic_cast<CppDecl *>(classDecl));
        }
        if (auto namespaceDecl = dynamic_cast<CppNamespaceDecl *>(stmt.get())) {
            stmt.release();
            return std::unique_ptr<CppDecl>(dynamic_cast<CppDecl *>(namespaceDecl));
        }
        if (auto enumDecl = dynamic_cast<CppEnumDecl *>(stmt.get())) {
            stmt.release();
            return std::unique_ptr<CppDecl>(dynamic_cast<CppDecl *>(enumDecl));
        }

        return nullptr;
    }

};


#endif //AISDLAB_CPPPARSERTOAST_H
