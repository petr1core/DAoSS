// Типы для парсеров (Pascal SPR и C/C++ AST)

// ===== Pascal SPR форматы =====
export interface PascalProgram {
    program: {
        name: string;
        sections: PascalSections;
    };
}

export interface PascalSections {
    functionBlock?: Record<string, PascalFunction | PascalProcedure>;
    constantBlock?: Record<string, string | PascalAssignment>;
    variableBlock?: Record<string, string | PascalAssignment>;
    mainBlock?: Record<string, PascalExpression>;
}

export interface PascalFunction {
    type: 'function';
    declaration: string;
    body?: Record<string, PascalExpression>;
}

export interface PascalProcedure {
    type: 'procedure';
    declaration: string;
    body?: Record<string, PascalExpression>;
}

export interface PascalExpression {
    type: 'io' | 'assign' | 'if' | 'If' | 'else' | 'Else' | 'while' | 'While' | 'for' | 'For' | 'repeat' | 'Repeat' | 'caseOf' | 'case';
    value?: string;
    condition?: string;
    body?: Record<string, PascalExpression> | PascalExpression[];
    compareValue?: string;
    branches?: Record<string, PascalCaseBranch>;
}

export interface PascalAssignment {
    type: 'assign';
    value: string;
}

export interface PascalCaseBranch {
    conditionValues: string;
    todo: Record<string, PascalExpression>;
}

// ===== C/C++ AST форматы =====
export interface ASTProgram {
    type: 'Program';
    name: string;
    body: ASTBlock;
}

export interface ASTBlock {
    type: 'Block';
    statements: ASTStatement[];
}

export type ASTStatement = 
    | ASTVarDeclStmt
    | ASTAssignStmt
    | ASTIfStmt
    | ASTWhileStmt
    | ASTForStmt
    | ASTDoWhileStmt
    | ASTSwitchStmt
    | ASTFunctionDecl
    | ASTExprStmt
    | ASTReturnStmt
    | ASTBreakStmt
    | ASTStructDecl
    | ASTTypedefDecl
    | ASTPreprocessorDirective;

export interface ASTVarDeclStmt {
    type: 'VarDeclStmt';
    varType: string;
    name: string;
    initializer?: ASTExpression;
}

export interface ASTAssignStmt {
    type: 'AssignStmt';
    target: string;
    operator: string;
    value: ASTExpression;
}

export interface ASTIfStmt {
    type: 'IfStmt';
    condition: ASTExpression;
    thenBranch: ASTStatement | ASTBlock;
    elseBranch?: ASTStatement | ASTBlock;
}

export interface ASTWhileStmt {
    type: 'WhileStmt';
    condition: ASTExpression;
    body: ASTStatement | ASTBlock;
}

export interface ASTForStmt {
    type: 'ForStmt';
    init?: ASTStatement;
    condition?: ASTExpression;
    increment?: ASTExpression;
    body?: ASTStatement | ASTBlock;
}

export interface ASTDoWhileStmt {
    type: 'DoWhileStmt';
    body: ASTStatement | ASTBlock;
    condition: ASTExpression;
}

export interface ASTSwitchStmt {
    type: 'SwitchStmt';
    condition: ASTExpression;
    cases: (ASTCaseStmt | ASTDefaultStmt)[];
}

export interface ASTCaseStmt {
    type: 'CaseStmt';
    value?: ASTExpression;
    body: ASTStatement[];
}

export interface ASTDefaultStmt {
    type: 'DefaultStmt';
    body: ASTStatement[];
}

export interface ASTFunctionDecl {
    type: 'FunctionDecl';
    returnType: string;
    name: string;
    parameters: ASTParameter[];
    body?: ASTBlock;
}

export interface ASTParameter {
    type: string;
    name: string;
}

export interface ASTExprStmt {
    type: 'ExprStmt';
    expression: ASTExpression;
}

export interface ASTReturnStmt {
    type: 'ReturnStmt';
    value?: ASTExpression;
}

export interface ASTBreakStmt {
    type: 'BreakStmt';
}

export interface ASTStructDecl {
    type: 'StructDecl';
    name: string;
    fields: ASTVarDeclStmt[];
}

export interface ASTTypedefDecl {
    type: 'TypedefDecl';
    typeName: string;
    alias: string;
}

export interface ASTPreprocessorDirective {
    type: 'PreprocessorDirective';
    directive: string;
    value: string;
}

// ===== AST Expressions =====
export type ASTExpression =
    | ASTIdentifier
    | ASTIntLiteral
    | ASTRealLiteral
    | ASTStringLiteral
    | ASTBinaryOp
    | ASTUnaryOp
    | ASTCallExpr
    | ASTArrayAccessExpr
    | ASTMemberAccessExpr
    | ASTTernaryOp;

export interface ASTIdentifier {
    type: 'Identifier';
    name: string;
}

export interface ASTIntLiteral {
    type: 'IntLiteral';
    value: number;
}

export interface ASTRealLiteral {
    type: 'RealLiteral';
    value: number;
}

export interface ASTStringLiteral {
    type: 'StringLiteral';
    value: string;
}

export interface ASTBinaryOp {
    type: 'BinaryOp';
    operator: string;
    left: ASTExpression;
    right: ASTExpression;
}

export interface ASTUnaryOp {
    type: 'UnaryOp';
    operator: string;
    postfix: boolean;
    operand: ASTExpression;
}

export interface ASTCallExpr {
    type: 'CallExpr';
    callee: string;
    arguments: ASTExpression[];
}

export interface ASTArrayAccessExpr {
    type: 'ArrayAccessExpr';
    array: ASTExpression;
    index: ASTExpression;
}

export interface ASTMemberAccessExpr {
    type: 'MemberAccessExpr';
    object: ASTExpression;
    member: string;
    isPointerAccess: boolean;
}

export interface ASTTernaryOp {
    type: 'TernaryOp';
    condition: ASTExpression;
    thenExpr: ASTExpression;
    elseExpr: ASTExpression;
}

// ===== Общие типы для парсера =====
export type ParserLanguage = 'pascal' | 'c' | 'cpp';

export interface ParserResponse {
    success: boolean;
    representation?: PascalProgram | ASTProgram;
    representationType?: 'SPR' | 'AST';
    error?: string | null;
    parserErrors?: ParserError[];
    lexerErrors?: ParserError[];
}

export interface ParserError {
    line?: number;
    column?: number;
    message: string;
}


