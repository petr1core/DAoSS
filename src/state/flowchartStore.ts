// State management для блок-схемы
import type { FlowchartState, FlowchartNode, Connection, HistoryEntry } from '../types/flowchart';

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
        this.setState({
            history: [...this.state.history, entry]
        });
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


