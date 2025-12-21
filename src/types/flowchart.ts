// Типы для блок-схемы

export type NodeType = 'start' | 'end' | 'process' | 'decision' | 'input' | 'output';
export type PortType = 'top' | 'right' | 'bottom' | 'left';

export interface FlowchartNode {
    id: string;
    type: NodeType;
    x: number;
    y: number;
    width: number;
    height: number;
    text: string;
    codeReference: string;
    comments: Comment[];
}

export interface Connection {
    id: string;
    from: string;
    to: string;
    fromPort: PortType;
    toPort: PortType;
    label?: string;
}

export interface Comment {
    id: string;
    text: string;
    timestamp: Date;
    nodeId: string;
}

export interface HistoryEntry {
    id: string;
    timestamp: Date;
    description: string;
    nodes: FlowchartNode[];
    connections: Connection[];
}

export interface FlowchartState {
    nodes: FlowchartNode[];
    connections: Connection[];
    selectedNodeId: string | null;
    selectedConnectionId: string | null;
    connectingFrom: string | null;
    connectingFromPort: PortType | null;
    history: HistoryEntry[];
    zoom: number;
    sourceCode: string;
    draggedNode: string | null;
    dragOffset: { x: number; y: number };
}

export interface NodeExitPoint {
    node: FlowchartNode;
    port?: PortType;
    label?: string;
}

export interface ProcessBlockResult {
    nodes: FlowchartNode[];
    exitNodes: (FlowchartNode | NodeExitPoint)[];
}


