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

class CppParserToAST {
public:
    std::unique_ptr<Program> parse(const std::string& code) {
        Lexer lexer(code, types::C);
        tokens = lexer.getTokenList();
        // Фильтруем пробелы и комментарии, если они вдруг остались
        std::vector<Token> filtered;
        filtered.reserve(tokens.size());
        for (const auto& token : tokens) {
            if (token.getType() == "SPACE" || token.getType() == "COMMENT") {
                continue;
            }
            filtered.push_back(token);
        }
        tokens.swap(filtered);
        current = 0;

        auto program = std::make_unique<Program>();
        program->name = "translation_unit";
        program->body = parseTranslationUnit();
        return program;
    }

private:
    std::vector<Token> tokens;
    size_t current{0};

    std::unique_ptr<Block> parseTranslationUnit() {
        auto block = std::make_unique<Block>();
        while (!isAtEnd()) {
            if (isPreprocessor(peekType())) {
                advance();
                continue;
            }

            if (isFunctionDefinitionAhead()) {
                block->statements.push_back(parseFunction());
                continue;
            }

            if (auto stmt = parseStatement()) {
                block->statements.push_back(std::move(stmt));
            } else {
                advance();
            }
        }
        return block;
    }

    std::unique_ptr<Stmt> parseStatement() {
        if (isAtEnd()) {
            return nullptr;
        }

        if (match("SEMICOLON")) {
            return nullptr; // пустой оператор
        }

        if (match("OPENCURLY")) {
            return parseBlockBody();
        }

        if (match("IF")) {
            return parseIf();
        }

        if (match("WHILE")) {
            return parseWhile();
        }

        if (match("FOR")) {
            return parseFor();
        }

        if (match("RETURN")) {
            return parseReturn();
        }

        if (isTypeToken(peekType())) {
            return parseVarDecl();
        }

        if (auto assign = parseAssignmentStmt()) {
            return assign;
        }

        return parseExprStmt();
    }

    std::unique_ptr<Stmt> parseFunction() {
        auto func = std::make_unique<FunctionDecl>();
        func->returnType = parseTypeName();
        auto nameTok = consume("IDENTIFIER", "Ожидалось имя функции");
        func->name = nameTok.getValue();
        consume("OPENPARENTHESES", "Ожидалась '(' после имени функции");
        func->params = parseParameters();
        consume("CLOSEPARENTHESES", "Ожидалась ')' после параметров");

        if (match("SEMICOLON")) {
            func->body = nullptr;
            return func;
        }

        if (!match("OPENCURLY")) {
            throw std::runtime_error("Ожидался блок тела функции");
        }
        func->body = parseBlockBody();
        return func;
    }

    std::unique_ptr<Block> parseBlockBody() {
        auto block = std::make_unique<Block>();
        while (!isAtEnd() && !check("CLOSECURLY")) {
            if (auto stmt = parseStatement()) {
                block->statements.push_back(std::move(stmt));
            }
        }
        consume("CLOSECURLY", "Ожидался символ '}'");
        return block;
    }

    std::unique_ptr<Stmt> parseIf() {
        consume("OPENPARENTHESES", "Ожидалась '(' после if");
        auto condition = parseExpression();
        consume("CLOSEPARENTHESES", "Ожидалась ')' после условия if");

        auto thenBranch = parseStatement();
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
        consume("CLOSEPARENTHESES", "Ожидалась ')' после условия while");
        auto body = parseStatement();

        auto stmt = std::make_unique<WhileStmt>();
        stmt->condition = std::move(condition);
        stmt->body = std::move(body);
        return stmt;
    }

    std::unique_ptr<Stmt> parseFor() {
        consume("OPENPARENTHESES", "Ожидалась '(' после for");
        std::unique_ptr<Stmt> init;
        if (!check("SEMICOLON")) {
            init = parseSimpleStatement(false);
        }
        consume("SEMICOLON", "Ожидался ';' после инициализации for");

        std::unique_ptr<Expr> condition;
        if (!check("SEMICOLON")) {
            condition = parseExpression();
        }
        consume("SEMICOLON", "Ожидался ';' после условия for");

        std::unique_ptr<Stmt> increment;
        if (!check("CLOSEPARENTHESES")) {
            increment = parseSimpleStatement(false);
        }
        consume("CLOSEPARENTHESES", "Ожидалась ')' после выражения for");

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
        auto decl = std::make_unique<VarDeclStmt>();
        decl->typeName = parseTypeName();
        auto nameTok = consume("IDENTIFIER", "Ожидалось имя переменной");
        decl->name = nameTok.getValue();
        if (match("ASSIGN")) {
            decl->initializer = parseExpression();
        }
        if (requireSemicolon) {
            consume("SEMICOLON", "Ожидался ';' после объявления");
        }
        return decl;
    }

    std::unique_ptr<Stmt> parseAssignmentStmt(bool requireSemicolon = true) {
        if (!check("IDENTIFIER")) {
            return nullptr;
        }
        size_t save = current;
        auto ident = advance();
        if (!match("ASSIGN")) {
            current = save;
            return nullptr;
        }
        auto assign = std::make_unique<AssignStmt>();
        assign->target = ident.getValue();
        assign->value = parseExpression();
        if (requireSemicolon) {
            consume("SEMICOLON", "Ожидался ';' после присваивания");
        }
        return assign;
    }

    std::unique_ptr<Stmt> parseExprStmt(bool requireSemicolon = true) {
        auto expr = parseExpression();
        if (requireSemicolon) {
            consume("SEMICOLON", "Ожидался ';' после выражения");
        }
        auto stmt = std::make_unique<ExprStmt>();
        stmt->expression = std::move(expr);
        return stmt;
    }

    std::unique_ptr<Stmt> parseSimpleStatement(bool requireSemicolon) {
        if (isTypeToken(peekType())) {
            return parseVarDecl(requireSemicolon);
        }
        if (auto assign = parseAssignmentStmt(requireSemicolon)) {
            return assign;
        }
        return parseExprStmt(requireSemicolon);
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
        if (!isTypeToken(peekType())) {
            throw std::runtime_error("Ожидался тип");
        }
        std::string typeName;
        while (!isAtEnd() && isTypeToken(peekType())) {
            if (!typeName.empty()) {
                typeName += " ";
            }
            typeName += advance().getValue();
        }
        while (match("MULTI")) {
            typeName += "*";
        }
        return typeName;
    }

    std::unique_ptr<Expr> parseExpression() {
        return parseLogicalOr();
    }

    std::unique_ptr<Expr> parseLogicalOr() {
        auto expr = parseLogicalAnd();
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

    std::unique_ptr<Expr> parseUnary() {
        if (match("INCREMENT")) {
            auto operand = parseUnary();
            return makeUnary("++", std::move(operand), false);
        }
        if (match("DECREMENT")) {
            auto operand = parseUnary();
            return makeUnary("--", std::move(operand), false);
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
            auto name = previous().getValue();
            if (match("OPENPARENTHESES")) {
                auto call = std::make_unique<CallExpr>();
                call->callee = name;
                if (!check("CLOSEPARENTHESES")) {
                    do {
                        call->arguments.push_back(parseExpression());
                    } while (match("COMMA"));
                }
                consume("CLOSEPARENTHESES", "Ожидалась ')' после аргументов");
                return call;
            }
            auto ident = std::make_unique<Identifier>();
            ident->name = name;
            return ident;
        }

        throw std::runtime_error("Неожиданное выражение");
    }

    std::unique_ptr<Expr> makeUnary(const std::string& op, std::unique_ptr<Expr> operand, bool postfix) {
        auto node = std::make_unique<UnaryOp>();
        node->op = op;
        node->postfix = postfix;
        node->operand = std::move(operand);
        return node;
    }

    std::unique_ptr<Expr> makeBinary(std::unique_ptr<Expr> left, BinOpKind kind, std::unique_ptr<Expr> right) {
        auto node = std::make_unique<BinaryOp>();
        node->op = kind;
        node->left = std::move(left);
        node->right = std::move(right);
        return node;
    }

    bool isFunctionDefinitionAhead() {
        size_t i = current;
        if (i >= tokens.size() || !isTypeToken(tokens[i].getType())) {
            return false;
        }
        while (i < tokens.size() && isTypeToken(tokens[i].getType())) {
            ++i;
        }
        while (i < tokens.size() && tokens[i].getType() == "MULTI") {
            ++i;
        }
        if (i >= tokens.size() || tokens[i].getType() != "IDENTIFIER") {
            return false;
        }
        ++i;
        if (i >= tokens.size()) {
            return false;
        }
        return tokens[i].getType() == "OPENPARENTHESES";
    }

    bool isTypeToken(const std::string& type) const {
        static const std::vector<std::string> typeTokens = {
                "VOID", "INT", "FLOAT", "DOUBLE", "CHAR", "SHORT", "LONG",
                "SIGNED", "UNSIGNED", "STRUCT", "CONST", "STATIC"
        };
        return std::find(typeTokens.begin(), typeTokens.end(), type) != typeTokens.end();
    }

    bool isPreprocessor(const std::string& type) const {
        return type == "INCLUDE" || type == "DEFINE" || type == "PREPROCESSOR";
    }

    bool match(const std::string& type) {
        if (check(type)) {
            advance();
            return true;
        }
        return false;
    }

    bool check(const std::string& type) const {
        if (isAtEnd()) {
            return false;
        }
        return peek().getType() == type;
    }

    Token advance() {
        if (!isAtEnd()) {
            ++current;
        }
        return tokens[current - 1];
    }

    const Token& peek() const {
        static const Token dummy;
        if (tokens.empty()) {
            return dummy;
        }
        if (isAtEnd()) {
            return tokens.back();
        }
        return tokens[current];
    }

    const std::string& peekType() const {
        if (isAtEnd()) {
            static const std::string empty;
            return empty;
        }
        return tokens[current].getType();
    }

    const Token& previous() const {
        return tokens[current - 1];
    }

    bool isAtEnd() const {
        return current >= tokens.size();
    }

    Token consume(const std::string& type, const std::string& message) {
        if (check(type)) {
            return advance();
        }
        throw std::runtime_error(message);
    }
};

#endif // SIMPLE_CPP_PARSER_H
