#ifndef AST_VISITOR_H
#define AST_VISITOR_H

struct Identifier; struct StringLiteral; struct IntLiteral; struct RealLiteral; struct BinaryOp;
struct AssignStmt; struct WriteStmt; struct Block; struct Program;

struct Visitor {
    virtual ~Visitor() = default;
    virtual void visit(Identifier&) = 0;
    virtual void visit(StringLiteral&) = 0;
    virtual void visit(IntLiteral&) = 0;
    virtual void visit(RealLiteral&) = 0;
    virtual void visit(BinaryOp&) = 0;
    virtual void visit(AssignStmt&) = 0;
    virtual void visit(WriteStmt&) = 0;
    virtual void visit(Block&) = 0;
    virtual void visit(Program&) = 0;
};

#endif // AST_VISITOR_H




