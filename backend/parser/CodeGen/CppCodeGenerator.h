#ifndef CODEGEN_CPP_CODE_GENERATOR_H
#define CODEGEN_CPP_CODE_GENERATOR_H

#include <string>
#include <sstream>
#include "../Ast/Visitor.h"
#include "../Ast/Expr.h"
#include "../Ast/Stmt.h"
#include "../Ast/ExprAcceptImpl.h"

class CppCodeGenerator : public Visitor {
public:
    std::string generate(Program &program) {
        out.str("");
        out.clear();
        //emit("#include <bits/stdc++.h>\nusing namespace std;\n\n");
        program.accept(*this);
        return out.str();
    }

    void visit(Identifier &n) override { emit(n.name); }

    void visit(StringLiteral &n) override { emit('"' + n.value + '"'); }

    void visit(IntLiteral &n) override { emit(std::to_string(n.value)); }

    void visit(RealLiteral &n) override { emit(std::to_string(n.value)); }

    void visit(BinaryOp &n) override {
        emit("(");
        n.left->accept(*this);
        emit(" ");
        emit(opToStr(n.op));
        emit(" ");
        n.right->accept(*this);
        emit(")");
    }

    void visit(AssignStmt &s) override {
        indent();
        emit(s.target + " = ");
        s.value->accept(*this);
        emit(";\n");
    }

    void visit(WriteStmt &s) override {
        indent();
        emit("cout");
        for (size_t i = 0; i < s.args.size(); ++i) {
            emit(" << ");
            s.args[i]->accept(*this);
        }
        if (s.newline)
            emit(" << \\\"\\n\\\"");
        emit(";\n");
    }

    void visit(Block &b) override {
        emit("int main(){\n");
        ++tab;
        for (auto &st: b.statements)
            st->accept(*this);
        indent();
        emit("return 0;\n");
        --tab;
        emit("}\n");
    }

    void visit(Program &p) override {
        if (p.body)
            p.body->accept(*this);
    }

private:
    std::ostringstream out;
    int tab{0};

    void emit(const std::string &s) { out << s; }

    void indent() {
        for (int i = 0; i < tab; i++)
            out << "    ";
    }

    static std::string opToStr(BinOpKind k) {
        switch (k) {
            case BinOpKind::Add:
                return "+";
            case BinOpKind::Sub:
                return "-";
            case BinOpKind::Mul:
                return "*";
            case BinOpKind::Div:
                return "/";
            case BinOpKind::Mod:
                return "%";
            case BinOpKind::And:
                return "&&";
            case BinOpKind::Or:
                return "||";
            case BinOpKind::Xor:
                return "^";
            case BinOpKind::Lt:
                return "<";
            case BinOpKind::Gt:
                return ">";
            case BinOpKind::Le:
                return "<=";
            case BinOpKind::Ge:
                return ">=";
            case BinOpKind::Eq:
                return "==";
            case BinOpKind::Ne:
                return "!=";
        }
        return "?";
    }
};

#endif // CODEGEN_CPP_CODE_GENERATOR_H




