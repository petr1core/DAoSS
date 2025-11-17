#ifndef AST_VISITOR_H
#define AST_VISITOR_H

struct Identifier; struct StringLiteral; struct IntLiteral; struct RealLiteral; struct UnaryOp;
struct BinaryOp; struct CallExpr;
struct AssignStmt; struct ExprStmt; struct VarDeclStmt; struct IfStmt; struct WhileStmt;
struct ForStmt; struct ReturnStmt; struct WriteStmt; struct Block; struct FunctionDecl; struct Program;

struct Visitor {
    virtual ~Visitor() = default;
    virtual void visit(Identifier&) = 0;
    virtual void visit(StringLiteral&) = 0;
    virtual void visit(IntLiteral&) = 0;
    virtual void visit(RealLiteral&) = 0;
    virtual void visit(UnaryOp&) = 0;
    virtual void visit(BinaryOp&) = 0;
    virtual void visit(CallExpr&) = 0;
    virtual void visit(AssignStmt&) = 0;
    virtual void visit(ExprStmt&) = 0;
    virtual void visit(VarDeclStmt&) = 0;
    virtual void visit(IfStmt&) = 0;
    virtual void visit(WhileStmt&) = 0;
    virtual void visit(ForStmt&) = 0;
    virtual void visit(ReturnStmt&) = 0;
    virtual void visit(WriteStmt&) = 0;
    virtual void visit(Block&) = 0;
    virtual void visit(FunctionDecl&) = 0;
    virtual void visit(Program&) = 0;
};

#endif // AST_VISITOR_H
