// State management для блок-схемы
import type { FlowchartState, FlowchartNode, Connection, HistoryEntry } from '../types/flowchart';
import type { ASTProgram, PascalProgram } from '../types/parser';
import { flowchartToJSON } from '../converters';

/**
 * Создает начальное состояние блок-схемы
 */
export function createInitialState(): FlowchartState {
    return {
        nodes: [
            {
                id: '1',
                type: 'start',
                x: 400,
                y: 50,
                width: 120,
                height: 60,
                text: 'Начало',
                codeReference: '',
                comments: []
            },
            {
                id: '2',
                type: 'process',
                x: 370,
                y: 150,
                width: 180,
                height: 80,
                text: 'Инициализация\nпеременных',
                codeReference: 'int x = 0;\nint y = 10;',
                comments: []
            },
            {
                id: '3',
                type: 'decision',
                x: 370,
                y: 270,
                width: 180,
                height: 100,
                text: 'x < y?',
                codeReference: 'if (x < y)',
                comments: []
            }
        ],
        connections: [
            { id: 'c1', from: '1', to: '2', fromPort: 'bottom', toPort: 'top' },
            { id: 'c2', from: '2', to: '3', fromPort: 'bottom', toPort: 'top' }
        ],
        selectedNodeId: null,
        selectedConnectionId: null,
        connectingFrom: null,
        connectingFromPort: null,
        history: [
            {
                id: 'h1',
                timestamp: new Date(Date.now() - 3600000),
                description: 'Создание начальной схемы',
                nodes: [],
                connections: []
            }
        ],
        historyIndex: 0, // Начальный индекс истории
        zoom: 1,
        sourceCode: '',
        draggedNode: null,
        dragOffset: { x: 0, y: 0 }
    };
}

/**
 * Класс для управления состоянием блок-схемы
 */
export class FlowchartStore {
    private state: FlowchartState;
    private listeners: Set<() => void> = new Set();

    constructor(initialState?: FlowchartState) {
        this.state = initialState || createInitialState();
    }

    /**
     * Подписывается на изменения состояния
     */
    subscribe(listener: () => void): () => void {
        this.listeners.add(listener);
        return () => {
            this.listeners.delete(listener);
        };
    }

    /**
     * Уведомляет всех подписчиков об изменении
     */
    private notify(): void {
        this.listeners.forEach(listener => listener());
    }

    /**
     * Получает текущее состояние
     */
    getState(): FlowchartState {
        return this.state;
    }

    /**
     * Устанавливает состояние
     */
    setState(updater: Partial<FlowchartState> | ((state: FlowchartState) => FlowchartState)): void {
        if (typeof updater === 'function') {
            this.state = updater(this.state);
        } else {
            this.state = { ...this.state, ...updater };
        }
        this.notify();
    }

    // Узлы
    getNodes(): FlowchartNode[] {
        return this.state.nodes;
    }

    setNodes(nodes: FlowchartNode[]): void {
        this.setState({ nodes });
    }

    addNode(node: FlowchartNode): void {
        this.setState({ nodes: [...this.state.nodes, node] });
    }

    updateNode(nodeId: string, updates: Partial<FlowchartNode>): void {
        this.setState({
            nodes: this.state.nodes.map(node =>
                node.id === nodeId ? { ...node, ...updates } : node
            )
        });
    }

    removeNode(nodeId: string): void {
        this.setState({
            nodes: this.state.nodes.filter(node => node.id !== nodeId),
            connections: this.state.connections.filter(
                conn => conn.from !== nodeId && conn.to !== nodeId
            )
        });
    }

    // Соединения
    getConnections(): Connection[] {
        return this.state.connections;
    }

    setConnections(connections: Connection[]): void {
        this.setState({ connections });
    }

    addConnection(connection: Connection): void {
        this.setState({ connections: [...this.state.connections, connection] });
    }

    removeConnection(connectionId: string): void {
        this.setState({
            connections: this.state.connections.filter(conn => conn.id !== connectionId)
        });
    }

    updateConnection(connectionId: string, updates: Partial<Connection>): void {
        this.setState({
            connections: this.state.connections.map(conn =>
                conn.id === connectionId ? { ...conn, ...updates } : conn
            )
        });
    }

    // Выбор
    selectNode(nodeId: string | null): void {
        this.setState({ selectedNodeId: nodeId, selectedConnectionId: null });
    }

    selectConnection(connectionId: string | null): void {
        this.setState({ selectedConnectionId: connectionId, selectedNodeId: null });
    }

    clearSelection(): void {
        this.setState({ selectedNodeId: null, selectedConnectionId: null });
    }

    // История
    addHistoryEntry(entry: HistoryEntry): void {
        // Удаляем все записи после текущего индекса (если сделали новое изменение после undo)
        const newHistory = this.state.history.slice(0, this.state.historyIndex + 1);
        
        // Добавляем новую запись
        newHistory.push(entry);
        
        this.setState({
            history: newHistory,
            historyIndex: newHistory.length - 1
        });
    }

    /**
     * Откатывает последнее изменение (undo)
     */
    undo(): boolean {
        if (this.state.historyIndex > 0) {
            const newIndex = this.state.historyIndex - 1;
            const entry = this.state.history[newIndex];
            
            // Восстанавливаем состояние из истории
            this.setState({
                nodes: JSON.parse(JSON.stringify(entry.nodes)),
                connections: JSON.parse(JSON.stringify(entry.connections)),
                historyIndex: newIndex
            });
            
            return true;
        }
        return false;
    }

    /**
     * Повторяет отмененное изменение (redo)
     */
    redo(): boolean {
        if (this.state.historyIndex < this.state.history.length - 1) {
            const newIndex = this.state.historyIndex + 1;
            const entry = this.state.history[newIndex];
            
            // Восстанавливаем состояние из истории
            this.setState({
                nodes: JSON.parse(JSON.stringify(entry.nodes)),
                connections: JSON.parse(JSON.stringify(entry.connections)),
                historyIndex: newIndex
            });
            
            return true;
        }
        return false;
    }

    /**
     * Проверяет, можно ли выполнить undo
     */
    canUndo(): boolean {
        return this.state.historyIndex > 0;
    }

    /**
     * Проверяет, можно ли выполнить redo
     */
    canRedo(): boolean {
        return this.state.historyIndex < this.state.history.length - 1;
    }

    // Зум
    setZoom(zoom: number): void {
        this.setState({ zoom: Math.max(0.1, Math.min(2, zoom)) });
    }

    // Исходный код
    setSourceCode(code: string): void {
        this.setState({ sourceCode: code });
    }

    // Перетаскивание
    setDraggedNode(nodeId: string | null, offset: { x: number; y: number } = { x: 0, y: 0 }): void {
        this.setState({ draggedNode: nodeId, dragOffset: offset });
    }

    // Соединение
    setConnectingFrom(nodeId: string | null, port: 'top' | 'right' | 'bottom' | 'left' | null = null): void {
        this.setState({ connectingFrom: nodeId, connectingFromPort: port });
    }

    /**
     * Восстанавливает JSON (AST/SPR) из текущего состояния блок-схемы
     */
    getRestoredJSON(): ASTProgram | PascalProgram | null {
        return flowchartToJSON(this.state.nodes, this.state.connections);
    }
}

// Singleton instance (можно использовать в React компонентах)
let storeInstance: FlowchartStore | null = null;

export function getFlowchartStore(): FlowchartStore {
    if (!storeInstance) {
        storeInstance = new FlowchartStore();
    }
    return storeInstance;
}

export function resetFlowchartStore(): void {
    storeInstance = null;
}


