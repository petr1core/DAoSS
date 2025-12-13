//
// Created by пк on 19.11.2025.
//

#ifndef AISDLAB_CPPSTMT_H
#define AISDLAB_CPPSTMT_H

#include "CppExpr.h"
#include "CppAst.h"
#include <memory>
#include <vector>

// Базовые операторы
struct CppExprStmt : CppStmt {
    CppExprPtr expression;
    void accept(CppVisitor &v) override;
};

struct CppCompoundStmt : CppStmt {
    CppStmtList statements;
    void accept(CppVisitor &v) override;
};

struct CppReturnStmt : CppStmt {
    CppExprPtr value;
    void accept(CppVisitor &v) override;
};

struct CppBreakStmt : CppStmt {
    void accept(CppVisitor &v) override;
};

struct CppContinueStmt : CppStmt {
    void accept(CppVisitor &v) override;
};

// Управление потоком
struct CppIfStmt : CppStmt {
    CppExprPtr condition;
    CppStmtPtr thenBranch;
    CppStmtPtr elseBranch;
    void accept(CppVisitor &v) override;
};

struct CppWhileStmt : CppStmt {
    CppExprPtr condition;
    CppStmtPtr body;
    void accept(CppVisitor &v) override;
};

struct CppDoWhileStmt : CppStmt {
    CppStmtPtr body;
    CppExprPtr condition;
    void accept(CppVisitor &v) override;
};

struct CppForStmt : CppStmt {
    CppStmtPtr init;
    CppExprPtr condition;
    CppExprPtr increment;
    CppStmtPtr body;
    void accept(CppVisitor &v) override;
};

struct CppRangeForStmt : CppStmt {
    CppStmtPtr init; // может быть объявление переменной
    CppExprPtr range;
    CppStmtPtr body;
    void accept(CppVisitor &v) override;
};

struct CppCaseStmt : CppStmt {
    CppExprPtr value;
    CppStmtList body;
    void accept(CppVisitor &v) override;
};

struct CppSwitchStmt : CppStmt {
    CppExprPtr condition;
    std::vector<std::unique_ptr<CppCaseStmt>> cases;
    void accept(CppVisitor &v) override;
};



struct CppDefaultStmt : CppStmt {
    CppStmtList body;
    void accept(CppVisitor &v) override;
};

// Обработка исключений
struct CppCatchStmt : CppStmt {
    std::string exceptionType;
    std::string exceptionName;
    std::unique_ptr<CppCompoundStmt> body;
    void accept(CppVisitor &v) override;
};

struct CppTryStmt : CppStmt {
    std::unique_ptr<CppCompoundStmt> tryBlock;
    std::vector<std::unique_ptr<CppCatchStmt>> handlers;
    void accept(CppVisitor &v) override;
};



// Объявления
struct CppVarDecl : CppStmt {
    std::string typeName;
    std::string name;
    CppExprPtr initializer;
    CppStorageClass storage{CppStorageClass::Auto};
    CppCVQualifier cvQualifier{CppCVQualifier::None};
    void accept(CppVisitor &v) override;
};

struct CppAssignStmt : CppStmt {
    CppExprPtr target;
    std::string op; // =, +=, -= и т.д.
    CppExprPtr value;
    void accept(CppVisitor &v) override;
};
#endif //AISDLAB_CPPSTMT_H
