#ifndef AST_STMT_H
#define AST_STMT_H

#include "Ast.h"
#include "Expr.h"

struct Stmt : AstNode {};

// Добавляем новые узлы
struct PreprocessorDirective : Stmt {
    std::string directive;
    std::string value;
    void accept(Visitor &v) override;
};
struct BreakStmt : Stmt {
    void accept(Visitor &v) override;
};
struct StructDecl : Stmt {
    std::string name;
    std::vector<std::unique_ptr<VarDeclStmt>> fields;
    void accept(Visitor &v) override;
};

struct TypedefDecl : Stmt {
    std::string typeName;
    std::string alias;
    void accept(Visitor &v) override;
};

struct SwitchStmt : Stmt {
    std::unique_ptr<Expr> condition;
    std::vector<std::unique_ptr<Stmt>> cases; // CaseStmt или DefaultStmt
    void accept(Visitor &v) override;
};

struct CaseStmt : Stmt {
    std::unique_ptr<Expr> value;
    std::vector<std::unique_ptr<Stmt>> body;
    void accept(Visitor &v) override;
};

struct DefaultStmt : Stmt {
    std::vector<std::unique_ptr<Stmt>> body;
    void accept(Visitor &v) override;
};

struct DoWhileStmt : Stmt {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;
    void accept(Visitor &v) override;
};

// Обновляем AssignStmt для поддержки составных операторов
struct AssignStmt : Stmt {
    std::string target;
    std::string op; // "=", "+=", "-=", etc.
    std::unique_ptr<Expr> value;
    void accept(Visitor &v) override;
};

// Остальные существующие узлы...
struct ExprStmt : Stmt {
    std::unique_ptr<Expr> expression;
    void accept(Visitor &v) override;
};

struct VarDeclStmt : Stmt {
    std::string typeName;
    std::string name;
    std::unique_ptr<Expr> initializer;
    void accept(Visitor &v) override;
};

struct IfStmt : Stmt {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> thenBranch;
    std::unique_ptr<Stmt> elseBranch;
    void accept(Visitor &v) override;
};

struct WhileStmt : Stmt {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;
    void accept(Visitor &v) override;
};

struct ForStmt : Stmt {
    std::unique_ptr<Stmt> init;
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Expr> increment;
    std::unique_ptr<Stmt> body;
    void accept(Visitor &v) override;
};

struct ReturnStmt : Stmt {
    std::unique_ptr<Expr> value;
    void accept(Visitor &v) override;
};

struct WriteStmt : Stmt {
    std::vector<std::unique_ptr<Expr>> args;
    bool newline{false};
    void accept(Visitor &v) override;
};

struct Block : Stmt {
    std::vector<std::unique_ptr<Stmt>> statements;
    void accept(Visitor &v) override;
};

struct FunctionDecl : Stmt {
    struct Parameter {
        std::string typeName;
        std::string name;
    };
    std::string returnType;
    std::string name;
    std::vector<Parameter> params;
    std::unique_ptr<Block> body;
    void accept(Visitor &v) override;
};

struct Program : AstNode {
    std::string name;
    std::unique_ptr<Block> body;
    void accept(Visitor &v) override;
};

#endif // AST_STMT_H