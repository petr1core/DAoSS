// Конвертация AST выражений в строки (для C/C++)
import type { ASTExpression } from '../types/parser';

/**
 * Преобразует AST выражение в строковое представление
 */
export function convertExprToString(expr: ASTExpression | null | undefined): string {
    if (!expr || typeof expr !== 'object') return '';
    
    switch (expr.type) {
        case 'Identifier':
            return expr.name || '';
            
        case 'IntLiteral':
        case 'RealLiteral':
            return String(expr.value ?? '');
            
        case 'StringLiteral':
            return `"${expr.value ?? ''}"`;
            
        case 'BinaryOp':
            const left = convertExprToString(expr.left);
            const right = convertExprToString(expr.right);
            const binOp = expr.operator || '';
            return `${left} ${binOp} ${right}`;
            
        case 'UnaryOp':
            const operand = convertExprToString(expr.operand);
            const unaryOp = expr.operator || '';
            const isPostfix = expr.postfix || false;
            return isPostfix ? `${operand}${unaryOp}` : `${unaryOp}${operand}`;
            
        case 'CallExpr':
            const callee = expr.callee || '';
            const args = (expr.arguments || []).map(arg => convertExprToString(arg)).join(', ');
            return `${callee}(${args})`;
            
        case 'ArrayAccessExpr':
            const array = convertExprToString(expr.array);
            const index = convertExprToString(expr.index);
            return `${array}[${index}]`;
            
        case 'MemberAccessExpr':
            const object = convertExprToString(expr.object);
            const member = expr.member || '';
            const accessor = expr.isPointerAccess ? '->' : '.';
            return `${object}${accessor}${member}`;
            
        case 'TernaryOp':
            const cond = convertExprToString(expr.condition);
            const thenExpr = convertExprToString(expr.thenExpr);
            const elseExpr = convertExprToString(expr.elseExpr);
            return `${cond} ? ${thenExpr} : ${elseExpr}`;
            
        default:
            return '';
    }
}

