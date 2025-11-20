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

    // Контекст парсера
    std::vector<std::string> currentNamespace;
    std::vector<CppTemplateParameter> currentTemplateParams;
    CppClassDecl *currentClass{nullptr};
    bool inTemplate{false};
    std::unordered_set<std::string> knownClasses;
    std::unordered_set<std::string> knownTypedefs;
    std::unordered_set<std::string> currentAccessScope;

    // ============ ОСНОВНОЙ ПАРСИНГ ============

    std::unique_ptr<CppCompoundStmt> parseTranslationUnit() {
        auto block = std::make_unique<CppCompoundStmt>();

        while (!isAtEnd()) {
            std::string tokenType = peekType();

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

            // Шаблоны
            if (match("TEMPLATE")) {
                std::cout << "Skipping template for now" << std::endl;
                while (!isAtEnd() && !check("SEMICOLON") && !check("OPENCURLY")) {
                    advance();
                }
//                auto templ = parseTemplate();
//                if (templ) block->statements.push_back(std::move(templ));
                continue;
            }

            if (check("IDENTIFIER") && peek().getValue() == "std") {
                // Пропускаем сложные типы с std::
                std::cout << "Skipping std:: types for now" << std::endl;
                while (!isAtEnd() && !check("SEMICOLON") && !check("OPENCURLY")) {
                    advance();
                }
                continue;
            }

            // Классы и структуры
            if (match("CLASS") || match("STRUCT")) {
                auto classDecl = parseClass();
                if (classDecl) {
                    block->statements.push_back(std::move(classDecl));
                    knownClasses.insert(static_cast<CppClassDecl *>(classDecl.get())->name);
                }
                continue;
            }

            // Перечисления
            if (match("ENUM")) {
                auto enumDecl = parseEnum();
                if (enumDecl) block->statements.push_back(std::move(enumDecl));
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

            // Функции (глобальные)
            if (isFunctionStart()) {
                auto func = parseFunction();
                if (func) block->statements.push_back(std::move(func));
                continue;
            }

            // Глобальные переменные
            if (isGlobalVarStart()) {
                auto var = parseGlobalVar();
                if (var) block->statements.push_back(std::move(var));
                continue;
            }

            // Если ничего не распарсилось, пропускаем токен
            std::cout << "Skipping token in translation unit: " << tokenType
                      << " [" << peek().getValue() << "]" << std::endl;
            advance();
        }

        return block;
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

            // Правильное преобразование через dynamic_cast
            return std::unique_ptr<CppDecl>(dynamic_cast<CppDecl*>(decl.release()));
        } catch (const std::exception &e) {
            std::cerr << "Error in parseGlobalVarAsDecl: " << e.what() << std::endl;
            return nullptr;
        }
    }
    template<typename To, typename From>
    std::unique_ptr<To> safe_cast(std::unique_ptr<From> from) {
        if (!from) return nullptr;

        // Проверяем, что To является наследником From
        static_assert(std::is_base_of_v<From, To>, "To must be derived from From");

        To* ptr = dynamic_cast<To*>(from.get());
        if (!ptr) {
            return nullptr;
        }
        from.release(); // освобождаем владение из исходного unique_ptr
        return std::unique_ptr<To>(ptr);
    }

    // Специализированные методы преобразования
    std::unique_ptr<CppDecl> stmtToDecl(std::unique_ptr<CppStmt> stmt) {
        if (!stmt) return nullptr;

        // Используем dynamic_cast для безопасного преобразования
        if (auto varDecl = dynamic_cast<CppVarDecl*>(stmt.get())) {
            stmt.release();
            return std::unique_ptr<CppDecl>(dynamic_cast<CppDecl*>(varDecl));
        }
        if (auto funcDecl = dynamic_cast<CppFunctionDecl*>(stmt.get())) {
            stmt.release();
            return std::unique_ptr<CppDecl>(dynamic_cast<CppDecl*>(funcDecl));
        }
        if (auto classDecl = dynamic_cast<CppClassDecl*>(stmt.get())) {
            stmt.release();
            return std::unique_ptr<CppDecl>(dynamic_cast<CppDecl*>(classDecl));
        }
        if (auto namespaceDecl = dynamic_cast<CppNamespaceDecl*>(stmt.get())) {
            stmt.release();
            return std::unique_ptr<CppDecl>(dynamic_cast<CppDecl*>(namespaceDecl));
        }
        if (auto enumDecl = dynamic_cast<CppEnumDecl*>(stmt.get())) {
            stmt.release();
            return std::unique_ptr<CppDecl>(dynamic_cast<CppDecl*>(enumDecl));
        }

        return nullptr;
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
        auto classDecl = std::make_unique<CppClassDecl>();
        classDecl->isStruct = previous().getType() == "STRUCT";

        if (check("IDENTIFIER")) {
            classDecl->name = advance().getValue();
            knownClasses.insert(classDecl->name);
        }

        // Наследование
        if (match("COLON")) {
            do {
//                // Спецификатор доступа
//                CppAccessSpecifier access = CppAccessSpecifier::Private;
//                if (match("PUBLIC")) {
//                    access = CppAccessSpecifier::Public;
//                } else if (match("PROTECTED")) {
//                    access = CppAccessSpecifier::Protected;
//                } else if (match("PRIVATE")) {
//                    access = CppAccessSpecifier::Private;
//                }

                // classDecl->baseAccess.push_back(access);
                classDecl->baseClasses.push_back(parseTypeName());
            } while (match("COMMA"));
        }

//        if (match("FINAL")) {
//            classDecl->isFinal = true;
//        }

        if (!match("OPENCURLY")) {
            throw std::runtime_error("Expected '{' after class");
        }

        // Парсим члены класса
        currentClass = classDecl.get();
        CppAccessSpecifier currentAccess = classDecl->isStruct ?
                                           CppAccessSpecifier::Public : CppAccessSpecifier::Private;

        while (!isAtEnd() && !check("CLOSECURLY")) {

//            // Спецификаторы доступа
//            if (match("PUBLIC")) {
//                currentAccess = CppAccessSpecifier::Public;
//                consume("COLON", "Expected ':' after access specifier");
//                continue;
//            } else if (match("PROTECTED")) {
//                currentAccess = CppAccessSpecifier::Protected;
//                consume("COLON", "Expected ':' after access specifier");
//                continue;
//            } else if (match("PRIVATE")) {
//                currentAccess = CppAccessSpecifier::Private;
//                consume("COLON", "Expected ':' after access specifier");
//                continue;
//            }

            // Спецификаторы доступа
            if (match("PUBLIC") || match("PROTECTED") || match("PRIVATE")) {
                if (previous().getType() == "PUBLIC") currentAccess = CppAccessSpecifier::Public;
                else if (previous().getType() == "PROTECTED") currentAccess = CppAccessSpecifier::Protected;
                else currentAccess = CppAccessSpecifier::Private;

                consume("COLON", "Expected ':' after access specifier");
                continue;
            }

            auto member = parseClassMember(currentAccess);
            if (member) {
                classDecl->members.push_back(std::move(member));
            } else {
                advance(); // skip problematic token
            }
        }

        currentClass = nullptr;
        consume("CLOSECURLY", "Expected '}' after class body");
        consume("SEMICOLON", "Expected ';' after class");

        return classDecl;
    }

    std::unique_ptr<CppDecl> parseClassMember(CppAccessSpecifier access) {
        // Пропускаем сложные случаи
        if (isTypeToken(peekType())) {
            size_t save = current;
            try {
                auto field = parseFieldDecl();
                if (field) {
                    field->access = access;
                }
                return field;
            }catch (...) {
                current = save;
            }
        }
        // Конструкторы и деструкторы
        if (isFunctionStart()||(check("IDENTIFIER") && peek().getValue() == currentClass->name) ||
            (check("BITNOT") && lookAhead(1).getValue() == currentClass->name)) {
            auto func = parseFunction();
            if (func) {
                return func;
            }
        }

        // Дружественные объявления
        if (match("FRIEND")) {
            // Пропускаем friend объявления для упрощения
            while (!isAtEnd() && !check("SEMICOLON")) {
                advance();
            }
            if (match("SEMICOLON")) {
                return nullptr; // friend объявления не добавляем в AST пока
            }
        }



        return nullptr;
    }

    Token previous() {
        static Token dummy;
        if (current == 0 || tokens.empty()) return dummy;
        return tokens[current - 1];
    }

    std::unique_ptr<CppFieldDecl> parseFieldDecl() {
        auto field = std::make_unique<CppFieldDecl>();
        field->typeName = parseTypeName();

        if (!check("IDENTIFIER")) {
            return nullptr;
        }

        field->name = advance().getValue();

        // Битовые поля
        if (match("COLON")) {
            // Пропускаем размер битового поля
            advance();
        }

        if (match("ASSIGN")) {
            field->initializer = parseExpression();
        }

        consume("SEMICOLON", "Expected ';' after field declaration");
        return field;
    }

    std::unique_ptr<CppFunctionDecl> parseFunction() {
        std::cout << "DEBUG: Starting parseFunction at token: " << peek().getValue() << std::endl;

        auto func = std::make_unique<CppFunctionDecl>();

        // Определяем, это конструктор или обычная функция
        bool isConstructor = false;
        bool isDestructor = false;

        // Проверяем, является ли это конструктором/деструктором
        if (currentClass) {
            if (check("IDENTIFIER") && peek().getValue() == currentClass->name) {
                isConstructor = true;
            } else if (check("BITNOT") && lookAhead(1).getType() == "IDENTIFIER" &&
                       lookAhead(1).getValue() == currentClass->name) {
                isDestructor = true;
            }
        }

        if (isConstructor || isDestructor) {
            // Конструктор или деструктор - нет возвращаемого типа
            if (isDestructor) {
                match("BITNOT"); // пропускаем ~
                func->name = "~" + advance().getValue(); // деструктор
            } else {
                func->name = advance().getValue(); // конструктор
            }
            func->returnType = ""; // конструкторы/деструкторы не имеют возвращаемого типа
        } else {
            // Возвращаемый тип
            func->returnType = parseTypeName();
            std::cout << "DEBUG: Return type: " << func->returnType << std::endl;

            // Имя функции
            if (check("IDENTIFIER") || check("OPERATOR")) {
                func->name = advance().getValue();
                std::cout << "DEBUG: Function name: " << func->name << std::endl;
            } else {
                std::cout << "DEBUG: Expected function name, got: " << peek().getValue() << std::endl;
                return nullptr;
            }
        }

        // Шаблонные специализации
        if (match("OPENANGLE")) {
            // Пропускаем template arguments для упрощения
            while (!isAtEnd() && !check("CLOSEANGLE")) {
                advance();
            }
            consume("CLOSEANGLE", "Expected '>'");
        }

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
//        // Квалификаторы
//        if (match("CONST")) {
//            func->isConst = true;
//        }
//        if (match("VOLATILE")) {
//            // volatile qualifier
//        }
//        if (match("NOEXCEPT")) {
//            func->isNoexcept = true;
//            if (match("OPENPARENTHESES")) {
//                // Пропускаем condition
//                parseExpression();
//                consume("CLOSEPARENTHESES", "Expected ')' after noexcept");
//            }
//        }
//        if (match("OVERRIDE")) {
//            func->isOverride = true;
//        }
//        if (match("FINAL")) {
//            func->isFinal = true;
//        }

        // Тело функции
        if (match("ASSIGN")) {
            if (match("ZERO")) { // = 0 (pure virtual)
                // pure virtual function
            } else if (match("DEFAULT")) { // = default
                // defaulted function
            } else if (match("DELETE")) { // = delete
                // deleted function
            }
            consume("SEMICOLON", "Expected ';'");
        } else if (match("OPENCURLY")) {
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
            match("MULTIASSIGN") || match("DIVASSIGN") || match("MODASSIGN")||
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

    std::unique_ptr<CppExpr>makeBinary(std::unique_ptr<CppExpr> left, CppBinOpKind kind, std::unique_ptr<CppExpr> right) {
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
        // Просто создаем новый экземпляр вместо преобразования
        try {
            auto decl = std::make_unique<CppVarDecl>();

            // Сохраняем позицию для отката
            size_t save = current;

            try {
                decl->typeName = parseTypeName();
            } catch (...) {
                current = save;
                return nullptr;
            }
            // Проверяем, что после типа идет идентификатор
            if (!check("IDENTIFIER")) {
                std::cout << "DEBUG: Expected identifier after type, got: " << peek().getValue() << std::endl;
                return nullptr;
            }

            auto nameTok = consume("IDENTIFIER", "Ожидалось имя переменной");
            decl->name = nameTok.getValue();


            // Обработка массивов
            if (match("OPENBRACKET")) {
                decl->typeName += "[]";
                // Пропускаем размер массива если есть
                if (!check("CLOSEBRACKET")) {
                    advance(); // пропускаем размер
                }
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
            match("VALUEINTEGER") || match("VALUEFLOAT")) {
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
            else if (match("MEMBERACCESS")||match("PTRACCESS")) {
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
        if (check("CLOSEPARENTHESES")) return params;

        do {
            CppParameter param;

            // Парсим полный тип (включая квалификаторы)
            param.typeName = parseTypeName();

            // Имя параметра (может отсутствовать)
            if (check("IDENTIFIER")) {
                param.name = advance().getValue();
            }

            // Значение по умолчанию
            if (match("ASSIGN")) {
                // Упрощенно - сохраняем как строку
                std::string defaultValue;
                while (!isAtEnd() && !check("COMMA") && !check("CLOSEPARENTHESES")) {
                    if (!defaultValue.empty()) defaultValue += " ";
                    defaultValue += advance().getValue();
                }
                param.defaultValue = defaultValue;
            }

            params.push_back(param);
        } while (match("COMMA"));

        return params;
    }

    std::string parseTypeName() {
        std::string typeName;
        bool inTemplate = false;
        int angleBracketDepth = 0;

        while (match("CONST") || match("VOLATILE")) {
            if (!typeName.empty()) typeName += " ";
            typeName += previous().getValue();
        }

        while (!isAtEnd()) {
            std::string type = peekType();
            // Базовый тип
            if (isTypeToken(peekType()) || check("IDENTIFIER")) {
                if (!typeName.empty()) typeName += " ";
                typeName += advance().getValue();

                // Квалифицированные имена (std::vector)
                while (match("SCOPE")) {
                    typeName += "::";
                    if (check("IDENTIFIER")) {
                        typeName += advance().getValue();
                    }
                }
                // Шаблонные параметры
                if (match("JL")) {
                    typeName += "<";
                    inTemplate = true;
                    angleBracketDepth++;
                    continue;
                }
            }
            else if (check("JG") && inTemplate) {
                typeName += ">";
                advance();
                angleBracketDepth--;
                if (angleBracketDepth == 0) inTemplate = false;
                continue;
            }
            else if (inTemplate && (check("COMMA") || isTypeToken(type))) {
                // Внутри шаблона - продолжаем парсить
                if (check("COMMA")) {
                    typeName += ",";
                    advance();
                }
                continue;
            }
            else {
                // Указатели и ссылки
                if (match("MULTI")) typeName += "*";
                else if (match("BITAND")) typeName += "&";
                else break;
            }
        }
        return typeName;
    }

    CppStmtPtr parseThrowStmt() {
        auto throwExpr = parseExpression();
        consume("SEMICOLON", "Expected ';' after throw");

        auto stmt = std::make_unique<CppExprStmt>();
        stmt->expression = std::move(throwExpr);
        return stmt;
    }

    std::unique_ptr<CppStmt> parseStatement() {
        if (isAtEnd()) return nullptr;

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
        } catch (const std::exception& e) {
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
        auto decl = std::make_unique<CppVarDecl>();
        decl->typeName = parseTypeName();

        if (!check("IDENTIFIER")) {
            return nullptr;
        }

        auto nameTok = advance();
        decl->name = nameTok.getValue();

        if (match("ASSIGN")) {
            decl->initializer = parseExpression();
        }

        if (requireSemicolon) {
            consume("SEMICOLON", "Expected ';' after declaration");
        }
        return decl;
    }

    CppStmtPtr parseCompoundStmt() {
        auto block = std::make_unique<CppCompoundStmt>();

        while (!isAtEnd() && !check("CLOSECURLY")) {
            if (auto stmt = parseStatement()) {
                block->statements.push_back(std::move(stmt));
            } else {
                advance();
            }
        }

        consume("CLOSECURLY", "Expected '}'");
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

//        // Просто проверяем паттерн: тип + идентификатор + (
//        if (!isTypeToken(peekType())) {
//            current = save;
//            return false;
//        }
//        advance(); // тип

        try {
            std::string typeName =parseTypeName();

            if (!check("IDENTIFIER") && !check("OPERATOR")) {
                current = save;
                return false;
            }
            advance(); // имя

            bool isFunc = check("OPENPARENTHESES");
            current = save;
            return isFunc;

        } catch (...) {
            current = save;
            return false;
        }
    }

    bool isGlobalVarStart() {
        size_t save = current;
        if (!isTypeToken(peekType())) {
            current = save;
            return false;
        }
        advance(); // тип

        if (!check("IDENTIFIER")) {
            current = save;
            return false;
        }
        advance(); // имя

        bool isVar = !check("OPENPARENTHESES"); // не функция если нет (
        current = save;
        return isVar;
//        try {
//            parseTypeName();
//            return check("IDENTIFIER") && !isFunctionStart();
//        } catch (...) {
//            current = save;
//            return false;
//        }
    }

    bool isDeclarationStart(const std::string &type) {
        return isTypeToken(type) || type == "CLASS" || type == "STRUCT" ||
               type == "ENUM" || type == "TYPEDEF" || type == "USING" ||
               type == "TEMPLATE" || type == "NAMESPACE";
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

};


#endif //AISDLAB_CPPPARSERTOAST_H
