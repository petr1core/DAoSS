//
// Created by пк on 18.11.2025.
//

#ifndef AST_VISUALIZER_H
#define AST_VISUALIZER_H

#include "../Ast/Visitor.h"
#include <iostream>
#include <string>

class AstVisualizer : public Visitor {
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

public:
    AstVisualizer(std::ostream& os = std::cout) : output(os) {}

    // Expressions
    void visit(Identifier& expr) override {
        printNode("Identifier", expr.name);
    }

    void visit(StringLiteral& expr) override {
        printNode("StringLiteral", "\"" + expr.value + "\"");
    }

    void visit(IntLiteral& expr) override {
        printNode("IntLiteral", std::to_string(expr.value));
    }

    void visit(RealLiteral& expr) override {
        printNode("RealLiteral", std::to_string(expr.value));
    }

    void visit(UnaryOp& expr) override {
        printNode("UnaryOp", expr.op + (expr.postfix ? " (postfix)" : " (prefix)"));
        indentLevel++;
        expr.operand->accept(*this);
        indentLevel--;
    }

    void visit(BinaryOp& expr) override {
        std::string opStr;
        switch (expr.op) {
            case BinOpKind::Add: opStr = "+"; break;
            case BinOpKind::Sub: opStr = "-"; break;
            case BinOpKind::Mul: opStr = "*"; break;
            case BinOpKind::Div: opStr = "/"; break;
            case BinOpKind::Mod: opStr = "%"; break;
            case BinOpKind::And: opStr = "&&"; break;
            case BinOpKind::Or: opStr = "||"; break;
            case BinOpKind::BitAnd: opStr = "&"; break;
            case BinOpKind::BitOr: opStr = "|"; break;
            case BinOpKind::BitXor: opStr = "^"; break;
            case BinOpKind::Shl: opStr = "<<"; break;
            case BinOpKind::Shr: opStr = ">>"; break;
            case BinOpKind::Lt: opStr = "<"; break;
            case BinOpKind::Gt: opStr = ">"; break;
            case BinOpKind::Le: opStr = "<="; break;
            case BinOpKind::Ge: opStr = ">="; break;
            case BinOpKind::Eq: opStr = "=="; break;
            case BinOpKind::Ne: opStr = "!="; break;
            default: opStr = "?"; break;
        }
        printNode("BinaryOp", opStr);
        indentLevel++;
        expr.left->accept(*this);
        expr.right->accept(*this);
        indentLevel--;
    }

    void visit(CallExpr& expr) override {
        printNode("CallExpr", expr.callee);
        indentLevel++;
        for (auto& arg : expr.arguments) {
            arg->accept(*this);
        }
        indentLevel--;
    }

    void visit(TernaryOp& expr) override {
        printNode("TernaryOp", "? :");
        indentLevel++;
        expr.condition->accept(*this);
        printNode("Then");
        expr.thenExpr->accept(*this);
        printNode("Else");
        expr.elseExpr->accept(*this);
        indentLevel--;
    }

    void visit(SizeofExpr& expr) override {
        printNode("SizeofExpr");
        if (expr.expression) {
            indentLevel++;
            expr.expression->accept(*this);
            indentLevel--;
        }
    }

    void visit(BreakStmt& stmt) override {
        printNode("BreakStmt");
    }

    void visit(ArrayAccessExpr& expr) override {
        printNode("ArrayAccessExpr");
        indentLevel++;
        expr.array->accept(*this);
        printNode("Index");
        expr.index->accept(*this);
        indentLevel--;
    }
    void visit(MemberAccessExpr& expr) override {
        printNode("MemberAccess", expr.member + (expr.isPointerAccess ? " (->)" : " (.)"));
        indentLevel++;
        expr.object->accept(*this);
        indentLevel--;
    }

    // Statements
    void visit(AssignStmt& stmt) override {
        printNode("AssignStmt", stmt.target + " " + stmt.op);
        indentLevel++;
        stmt.value->accept(*this);
        indentLevel--;
    }

    void visit(ExprStmt& stmt) override {
        printNode("ExprStmt");
        indentLevel++;
        stmt.expression->accept(*this);
        indentLevel--;
    }

    void visit(VarDeclStmt& stmt) override {
        printNode("VarDeclStmt", stmt.typeName + " " + stmt.name);
        if (stmt.initializer) {
            indentLevel++;
            printNode("Initializer");
            stmt.initializer->accept(*this);
            indentLevel--;
        }
    }

    void visit(IfStmt& stmt) override {
        printNode("IfStmt");
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

    void visit(WhileStmt& stmt) override {
        printNode("WhileStmt");
        indentLevel++;
        printNode("Condition");
        stmt.condition->accept(*this);
        printNode("Body");
        stmt.body->accept(*this);
        indentLevel--;
    }

    void visit(ForStmt& stmt) override {
        printNode("ForStmt");
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

    void visit(ReturnStmt& stmt) override {
        printNode("ReturnStmt");
        if (stmt.value) {
            indentLevel++;
            stmt.value->accept(*this);
            indentLevel--;
        }
    }

    void visit(WriteStmt& stmt) override {
        printNode(stmt.newline ? "WritelnStmt" : "WriteStmt");
        indentLevel++;
        for (auto& arg : stmt.args) {
            arg->accept(*this);
        }
        indentLevel--;
    }

    void visit(Block& block) override {
        printNode("Block");
        indentLevel++;
        for (auto& stmt : block.statements) {
            stmt->accept(*this);
        }
        indentLevel--;
    }

    void visit(FunctionDecl& func) override {
        printNode("FunctionDecl", func.returnType + " " + func.name);
        indentLevel++;
        printNode("Parameters");
        indentLevel++;
        for (auto& param : func.params) {
            printNode("Param", param.typeName + " " + param.name);
        }
        indentLevel--;
        if (func.body) {
            printNode("Body");
            func.body->accept(*this);
        }
        indentLevel--;
    }

    void visit(Program& program) override {
        printNode("Program", program.name);
        indentLevel++;
        if (program.body) {
            program.body->accept(*this);
        }
        indentLevel--;
    }

    // Новые statements
    void visit(PreprocessorDirective& stmt) override {
        printNode("PreprocessorDirective", stmt.directive + "->" + stmt.value);
    }

    void visit(StructDecl& stmt) override {
        printNode("StructDecl", stmt.name);
        indentLevel++;
        for (auto& field : stmt.fields) {
            field->accept(*this);
        }
        indentLevel--;
    }

    void visit(TypedefDecl& stmt) override {
        printNode("TypedefDecl", stmt.typeName + " -> " + stmt.alias);
    }

    void visit(SwitchStmt& stmt) override {
        printNode("SwitchStmt");
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

    void visit(CaseStmt& stmt) override {
        printNode("CaseStmt");
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

    void visit(DefaultStmt& stmt) override {
        printNode("DefaultStmt");
        indentLevel++;
        printNode("Body");
        indentLevel++;
        for (auto& bodyStmt : stmt.body) {
            bodyStmt->accept(*this);
        }
        indentLevel--;
        indentLevel--;
    }

    void visit(DoWhileStmt& stmt) override {
        printNode("DoWhileStmt");
        indentLevel++;
        printNode("Body");
        stmt.body->accept(*this);
        printNode("Condition");
        stmt.condition->accept(*this);
        indentLevel--;
    }
};

#endif // AST_VISUALIZER_H