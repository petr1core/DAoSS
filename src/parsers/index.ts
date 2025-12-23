// Главный модуль парсеров
import type { FlowchartNode, Connection } from '../types/flowchart';
import type { ParserLanguage, PascalProgram, ASTProgram } from '../types/parser';
import { parsePascalToFlowchart } from './pascalParser';
import { parseCToFlowchart } from './cParser';

/**
 * Парсит JSON данные в блок-схему в зависимости от языка
 */
export function parseJsonToFlowchart(
    jsonData: PascalProgram | ASTProgram,
    language: ParserLanguage
): { nodes: FlowchartNode[]; connections: Connection[] } {
    const nodes: FlowchartNode[] = [];
    const connections: Connection[] = [];
    
    if (language === 'pascal' && 'program' in jsonData) {
        return parsePascalToFlowchart(jsonData as PascalProgram, nodes, connections);
    } else if ((language === 'c' || language === 'cpp') && jsonData.type === 'Program') {
        return parseCToFlowchart(jsonData as ASTProgram, nodes, connections);
    }
    
    // Если формат не распознан, возвращаем пустую блок-схему
    return { nodes, connections };
}

export * from './pascalParser';
export * from './cParser';
export * from './expressionConverter';
export * from './parserUtils';
export * from './fallbackParser';

