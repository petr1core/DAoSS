// Простой fallback парсер для кода (используется при ошибках основного парсера)
import type { FlowchartNode } from '../types/flowchart';

/**
 * Простой парсер кода в блок-схему (fallback при ошибках основного парсера)
 */
export function parseCodeToFlowchart(code: string): FlowchartNode[] {
    const nodes: FlowchartNode[] = [];
    let yPosition = 50;
    
    // Начальный узел
    nodes.push({
        id: 'start',
        type: 'start',
        x: 400,
        y: yPosition,
        width: 120,
        height: 60,
        text: 'Начало',
        codeReference: '',
        comments: []
    });
    
    yPosition += 120;
    
    const lines = code.split('\n').filter(line => line.trim());
    
    lines.forEach((line, index) => {
        const trimmed = line.trim();
        
        if (trimmed.includes('if') || trimmed.includes('while') || trimmed.includes('for')) {
            nodes.push({
                id: `node-${index}`,
                type: 'decision',
                x: 370,
                y: yPosition,
                width: 180,
                height: 100,
                text: trimmed.substring(0, 30),
                codeReference: trimmed,
                comments: []
            });
            yPosition += 150;
        } else if (
            trimmed.includes('print') || 
            trimmed.includes('console.log') || 
            trimmed.includes('cout') || 
            trimmed.includes('Writeln') || 
            trimmed.includes('Write')
        ) {
            nodes.push({
                id: `node-${index}`,
                type: 'output',
                x: 370,
                y: yPosition,
                width: 180,
                height: 80,
                text: 'Вывод',
                codeReference: trimmed,
                comments: []
            });
            yPosition += 130;
        } else if (
            trimmed.includes('input') || 
            trimmed.includes('scanf') || 
            trimmed.includes('cin') ||
            trimmed.includes('Read')
        ) {
            nodes.push({
                id: `node-${index}`,
                type: 'input',
                x: 370,
                y: yPosition,
                width: 180,
                height: 80,
                text: 'Ввод',
                codeReference: trimmed,
                comments: []
            });
            yPosition += 130;
        } else if (trimmed && trimmed !== '{' && trimmed !== '}') {
            nodes.push({
                id: `node-${index}`,
                type: 'process',
                x: 370,
                y: yPosition,
                width: 180,
                height: 80,
                text: trimmed.substring(0, 30),
                codeReference: trimmed,
                comments: []
            });
            yPosition += 130;
        }
    });
    
    // Конечный узел
    nodes.push({
        id: 'end',
        type: 'end',
        x: 400,
        y: yPosition,
        width: 120,
        height: 60,
        text: 'Конец',
        codeReference: '',
        comments: []
    });
    
    return nodes;
}


