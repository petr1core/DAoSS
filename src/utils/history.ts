// Утилиты для работы с историей изменений
import type { FlowchartNode, Connection, HistoryEntry } from '../types/flowchart';

/**
 * Создает запись истории изменений
 */
export function createHistoryEntry(
    description: string,
    nodes: FlowchartNode[],
    connections: Connection[]
): HistoryEntry {
    return {
        id: `h${Date.now()}`,
        timestamp: new Date(),
        description,
        // Глубокое копирование для истории
        nodes: JSON.parse(JSON.stringify(nodes)),
        connections: JSON.parse(JSON.stringify(connections))
    };
}

/**
 * Добавляет запись в историю
 */
export function addToHistory(
    description: string,
    nodes: FlowchartNode[],
    connections: Connection[],
    history: HistoryEntry[]
): HistoryEntry[] {
    const entry = createHistoryEntry(description, nodes, connections);
    return [...history, entry];
}

/**
 * Восстанавливает состояние из записи истории
 */
export function restoreHistoryEntry(
    entry: HistoryEntry
): { nodes: FlowchartNode[]; connections: Connection[] } {
    // Глубокое копирование для восстановления
    return {
        nodes: JSON.parse(JSON.stringify(entry.nodes)),
        connections: JSON.parse(JSON.stringify(entry.connections))
    };
}

