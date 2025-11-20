#ifndef AST_VISITOR_H
#define AST_VISITOR_H

// Предварительные объявления всех узлов
struct Identifier; struct StringLiteral; struct IntLiteral; struct RealLiteral;
struct UnaryOp; struct BinaryOp; struct CallExpr; struct TernaryOp; struct SizeofExpr; struct MemberAccessExpr;
struct AssignStmt; struct ExprStmt; struct VarDeclStmt; struct IfStmt; struct WhileStmt;
struct ForStmt; struct ReturnStmt; struct WriteStmt; struct Block; struct FunctionDecl; struct Program;
struct PreprocessorDirective; struct StructDecl; struct TypedefDecl;
struct SwitchStmt; struct CaseStmt; struct DefaultStmt; struct DoWhileStmt; struct ArrayAccessExpr; struct BreakStmt;

struct Visitor {
    virtual ~Visitor() = default;

    // Expressions
    virtual void visit(Identifier&) = 0;
    virtual void visit(StringLiteral&) = 0;
    virtual void visit(IntLiteral&) = 0;
    virtual void visit(RealLiteral&) = 0;
    virtual void visit(UnaryOp&) = 0;
    virtual void visit(BinaryOp&) = 0;
    virtual void visit(CallExpr&) = 0;
    virtual void visit(TernaryOp&) = 0;
    virtual void visit(SizeofExpr&) = 0;
    virtual void visit(MemberAccessExpr&) = 0;

    // Statements
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

    // New statements
    virtual void visit(PreprocessorDirective&) = 0;
    virtual void visit(StructDecl&) = 0;
    virtual void visit(TypedefDecl&) = 0;
    virtual void visit(SwitchStmt&) = 0;
    virtual void visit(CaseStmt&) = 0;
    virtual void visit(DefaultStmt&) = 0;
    virtual void visit(DoWhileStmt&) = 0;
    virtual void visit(ArrayAccessExpr& expr) =0;
    virtual void visit(BreakStmt& stmt) =0;

    };

#endif // AST_VISITOR_H