// Общие утилиты для парсеров
import type { FlowchartNode, Connection, NodeExitPoint } from '../types/flowchart';
import type { PortType } from '../types/flowchart';

export interface NodeCreator {
    (type: 'start' | 'end' | 'process' | 'decision' | 'input' | 'output', 
     text: string, 
     codeRef?: string, 
     width?: number | null, 
     height?: number | null): FlowchartNode;
}

export interface ConnectionCreator {
    (fromNode: FlowchartNode, 
     toNode: FlowchartNode, 
     fromPort?: PortType, 
     toPort?: PortType, 
     label?: string): void;
}

/**
 * Создает фабрики для создания узлов и соединений
 */
export function createParserFactories(
    nodes: FlowchartNode[],
    connections: Connection[],
    getNextNodeId: () => number,
    getNextYPosition: () => number,
    updateYPosition: (delta: number) => void
) {
    function createNode(
        type: 'start' | 'end' | 'process' | 'decision' | 'input' | 'output',
        text: string,
        codeRef = '',
        width: number | null = null,
        height: number | null = null
    ): FlowchartNode {
        const nodeId = getNextNodeId();
        const node: FlowchartNode = {
            id: `node-${nodeId}`,
            type: type,
            x: 400,
            y: getNextYPosition(),
            width: width || (type === 'decision' ? 180 : type === 'start' || type === 'end' ? 120 : 180),
            height: height || (type === 'decision' ? 100 : type === 'start' || type === 'end' ? 60 : 80),
            text: text,
            codeReference: codeRef,
            comments: []
        };
        nodes.push(node);
        
        const deltaY = type === 'decision' ? 150 : type === 'start' || type === 'end' ? 120 : 130;
        updateYPosition(deltaY);
        
        console.log(`[DEBUG] Created node: node-${nodeId}, type: ${type}, text: ${text.substring(0, 50)}`);
        return node;
    }

    function createConnection(
        fromNode: FlowchartNode,
        toNode: FlowchartNode,
        fromPort: PortType = 'bottom',
        toPort: PortType = 'top',
        label = ''
    ): void {
        connections.push({
            id: `conn-${Date.now()}-${connections.length}`,
            from: fromNode.id,
            to: toNode.id,
            fromPort: fromPort,
            toPort: toPort,
            label: label
        });
    }

    return { createNode, createConnection };
}

/**
 * Нормализует exit node в объект с node
 */
export function normalizeExitNode(
    exit: FlowchartNode | NodeExitPoint
): NodeExitPoint {
    if (typeof exit === 'object' && 'node' in exit) {
        return exit;
    }
    return { node: exit as FlowchartNode };
}

/**
 * Проверяет, является ли IO операция вводом или выводом
 */
export function getIOType(ioText: string): 'input' | 'output' | 'process' {
    const isOutput = ioText && (
        ioText.includes('Writeln') || 
        ioText.includes('Write') ||
        ioText.includes('cout') ||
        ioText.includes('printf')
    );
    
    const isInput = ioText && (
        ioText.includes('Readln') || 
        ioText.includes('Read') ||
        ioText.includes('scanf') ||
        ioText.includes('cin')
    );
    
    return isOutput ? 'output' : (isInput ? 'input' : 'process');
}

/**
 * Обрезает текст до максимальной длины с добавлением "..."
 */
export function truncateText(text: string, maxLength: number): string {
    return text.length > maxLength ? text.substring(0, maxLength) + '...' : text;
}

/**
 * Сортирует ключи выражений Pascal (expr0, expr1, ...)
 */
export function sortExpressionKeys(entries: [string, unknown][]): [string, unknown][] {
    return entries
        .filter(([key]) => key.startsWith('expr'))
        .sort(([key1], [key2]) => {
            const num1 = parseInt(key1.replace('expr', '')) || 0;
            const num2 = parseInt(key2.replace('expr', '')) || 0;
            return num1 - num2;
        });
}

