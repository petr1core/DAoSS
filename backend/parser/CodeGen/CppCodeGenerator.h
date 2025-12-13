#ifndef CODEGEN_CPP_CODE_GENERATOR_H
#define CODEGEN_CPP_CODE_GENERATOR_H

#include <string>
#include <sstream>
#include "../Ast/Visitor.h"
#include "../Ast/Expr.h"
#include "../Ast/Stmt.h"
#include "../Ast/ExprAcceptImpl.h"
#include "../Scripts/json.hpp"


class CppCodeGenerator {
private:
    nlohmann::json currentJson;
    std::vector<nlohmann::json> jsonStack;

    void pushJson(const nlohmann::json& json) {
        jsonStack.push_back(currentJson);
        currentJson = json;
    }

    void popJson() {
        if (!jsonStack.empty()) {
            currentJson = jsonStack.back();
            jsonStack.pop_back();
        }
    }

public:
//    nlohmann::json convert(Program &program) {
//        currentJson = nlohmann::json::object();
//        jsonStack.clear();
//        program.accept(*this);
//        return currentJson;
//    }
//
//    void visit(Identifier &n) override {
//        currentJson["type"] = "identifier";
//        currentJson["name"] = n.name;
//    }
//
//    void visit(StringLiteral &n) override {
//        currentJson["type"] = "stringLiteral";
//        currentJson["value"] = n.value;
//    }
//
//    void visit(IntLiteral &n) override {
//        currentJson["type"] = "intLiteral";
//        currentJson["value"] = n.value;
//    }
//
//    void visit(RealLiteral &n) override {
//        currentJson["type"] = "realLiteral";
//        currentJson["value"] = n.value;
//    }
//
//    void visit(UnaryOp &n) override {
//        nlohmann::json json;
//        json["type"] = "unaryOperation";
//        json["operator"] = n.op;
//        json["postfix"] = n.postfix;
//        json["operand"] = n.operand ? captureExpr(*n.operand) : nlohmann::json();
//        currentJson = json;
//    }
//
//    void visit(BinaryOp &n) override {
//        nlohmann::json json;
//        json["type"] = "binaryOperation";
//        json["operator"] = opToStr(n.op);
//        json["left"] = n.left ? captureExpr(*n.left) : nlohmann::json();
//        json["right"] = n.right ? captureExpr(*n.right) : nlohmann::json();
//        currentJson = json;
//    }
//
//    void visit(CallExpr &n) override {
//        nlohmann::json json;
//        json["type"] = "call";
//        json["callee"] = n.callee;
//        nlohmann::json args = nlohmann::json::array();
//        for (auto &arg : n.arguments) {
//            if (arg) {
//                args.push_back(captureExpr(*arg));
//            }
//        }
//        json["arguments"] = args;
//        currentJson = json;
//    }
//
//    void visit(AssignStmt &s) override {
//        nlohmann::json json;
//        json["type"] = "assignment";
//        json["target"] = s.target;
//        json["value"] = s.value ? captureExpr(*s.value) : nlohmann::json();
//        currentJson = json;
//    }
//
//    void visit(ExprStmt &s) override {
//        nlohmann::json json;
//        json["type"] = "exprStmt";
//        json["expression"] = s.expression ? captureExpr(*s.expression) : nlohmann::json();
//        currentJson = json;
//    }
//
//    void visit(VarDeclStmt &s) override {
//        nlohmann::json json;
//        json["type"] = "varDecl";
//        json["varType"] = s.typeName;
//        json["name"] = s.name;
//        json["initializer"] = s.initializer ? captureExpr(*s.initializer) : nlohmann::json();
//        currentJson = json;
//    }
//
//    void visit(IfStmt &s) override {
//        nlohmann::json json;
//        json["type"] = "if";
//        json["condition"] = s.condition ? captureExpr(*s.condition) : nlohmann::json();
//        json["then"] = s.thenBranch ? captureStmt(*s.thenBranch) : nlohmann::json();
//        json["else"] = s.elseBranch ? captureStmt(*s.elseBranch) : nlohmann::json();
//        currentJson = json;
//    }
//
//    void visit(WhileStmt &s) override {
//        nlohmann::json json;
//        json["type"] = "while";
//        json["condition"] = s.condition ? captureExpr(*s.condition) : nlohmann::json();
//        json["body"] = s.body ? captureStmt(*s.body) : nlohmann::json();
//        currentJson = json;
//    }
//
//    void visit(ForStmt &s) override {
//        nlohmann::json json;
//        json["type"] = "for";
//        json["init"] = s.init ? captureStmt(*s.init) : nlohmann::json();
//        json["condition"] = s.condition ? captureExpr(*s.condition) : nlohmann::json();
//        json["increment"] = s.increment ? captureStmt(*s.increment) : nlohmann::json();
//        json["body"] = s.body ? captureStmt(*s.body) : nlohmann::json();
//        currentJson = json;
//    }
//
//    void visit(ReturnStmt &s) override {
//        nlohmann::json json;
//        json["type"] = "return";
//        json["value"] = s.value ? captureExpr(*s.value) : nlohmann::json();
//        currentJson = json;
//    }
//
//    void visit(WriteStmt &s) override {
//        nlohmann::json json;
//        json["type"] = "output";
//        json["operation"] = s.newline ? "coutln" : "cout";
//
//        nlohmann::json args = nlohmann::json::array();
//        for (auto &arg : s.args) {
//            if (arg) {
//                args.push_back(captureExpr(*arg));
//            }
//        }
//        json["arguments"] = args;
//
//        currentJson = json;
//    }
//
//    void visit(Block &b) override {
//        nlohmann::json json;
//        json["type"] = "block";
//
//        nlohmann::json statements = nlohmann::json::array();
//        for (auto &st : b.statements) {
//            if (st) {
//                statements.push_back(captureStmt(*st));
//            }
//        }
//        json["statements"] = statements;
//
//        currentJson = json;
//    }
//
//    void visit(FunctionDecl &f) override {
//        nlohmann::json json;
//        json["type"] = "function";
//        json["returnType"] = f.returnType;
//        json["name"] = f.name;
//
//        nlohmann::json params = nlohmann::json::array();
//        for (const auto &param : f.params) {
//            params.push_back({
//                                     {"type", param.typeName},
//                                     {"name", param.name}
//                             });
//        }
//        json["parameters"] = params;
//        json["body"] = f.body ? captureStmt(*f.body) : nlohmann::json();
//        currentJson = json;
//    }
//
//    void visit(Program &p) override {
//        nlohmann::json json;
//        json["type"] = "program";
//        json["name"] = p.name;
//
//        if (p.body) {
//            json["body"] = captureStmt(*p.body);
//        }
//
//        currentJson = json;
//    }
//
//private:
//    template<typename Node>
//    nlohmann::json captureNode(Node &node) {
//        pushJson(nlohmann::json::object());
//        node.accept(*this);
//        nlohmann::json json = currentJson;
//        popJson();
//        return json;
//    }
//
//    nlohmann::json captureExpr(Expr &expr) {
//        return captureNode(expr);
//    }
//
//    nlohmann::json captureExpr(Expr *expr) {
//        if (!expr) {
//            return nlohmann::json();
//        }
//        return captureExpr(*expr);
//    }
//
//    nlohmann::json captureStmt(Stmt &stmt) {
//        return captureNode(stmt);
//    }
//
//    nlohmann::json captureStmt(Stmt *stmt) {
//        if (!stmt) {
//            return nlohmann::json();
//        }
//        return captureStmt(*stmt);
//    }
//
//    static std::string opToStr(BinOpKind k) {
//        switch (k) {
//            case BinOpKind::Add: return "+";
//            case BinOpKind::Sub: return "-";
//            case BinOpKind::Mul: return "*";
//            case BinOpKind::Div: return "/";
//            case BinOpKind::Mod: return "%";
//            case BinOpKind::And: return "&&";
//            case BinOpKind::Or: return "||";
//            case BinOpKind::Xor: return "^";
//            case BinOpKind::BitAnd: return "&";
//            case BinOpKind::BitOr: return "|";
//            case BinOpKind::BitXor: return "^";
//            case BinOpKind::Shl: return "<<";
//            case BinOpKind::Shr: return ">>";
//            case BinOpKind::Lt: return "<";
//            case BinOpKind::Gt: return ">";
//            case BinOpKind::Le: return "<=";
//            case BinOpKind::Ge: return ">=";
//            case BinOpKind::Eq: return "==";
//            case BinOpKind::Ne: return "!=";
//        }
//        return "?";
//    }
};

#endif // CODEGEN_CPP_CODE_GENERATOR_H
