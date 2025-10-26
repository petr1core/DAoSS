// Expressions hierarchy

#ifndef AST_EXPR_H
#define AST_EXPR_H

#include "Ast.h"

struct Expr : AstNode {};

struct Identifier : Expr {
    std::string name;
    void accept(Visitor &v) override;
};

struct StringLiteral : Expr {
    std::string value;
    void accept(Visitor &v) override;
};

struct IntLiteral : Expr {
    long long value{0};
    void accept(Visitor &v) override;
};

struct RealLiteral : Expr {
    double value{0};
    void accept(Visitor &v) override;
};

enum class BinOpKind { Add, Sub, Mul, Div, Mod, And, Or, Xor, Lt, Gt, Le, Ge, Eq, Ne };

struct BinaryOp : Expr {
    BinOpKind op{BinOpKind::Add};
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;
    void accept(Visitor &v) override;
};

#endif // AST_EXPR_H




