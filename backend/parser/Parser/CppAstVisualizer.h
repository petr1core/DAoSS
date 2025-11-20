//
// Created by пк on 20.11.2025.
//

#ifndef AISDLAB_CPPASTVISUALIZER_H
#define AISDLAB_CPPASTVISUALIZER_H

#include "../CPPAst/CppVisitor.h"
#include "../CPPAst/CppAst.h"
#include "../CPPAst/CppExpr.h"
#include <iostream>
#include <string>
#include <sstream>

class CppAstVisualizer : public CppVisitor {
private:
    int indentLevel = 0;
    std::ostream& output;

    void indent() {
        for (int i = 0; i < indentLevel; ++i) {
            output << "  ";
        }
    }

    void printNode(const std::string& type, const std::string& value = "") {
        indent();
        output << type;
        if (!value.empty()) {
            output << " [" << value << "]";
        }
        output << "\n";
    }

    std::string accessSpecifierToString(CppAccessSpecifier access) {
        switch (access) {
            case CppAccessSpecifier::Public: return "public";
            case CppAccessSpecifier::Private: return "private";
            case CppAccessSpecifier::Protected: return "protected";
            default: return "unknown";
        }
    }

    std::string binOpToString(CppBinOpKind op) {
        switch (op) {
            case CppBinOpKind::Add: return "+";
            case CppBinOpKind::Sub: return "-";
            case CppBinOpKind::Mul: return "*";
            case CppBinOpKind::Div: return "/";
            case CppBinOpKind::Mod: return "%";
            case CppBinOpKind::And: return "&&";
            case CppBinOpKind::Or: return "||";
            case CppBinOpKind::BitAnd: return "&";
            case CppBinOpKind::BitOr: return "|";
            case CppBinOpKind::BitXor: return "^";
            case CppBinOpKind::Shl: return "<<";
            case CppBinOpKind::Shr: return ">>";
            case CppBinOpKind::Lt: return "<";
            case CppBinOpKind::Gt: return ">";
            case CppBinOpKind::Le: return "<=";
            case CppBinOpKind::Ge: return ">=";
            case CppBinOpKind::Eq: return "==";
            case CppBinOpKind::Ne: return "!=";
            case CppBinOpKind::Assign: return "=";
            case CppBinOpKind::AddAssign: return "+=";
            case CppBinOpKind::SubAssign: return "-=";
            case CppBinOpKind::MulAssign: return "*=";
            case CppBinOpKind::DivAssign: return "/=";
            case CppBinOpKind::ModAssign: return "%=";
            default: return "?";
        }
    }

public:
    CppAstVisualizer(std::ostream& os = std::cout) : output(os) {}

    // ============ EXPRESSIONS ============

    void visit(CppIdentifier& expr) override {
        printNode("CppIdentifier", expr.name);
    }

    void visit(CppStringLiteral& expr) override {
        printNode("CppStringLiteral", "\"" + expr.value + "\"");
    }

    void visit(CppIntLiteral& expr) override {
        printNode("CppIntLiteral", std::to_string(expr.value));
    }

    void visit(CppRealLiteral& expr) override {
        printNode("CppRealLiteral", std::to_string(expr.value));
    }

    void visit(CppBoolLiteral& expr) override {
        printNode("CppBoolLiteral", expr.value ? "true" : "false");
    }

    void visit(CppNullptrLiteral& expr) override {
        printNode("CppNullptrLiteral", "nullptr");
    }

    void visit(CppUnaryOp& expr) override {
        printNode("CppUnaryOp", expr.op + (expr.postfix ? " (postfix)" : " (prefix)"));
        indentLevel++;
        expr.operand->accept(*this);
        indentLevel--;
    }

    void visit(CppBinaryOp& expr) override {
        printNode("CppBinaryOp", binOpToString(expr.op));
        indentLevel++;
        expr.left->accept(*this);
        expr.right->accept(*this);
        indentLevel--;
    }

    void visit(CppTernaryOp& expr) override {
        printNode("CppTernaryOp", "? :");
        indentLevel++;
        printNode("Condition");
        expr.condition->accept(*this);
        printNode("Then");
        expr.thenExpr->accept(*this);
        printNode("Else");
        expr.elseExpr->accept(*this);
        indentLevel--;
    }

    void visit(CppCallExpr& expr) override {
        printNode("CppCallExpr");
        indentLevel++;
        printNode("Callee");
        expr.callee->accept(*this);
        printNode("Arguments");
        indentLevel++;
        for (auto& arg : expr.arguments) {
            arg->accept(*this);
        }
        indentLevel--;
        indentLevel--;
    }

    void visit(CppArrayAccessExpr& expr) override {
        printNode("CppArrayAccessExpr");
        indentLevel++;
        printNode("Array");
        expr.array->accept(*this);
        printNode("Index");
        expr.index->accept(*this);
        indentLevel--;
    }

    void visit(CppMemberAccessExpr& expr) override {
        printNode("CppMemberAccessExpr", expr.member + (expr.isPointerAccess ? " (->)" : " (.)"));
        indentLevel++;
        expr.object->accept(*this);
        indentLevel--;
    }

    void visit(CppPointerToMemberExpr& expr) override {
        printNode("CppPointerToMemberExpr", expr.member + (expr.isArrow ? " (->*)" : " (.*)"));
        indentLevel++;
        expr.object->accept(*this);
        indentLevel--;
    }

    void visit(CppNewExpr& expr) override {
        printNode("CppNewExpr", expr.typeName + (expr.isArray ? "[]" : ""));
        if (!expr.arguments.empty()) {
            indentLevel++;
            printNode("Arguments");
            indentLevel++;
            for (auto& arg : expr.arguments) {
                arg->accept(*this);
            }
            indentLevel--;
            indentLevel--;
        }
    }

    void visit(CppDeleteExpr& expr) override {
        printNode("CppDeleteExpr", expr.isArray ? "delete[]" : "delete");
        indentLevel++;
        expr.operand->accept(*this);
        indentLevel--;
    }

    void visit(CppThisExpr& expr) override {
        printNode("CppThisExpr", "this");
    }

    void visit(CppSizeofExpr& expr) override {
        printNode("CppSizeofExpr", expr.isType ? "sizeof(type)" : "sizeof(expr)");
        indentLevel++;
        expr.expression->accept(*this);
        indentLevel--;
    }

    void visit(CppTypeidExpr& expr) override {
        printNode("CppTypeidExpr", expr.isType ? "typeid(type)" : "typeid(expr)");
        indentLevel++;
        expr.expression->accept(*this);
        indentLevel--;
    }

    void visit(CppTemplateIdExpr& expr) override {
        printNode("CppTemplateIdExpr", expr.templateName);
        indentLevel++;
        printNode("TemplateArguments");
        indentLevel++;
        for (auto& arg : expr.templateArgs) {
            arg->accept(*this);
        }
        indentLevel--;
        indentLevel--;
    }

    void visit(CppLambdaExpr& expr) override {
        printNode("CppLambdaExpr");
        indentLevel++;
        if (!expr.captures.empty()) {
            printNode("Captures");
            indentLevel++;
            for (auto& capture : expr.captures) {
                std::string captureStr = capture.name;
                if (capture.isThis) captureStr = "this";
                if (capture.byReference) captureStr = "&" + captureStr;
                if (capture.init) {
                    captureStr += " = ";
                    // TODO: convert expression to string
                }
                printNode("Capture", captureStr);
            }
            indentLevel--;
        }
        if (!expr.parameters.empty()) {
            printNode("Parameters");
            indentLevel++;
            for (auto& param : expr.parameters) {
                printNode("Parameter", param->typeName + " " + param->name);
            }
            indentLevel--;
        }
        if (expr.returnType) {
            printNode("ReturnType");
            expr.returnType->accept(*this);
        }
        if (expr.body) {
            printNode("Body");
            expr.body->accept(*this);
        }
        indentLevel--;
    }

    void visit(CppCastExpr& expr) override {
        printNode("CppCastExpr", expr.castType + "<" + expr.targetType + ">");
        indentLevel++;
        expr.expression->accept(*this);
        indentLevel--;
    }

    // ============ STATEMENTS ============

    void visit(CppExprStmt& stmt) override {
        printNode("CppExprStmt");
        if (stmt.expression) {
            indentLevel++;
            stmt.expression->accept(*this);
            indentLevel--;
        }
    }

    void visit(CppCompoundStmt& stmt) override {
        printNode("CppCompoundStmt");
        indentLevel++;
        for (auto& st : stmt.statements) {
            st->accept(*this);
        }
        indentLevel--;
    }

    void visit(CppReturnStmt& stmt) override {
        printNode("CppReturnStmt");
        if (stmt.value) {
            indentLevel++;
            stmt.value->accept(*this);
            indentLevel--;
        }
    }

    void visit(CppBreakStmt& stmt) override {
        printNode("CppBreakStmt");
    }

    void visit(CppContinueStmt& stmt) override {
        printNode("CppContinueStmt");
    }

    void visit(CppIfStmt& stmt) override {
        printNode("CppIfStmt");
        indentLevel++;
        printNode("Condition");
        stmt.condition->accept(*this);
        printNode("Then");
        stmt.thenBranch->accept(*this);
        if (stmt.elseBranch) {
            printNode("Else");
            stmt.elseBranch->accept(*this);
        }
        indentLevel--;
    }

    void visit(CppWhileStmt& stmt) override {
        printNode("CppWhileStmt");
        indentLevel++;
        printNode("Condition");
        stmt.condition->accept(*this);
        printNode("Body");
        stmt.body->accept(*this);
        indentLevel--;
    }

    void visit(CppDoWhileStmt& stmt) override {
        printNode("CppDoWhileStmt");
        indentLevel++;
        printNode("Body");
        stmt.body->accept(*this);
        printNode("Condition");
        stmt.condition->accept(*this);
        indentLevel--;
    }

    void visit(CppForStmt& stmt) override {
        printNode("CppForStmt");
        indentLevel++;
        if (stmt.init) {
            printNode("Init");
            stmt.init->accept(*this);
        }
        if (stmt.condition) {
            printNode("Condition");
            stmt.condition->accept(*this);
        }
        if (stmt.increment) {
            printNode("Increment");
            stmt.increment->accept(*this);
        }
        printNode("Body");
        stmt.body->accept(*this);
        indentLevel--;
    }

    void visit(CppRangeForStmt& stmt) override {
        printNode("CppRangeForStmt");
        indentLevel++;
        if (stmt.init) {
            printNode("Init");
            stmt.init->accept(*this);
        }
        if (stmt.range) {
            printNode("Range");
            stmt.range->accept(*this);
        }
        printNode("Body");
        stmt.body->accept(*this);
        indentLevel--;
    }

    void visit(CppSwitchStmt& stmt) override {
        printNode("CppSwitchStmt");
        indentLevel++;
        printNode("Condition");
        stmt.condition->accept(*this);
        printNode("Cases");
        indentLevel++;
        for (auto& caseStmt : stmt.cases) {
            caseStmt->accept(*this);
        }
        indentLevel--;
        indentLevel--;
    }

    void visit(CppCaseStmt& stmt) override {
        printNode("CppCaseStmt");
        indentLevel++;
        if (stmt.value) {
            printNode("Value");
            stmt.value->accept(*this);
        }
        printNode("Body");
        indentLevel++;
        for (auto& bodyStmt : stmt.body) {
            bodyStmt->accept(*this);
        }
        indentLevel--;
        indentLevel--;
    }

    void visit(CppDefaultStmt& stmt) override {
        printNode("CppDefaultStmt");
        indentLevel++;
        printNode("Body");
        indentLevel++;
        for (auto& bodyStmt : stmt.body) {
            bodyStmt->accept(*this);
        }
        indentLevel--;
        indentLevel--;
    }

    void visit(CppTryStmt& stmt) override {
        printNode("CppTryStmt");
        indentLevel++;
        printNode("TryBlock");
        stmt.tryBlock->accept(*this);
        printNode("Handlers");
        indentLevel++;
        for (auto& handler : stmt.handlers) {
            handler->accept(*this);
        }
        indentLevel--;
        indentLevel--;
    }

    void visit(CppCatchStmt& stmt) override {
        printNode("CppCatchStmt", stmt.exceptionType + " " + stmt.exceptionName);
        indentLevel++;
        printNode("Body");
        stmt.body->accept(*this);
        indentLevel--;
    }

    void visit(CppVarDecl& stmt) override {
        printNode("CppVarDecl", stmt.typeName + " " + stmt.name);
        if (stmt.initializer) {
            indentLevel++;
            printNode("Initializer");
            stmt.initializer->accept(*this);
            indentLevel--;
        }
    }

    void visit(CppAssignStmt& stmt) override {
        printNode("CppAssignStmt", stmt.op);
        indentLevel++;
        printNode("Target");
        stmt.target->accept(*this);
        printNode("Value");
        stmt.value->accept(*this);
        indentLevel--;
    }

    // ============ DECLARATIONS ============

    void visit(CppFunctionDecl& decl) override {
        printNode("CppFunctionDecl", decl.returnType + " " + decl.name +
                  (decl.isVirtual ? " virtual" : "") +
                  (decl.isOverride ? " override" : "") +
                  (decl.isConst ? " const" : ""));
        indentLevel++;

        printNode("Parameters");
        indentLevel++;
        for (auto& param : decl.parameters) {
            printNode("Parameter", param.typeName + " " + param.name +
                      (param.defaultValue.empty() ? "" : " = " + param.defaultValue));
        }
        indentLevel--;

        if (decl.body) {
            printNode("Body");
            decl.body->accept(*this);
        }
        indentLevel--;
    }

    void visit(CppTemplateDecl& decl) override {
        printNode("CppTemplateDecl");
        indentLevel++;
        printNode("TemplateParameters");
        indentLevel++;
        for (auto& param : decl.parameters) {
            printNode("TemplateParameter", param.kind + " " + param.name);
        }
        indentLevel--;
        if (decl.declaration) {
            printNode("Declaration");
            decl.declaration->accept(*this);
        }
        indentLevel--;
    }

    void visit(CppClassDecl& decl) override {
        printNode("CppClassDecl", (decl.isStruct ? "struct " : "class ") + decl.name);
        indentLevel++;

        if (!decl.baseClasses.empty()) {
            printNode("BaseClasses");
            indentLevel++;
            for (auto& base : decl.baseClasses) {
                printNode("BaseClass", base);
            }
            indentLevel--;
        }

        printNode("Members");
        indentLevel++;
        for (auto& member : decl.members) {
            member->accept(*this);
        }
        indentLevel--;
        indentLevel--;
    }

    void visit(CppFieldDecl& decl) override {
        printNode("CppFieldDecl", accessSpecifierToString(decl.access) + ": " +
                  decl.typeName + " " + decl.name +
                  (decl.isMutable ? " mutable" : ""));
        if (decl.initializer) {
            indentLevel++;
            printNode("Initializer");
            decl.initializer->accept(*this);
            indentLevel--;
        }
    }

    void visit(CppNamespaceDecl& decl) override {
        printNode("CppNamespaceDecl", decl.name);
        indentLevel++;
        if (decl.body) {
            decl.body->accept(*this);
        }
        indentLevel--;
    }

    void visit(CppUsingDecl& decl) override {
        if (decl.isNamespace) {
            printNode("CppUsingNamespaceDecl", decl.target);
        } else {
            printNode("CppUsingDecl", decl.name);
        }
    }

    void visit(CppTypedefDecl& decl) override {
        printNode("CppTypedefDecl", decl.typeName + " -> " + decl.alias);
    }

    void visit(CppUsingAliasDecl& decl) override {
        printNode("CppUsingAliasDecl", decl.typeName + " -> " + decl.alias);
    }

    void visit(CppEnumDecl& decl) override {
        printNode("CppEnumDecl", (decl.isScoped ? "enum class " : "enum ") + decl.name);
        indentLevel++;
        if (!decl.underlyingType.empty()) {
            printNode("UnderlyingType", decl.underlyingType);
        }
        printNode("Enumerators");
        indentLevel++;
        for (auto& enumerator : decl.enumerators) {
            printNode("Enumerator", enumerator.first);
            if (enumerator.second) {
                indentLevel++;
                enumerator.second->accept(*this);
                indentLevel--;
            }
        }
        indentLevel--;
        indentLevel--;
    }

    void visit(CppPreprocessorDirective& decl) override {
        printNode("CppPreprocessorDirective", decl.directive + " " + decl.value);
    }

    void visit(CppProgram& program) override {
        printNode("CppProgram", program.name);
        indentLevel++;
        if (program.body) {
            program.body->accept(*this);
        }
        indentLevel--;
    }
};

#endif //AISDLAB_CPPASTVISUALIZER_H
