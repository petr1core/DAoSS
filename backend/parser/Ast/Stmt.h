// Statements hierarchy

#ifndef AST_STMT_H
#define AST_STMT_H

#include "Ast.h"
#include "Expr.h"

struct Stmt : AstNode {};

struct AssignStmt : Stmt {
    std::string target;
    std::unique_ptr<Expr> value;
    void accept(Visitor &v) override;
};

struct WriteStmt : Stmt { // Write / Writeln merged; flag for newline
    std::vector<std::unique_ptr<Expr>> args;
    bool newline{false};
    void accept(Visitor &v) override;
};

struct Block : Stmt {
    std::vector<std::unique_ptr<Stmt>> statements;
    void accept(Visitor &v) override;
};

struct Program : AstNode {
    std::string name;
    std::unique_ptr<Block> body;
    void accept(Visitor &v) override;
};

#endif // AST_STMT_H




