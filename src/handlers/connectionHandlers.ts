// Обработчики для работы с соединениями
import type { FlowchartNode, Connection, PortType } from '../types/flowchart';
import { getPortPosition } from '../utils/geometry';
import { showToast } from '../utils/toast';

/**
 * Определяет лучший порт для соединения на основе позиции узлов
 */
export function determineBestPort(
    fromNode: FlowchartNode,
    fromPort: PortType,
    toNode: FlowchartNode
): PortType {
    const fromPortPos = getPortPosition(fromNode, fromPort);
    const toNodeCenter = {
        x: toNode.x + toNode.width / 2,
        y: toNode.y + toNode.height / 2
    };
    
    const dx = toNodeCenter.x - fromPortPos.x;
    const dy = toNodeCenter.y - fromPortPos.y;
    
    if (Math.abs(dy) > Math.abs(dx)) {
        return dy > 0 ? 'top' : 'bottom';
    } else {
        return dx > 0 ? 'left' : 'right';
    }
}

/**
 * Создает соединение между узлами
 */
export function createConnection(
    fromNodeId: string,
    fromPort: PortType,
    toNodeId: string,
    toPort: PortType | null,
    nodes: FlowchartNode[],
    connections: Connection[]
): Connection | null {
    // Нельзя соединять узел сам с собой
    if (fromNodeId === toNodeId) {
        return null;
    }
    
    // Проверяем, не существует ли уже такое соединение
    const existing = connections.find(
        c => c.from === fromNodeId && 
             c.to === toNodeId && 
             c.fromPort === fromPort &&
             c.toPort === toPort
    );
    
    if (existing) {
        showToast('Такое соединение уже существует', 'warning');
        return null;
    }
    
    // Если порт назначения не указан, определяем лучший
    if (!toPort) {
        const fromNode = nodes.find(n => n.id === fromNodeId);
        const toNode = nodes.find(n => n.id === toNodeId);
        
        if (fromNode && toNode) {
            toPort = determineBestPort(fromNode, fromPort, toNode);
        } else {
            toPort = 'top';
        }
    }
    
    return {
        id: `conn-${Date.now()}-${connections.length}`,
        from: fromNodeId,
        to: toNodeId,
        fromPort,
        toPort
    };
}


