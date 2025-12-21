// React hook для работы с flowchart store
import { useEffect, useState, useCallback } from 'react';
import { FlowchartStore, getFlowchartStore } from '../state/flowchartStore';
import type { FlowchartState, FlowchartNode, Connection, HistoryEntry } from '../types/flowchart';

/**
 * Hook для работы с состоянием блок-схемы
 */
export function useFlowchartStore() {
    const [store] = useState<FlowchartStore>(() => getFlowchartStore());
    const [state, setState] = useState<FlowchartState>(() => store.getState());

    useEffect(() => {
        const unsubscribe = store.subscribe(() => {
            setState(store.getState());
        });

        return unsubscribe;
    }, [store]);

    const updateState = useCallback((updater: Partial<FlowchartState> | ((state: FlowchartState) => FlowchartState)) => {
        store.setState(updater);
    }, [store]);

    return {
        state,
        store,
        updateState,
        // Узлы
        nodes: state.nodes,
        setNodes: useCallback((nodes: FlowchartNode[]) => store.setNodes(nodes), [store]),
        addNode: useCallback((node: FlowchartNode) => store.addNode(node), [store]),
        updateNode: useCallback((nodeId: string, updates: Partial<FlowchartNode>) => 
            store.updateNode(nodeId, updates), [store]),
        removeNode: useCallback((nodeId: string) => store.removeNode(nodeId), [store]),
        // Соединения
        connections: state.connections,
        setConnections: useCallback((connections: Connection[]) => store.setConnections(connections), [store]),
        addConnection: useCallback((connection: Connection) => store.addConnection(connection), [store]),
        removeConnection: useCallback((connectionId: string) => store.removeConnection(connectionId), [store]),
        // Выбор
        selectedNodeId: state.selectedNodeId,
        selectedConnectionId: state.selectedConnectionId,
        selectNode: useCallback((nodeId: string | null) => store.selectNode(nodeId), [store]),
        selectConnection: useCallback((connectionId: string | null) => store.selectConnection(connectionId), [store]),
        clearSelection: useCallback(() => store.clearSelection(), [store]),
        // История
        history: state.history,
        addHistoryEntry: useCallback((entry: HistoryEntry) => store.addHistoryEntry(entry), [store]),
        // Зум
        zoom: state.zoom,
        setZoom: useCallback((zoom: number) => store.setZoom(zoom), [store]),
        // Исходный код
        sourceCode: state.sourceCode,
        setSourceCode: useCallback((code: string) => store.setSourceCode(code), [store]),
        // Перетаскивание
        draggedNode: state.draggedNode,
        dragOffset: state.dragOffset,
        setDraggedNode: useCallback((nodeId: string | null, offset?: { x: number; y: number }) => 
            store.setDraggedNode(nodeId, offset), [store]),
        // Соединение
        connectingFrom: state.connectingFrom,
        connectingFromPort: state.connectingFromPort,
        setConnectingFrom: useCallback((nodeId: string | null, port?: 'top' | 'right' | 'bottom' | 'left' | null) => 
            store.setConnectingFrom(nodeId, port), [store]),
    };
}


