//
// Created by пк on 19.11.2025.
//

#ifndef AISDLAB_CPPEXPR_H
#define AISDLAB_CPPEXPR_H

#include "CppAst.h"
#include "CppDecl.h"
#include <memory>
#include <vector>
#include <string>

// Базовые литералы
struct CppIdentifier : CppExpr {
    std::string name;

    void accept(CppVisitor &v) override;
};

struct CppStringLiteral : CppExpr {
    std::string value;

    void accept(CppVisitor &v) override;
};

struct CppIntLiteral : CppExpr {
    long long value{0};

    void accept(CppVisitor &v) override;
};

struct CppRealLiteral : CppExpr {
    double value{0.0};

    void accept(CppVisitor &v) override;
};

struct CppBoolLiteral : CppExpr {
    bool value{false};

    void accept(CppVisitor &v) override;
};

struct CppNullptrLiteral : CppExpr {
    void accept(CppVisitor &v) override;
};

// Операторы
struct CppUnaryOp : CppExpr {
    std::string op;
    bool postfix{false};
    CppExprPtr operand;

    void accept(CppVisitor &v) override;
};

enum class CppBinOpKind {
    Add, Sub, Mul, Div, Mod,
    And, Or,
    BitAnd, BitOr, BitXor,
    Shl, Shr,
    Lt, Gt, Le, Ge, Eq, Ne,
    Assign, AddAssign, SubAssign, MulAssign, DivAssign, ModAssign,
    AndAssign, OrAssign, XorAssign, ShlAssign, ShrAssign
};

struct CppBinaryOp : CppExpr {
    CppBinOpKind op{CppBinOpKind::Add};
    CppExprPtr left;
    CppExprPtr right;

    void accept(CppVisitor &v) override;
};

struct CppTernaryOp : CppExpr {
    CppExprPtr condition;
    CppExprPtr thenExpr;
    CppExprPtr elseExpr;

    void accept(CppVisitor &v) override;
};

// Вызовы и доступы
struct CppCallExpr : CppExpr {
    CppExprPtr callee;
    CppExprList arguments;

    void accept(CppVisitor &v) override;
};

struct CppArrayAccessExpr : CppExpr {
    CppExprPtr array;
    CppExprPtr index;

    void accept(CppVisitor &v) override;
};

struct CppMemberAccessExpr : CppExpr {
    CppExprPtr object;
    std::string member;
    bool isPointerAccess{false}; // -> или .
    void accept(CppVisitor &v) override;
};

struct CppPointerToMemberExpr : CppExpr {
    std::unique_ptr<CppExpr> object;
    std::string member;
    bool isArrow{false}; // ->* или .*
    void accept(CppVisitor &v) override;
};

// Специфичные для C++ выражения
struct CppNewExpr : CppExpr {
    std::string typeName;
    CppExprList arguments;
    CppExprPtr arraySize; // для new[]
    bool isArray{false};

    void accept(CppVisitor &v) override;
};

struct CppDeleteExpr : CppExpr {
    CppExprPtr operand;
    bool isArray{false}; // delete[]
    void accept(CppVisitor &v) override;
};

struct CppThisExpr : CppExpr {
    void accept(CppVisitor &v) override;
};

struct CppSizeofExpr : CppExpr {
    CppExprPtr expression;
    bool isType{false};

    void accept(CppVisitor &v) override;
};

struct CppTypeidExpr : CppExpr {
    CppExprPtr expression;
    bool isType{false};

    void accept(CppVisitor &v) override;
};

// Шаблонные выражения
struct CppTemplateIdExpr : CppExpr {
    std::string templateName;
    CppExprList templateArgs;

    void accept(CppVisitor &v) override;
};

// Лямбда-выражения
struct CppLambdaExpr : CppExpr {
    struct Capture {
        std::string name;
        bool byReference{false};
        bool isThis{false}; // [this]
        CppExprPtr init; // инициализатор захвата [x = expr]
    };

    std::vector<Capture> captures;
    std::vector<std::unique_ptr<CppParameter>> parameters;
    CppExprPtr returnType; // может быть auto
    std::unique_ptr<CppCompoundStmt> body;
    bool isMutable{false};
    std::string exceptionSpec; // noexcept
    void accept(CppVisitor &v) override;
};

// Cast выражения
struct CppCastExpr : CppExpr {
    std::string castType; // static_cast, dynamic_cast, etc.
    std::string targetType;
    CppExprPtr expression;

    void accept(CppVisitor &v) override;
};

#endif //AISDLAB_CPPEXPR_H
