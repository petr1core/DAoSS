// Рефакторенная версия FlowchartEditor с использованием новых модулей
import { useEffect, useRef, useState, useCallback } from 'react';
import '../../styles.css';
import './FlowchartEditor.css';
import { useFlowchartStore } from '../hooks/useFlowchartStore';
import { useFlowchartGenerator } from '../hooks/useFlowchartGenerator';
import { renderConnections, renderAllNodes, renderNodeControls, renderInfoPanel, renderComments, renderHistory, renderConnectionControls } from '../rendering';
import { createNode, startEditingNode, startDraggingNode, updateNodePosition, stopDraggingNode } from '../handlers/nodeHandlers';
import { createConnection } from '../handlers/connectionHandlers';
import { startCanvasPan, updateCanvasPan, stopCanvasPan, zoomIn, zoomOut } from '../handlers/canvasHandlers';
import { showToast } from '../utils/toast';
import { addToHistory as addHistoryEntry } from '../utils/history';
import { addCommentToNode } from '../utils/commentUtils';
import { exportToSVG, exportToPNG, downloadSVG, downloadPNG } from '../utils/exportUtils';
import { loadExampleCode } from '../utils/fileUtils';
import { initTheme, toggleTheme as toggleThemeUtil } from '../utils/themeUtils';
import { initializeFileUpload, initializeComments, initializeTabs } from '../utils/editorInitializer';
import { renderTemporaryConnection } from '../rendering/connectionRenderer';
import type { NodeType, PortType, FlowchartNode, Connection } from '../types/flowchart';

function FlowchartEditorRefactored() {
  const editorRef = useRef<HTMLDivElement>(null);
  const canvasWrapperRef = useRef<HTMLDivElement>(null);
  const connectionsCanvasRef = useRef<HTMLCanvasElement>(null);
  const connectionsSvgRef = useRef<SVGSVGElement>(null);
  const flowchartCanvasRef = useRef<HTMLDivElement>(null);
  
  const [isDark, setIsDark] = useState(() => {
    const savedTheme = localStorage.getItem('theme');
    const prefersDark = window.matchMedia('(prefers-color-scheme: dark)').matches;
    return savedTheme === 'dark' || (!savedTheme && prefersDark);
  });

  const [panState, setPanState] = useState<{ isPanning: boolean; panStart: { x: number; y: number }; panScroll: { x: number; y: number } } | null>(null);
  const [draggedNodeState, setDraggedNodeState] = useState<{ 
    nodeId: string; 
    offset: { x: number; y: number };
  } | null>(null);
  const [mousePos, setMousePos] = useState<{ x: number; y: number } | null>(null);
  
  // Используем ref для хранения текущей позиции во время перетаскивания
  // Это позволяет избежать re-render при каждом движении мыши
  const draggedNodePositionRef = useRef<{ x: number; y: number } | null>(null);

  const flowchartStore = useFlowchartStore();
  const generator = useFlowchartGenerator();
  
  // Получаем актуальные значения из React state
  const nodes = flowchartStore.nodes;
  const connections = flowchartStore.connections;
  const selectedNodeId = flowchartStore.selectedNodeId;
  const selectedConnectionId = flowchartStore.selectedConnectionId;
  const connectingFrom = flowchartStore.connectingFrom;
  const connectingFromPort = flowchartStore.connectingFromPort;
  const zoom = flowchartStore.zoom;

  // Инициализация темы
  useEffect(() => {
    const html = document.documentElement;
    if (isDark) {
      html.classList.add('dark');
    } else {
      html.classList.remove('dark');
    }
  }, [isDark]);

  // Создаем контейнер для toast
  useEffect(() => {
    if (!document.getElementById('toast-container')) {
      const toastContainer = document.createElement('div');
      toastContainer.id = 'toast-container';
      toastContainer.className = 'toast-container';
      document.body.appendChild(toastContainer);
    }
  }, []);

  // Сохраняем актуальные значения в ref для использования в renderFlowchart
  const renderFlowchartRef = useRef<{
    nodes: FlowchartNode[];
    connections: Connection[];
    selectedNodeId: string | null;
    selectedConnectionId: string | null;
    connectingFrom: string | null;
    connectingFromPort: PortType | null;
    zoom: number;
    draggedNodeId: string | null;
  }>({
    nodes: [],
    connections: [],
    selectedNodeId: null,
    selectedConnectionId: null,
    connectingFrom: null,
    connectingFromPort: null,
    zoom: 1,
    draggedNodeId: null
  });
  
  // Обновляем ref при изменении значений
  useEffect(() => {
    renderFlowchartRef.current = {
      nodes,
      connections,
      selectedNodeId,
      selectedConnectionId,
      connectingFrom,
      connectingFromPort,
      zoom,
      draggedNodeId: draggedNodeState?.nodeId || null
    };
  }, [nodes, connections, selectedNodeId, selectedConnectionId, connectingFrom, connectingFromPort, zoom, draggedNodeState]);
  
  // Рендеринг узлов и соединений
  const renderFlowchart = useCallback(() => {
    console.log('[RENDER] renderFlowchart called');
    const canvas = connectionsCanvasRef.current;
    const svg = connectionsSvgRef.current;
    const wrapper = canvasWrapperRef.current;
    const flowchartCanvas = flowchartCanvasRef.current;

    if (!canvas || !svg || !wrapper || !flowchartCanvas) {
      console.log('[RENDER] Missing refs, skipping render');
      return;
    }
    
    // Используем актуальные значения из ref
    const currentState = renderFlowchartRef.current;
    console.log('[RENDER] Rendering flowchart with', currentState.nodes.length, 'nodes and', currentState.connections.length, 'connections');
    console.log('[RENDER] Nodes:', currentState.nodes.slice(0, 5).map(n => ({ id: n.id, type: n.type, text: n.text.substring(0, 20) })));
    
    // Рендерим соединения
    renderConnections(
      currentState.nodes,
      currentState.connections,
      canvas,
      svg,
      wrapper,
      {
        zoom: currentState.zoom,
        selectedConnectionId: currentState.selectedConnectionId,
        onConnectionClick: (connectionId) => {
          flowchartStore.selectConnection(connectionId);
        }
      }
    );

    // Рендерим узлы
    // Если узел перетаскивается, пропускаем его перерисовку чтобы сохранить позицию из DOM
    renderAllNodes(
      currentState.nodes,
      flowchartCanvas,
      {
        selectedNodeId: currentState.selectedNodeId,
        connectingFrom: currentState.connectingFrom,
        connectingFromPort: currentState.connectingFromPort,
        zoom: currentState.zoom,
        skipNodeId: renderFlowchartRef.current.draggedNodeId || undefined, // Пропускаем перерисовку перетаскиваемого узла
        onNodeClick: (nodeId) => {
          flowchartStore.selectNode(nodeId);
        },
        onNodeDoubleClick: (nodeId) => {
          startEditingNode(nodeId, flowchartStore.nodes, (nodeId, updates) => {
            flowchartStore.updateNode(nodeId, updates);
            // renderFlowchart() вызовется автоматически через useEffect
          });
        },
        onNodeDragStart: (nodeId, event) => {
          const wrapper = canvasWrapperRef.current;
          if (!wrapper) return;
          
          // Используем актуальные данные из store напрямую, а не из React state
          const currentNodes = flowchartStore.store.getState().nodes;
          const currentZoom = flowchartStore.store.getState().zoom;
          
          const dragState = startDraggingNode(nodeId, currentNodes, event, currentZoom, wrapper);
          if (dragState) {
            setDraggedNodeState(dragState);
            // Инициализируем ref с начальной позицией узла
            const node = currentNodes.find((n: FlowchartNode) => n.id === nodeId);
            if (node) {
              draggedNodePositionRef.current = { x: node.x, y: node.y };
            }
          }
        },
        onPortClick: (nodeId, port, event) => {
          event.stopPropagation();
          if (flowchartStore.connectingFrom) {
            // Завершаем соединение
            const newConnection = createConnection(
              flowchartStore.connectingFrom,
              flowchartStore.connectingFromPort || 'bottom',
              nodeId,
              port,
              flowchartStore.nodes,
              flowchartStore.connections
            );
            if (newConnection) {
              flowchartStore.addConnection(newConnection);
              flowchartStore.setConnectingFrom(null, null);
              flowchartStore.addHistoryEntry(addHistoryEntry(
                'Добавлена связь между блоками',
                flowchartStore.nodes,
                flowchartStore.connections,
                flowchartStore.history
              )[flowchartStore.history.length]);
              showToast('Связь добавлена');
            }
          } else {
            // Начинаем соединение
            flowchartStore.setConnectingFrom(nodeId, port);
            showToast('Выберите точку подключения на другом блоке', 'success');
          }
          renderFlowchart();
        },
        onRenderControls: (node, container) => {
          renderNodeControls(node, container, {
            onConnect: () => {
              flowchartStore.setConnectingFrom(node.id, 'bottom');
              // renderFlowchart() вызовется автоматически через useEffect
            },
            onDelete: () => {
              flowchartStore.removeNode(node.id);
              flowchartStore.clearSelection();
              flowchartStore.addHistoryEntry(addHistoryEntry(
                `Удален блок: ${node.text || 'Без названия'}`,
                flowchartStore.nodes,
                flowchartStore.connections,
                flowchartStore.history
              )[flowchartStore.history.length]);
              showToast('Блок удален');
              // renderFlowchart() вызовется автоматически через useEffect
            }
          });
        }
      }
    );

    // Рендерим панели
    const selectedNode = currentState.selectedNodeId
      ? currentState.nodes.find(n => n.id === currentState.selectedNodeId)
      : null;

    const infoEmpty = document.getElementById('info-empty');
    const infoPanel = document.getElementById('info-panel');
    renderInfoPanel(selectedNode || null, infoEmpty, infoPanel);

    const commentsEmpty = document.getElementById('comments-empty');
    const commentsPanel = document.getElementById('comments-panel');
    renderComments(selectedNode || null, commentsEmpty, commentsPanel);

    const historyList = document.getElementById('history-list');
    const historyCount = document.getElementById('history-count');
    renderHistory(
      flowchartStore.history,
      historyList,
      historyCount,
      (entry) => {
        const restored = entry;
        flowchartStore.setNodes(restored.nodes);
        flowchartStore.setConnections(restored.connections);
        flowchartStore.clearSelection();
        showToast('Состояние восстановлено');
        // renderFlowchart() вызовется автоматически через useEffect
      }
    );

    // Рендерим контролы соединения
    if (currentState.selectedConnectionId && flowchartCanvasRef.current) {
      const connection = currentState.connections.find(c => c.id === currentState.selectedConnectionId);
      if (connection) {
        const fromNode = currentState.nodes.find(n => n.id === connection.from) || null;
        const toNode = currentState.nodes.find(n => n.id === connection.to) || null;
        
        renderConnectionControls(
          flowchartStore.selectedConnectionId,
          connection,
          fromNode,
          toNode,
          flowchartCanvasRef.current,
          () => {
            flowchartStore.removeConnection(flowchartStore.selectedConnectionId!);
            flowchartStore.clearSelection();
            flowchartStore.addHistoryEntry(addHistoryEntry(
              'Удалена связь между блоками',
              flowchartStore.nodes,
              flowchartStore.connections,
              flowchartStore.history
            )[flowchartStore.history.length]);
            showToast('Связь удалена');
            // renderFlowchart() вызовется автоматически через useEffect
          }
        );
      }
    } else if (flowchartCanvasRef.current) {
      const existingControls = flowchartCanvasRef.current.querySelector('.connection-controls');
      if (existingControls) existingControls.remove();
    }
  }, [
    selectedConnectionId,
    connections,
    nodes
    // Отслеживаем изменения для корректного отображения контролов
  ]);

  // Обновляем рендеринг при изменении состояния
  useEffect(() => {
    console.log('[EFFECT] renderFlowchart effect triggered');
    renderFlowchart();
  }, [
    nodes.length,
    connections.length,
    selectedNodeId,
    selectedConnectionId,
    connectingFrom,
    connectingFromPort,
    zoom
    // Используем значения из React state, чтобы отслеживать реальные изменения
  ]);

  // Обработчики мыши
  const handleMouseMove = useCallback((e: MouseEvent) => {
    const wrapper = canvasWrapperRef.current;
    if (!wrapper) return;

    if (panState?.isPanning) {
      updateCanvasPan(e, panState, wrapper);
      setPanState({ ...panState });
      return;
    }

    if (draggedNodeState && wrapper) {
      // Используем прямую DOM-манипуляцию для плавного перетаскивания
      // React state обновим только при отпускании мыши
      // Используем актуальные данные из store напрямую
      const currentNodes = flowchartStore.store.getState().nodes;
      const currentZoom = flowchartStore.store.getState().zoom;
      
      const newPos = updateNodePosition(
        draggedNodeState.nodeId,
        currentNodes,
        e,
        draggedNodeState.offset,
        currentZoom,
        wrapper
      );
      
      // Сохраняем позицию в ref вместо setState, чтобы избежать re-render
      draggedNodePositionRef.current = newPos;
      
      // Рендеринг узлов не нужен - мы используем прямую DOM-манипуляцию
      // Соединения обновим только при mouseUp
    }

    // Отслеживаем позицию мыши для временной линии соединения
    const rect = wrapper.getBoundingClientRect();
    setMousePos({
      x: e.clientX - rect.left + wrapper.scrollLeft,
      y: e.clientY - rect.top + wrapper.scrollTop
    });

    if (connectingFrom && mousePos) {
      const fromNode = nodes.find(n => n.id === connectingFrom);
      if (fromNode && connectionsCanvasRef.current) {
        // Рендерим временную линию соединения
        renderTemporaryConnection(
          connectionsCanvasRef.current,
          fromNode,
          connectingFromPort || 'bottom',
          mousePos.x,
          mousePos.y,
          zoom
        );
      }
    }
  }, [panState, draggedNodeState, nodes, connectingFrom, connectingFromPort, zoom, mousePos]);

  const handleMouseUp = useCallback(() => {
    if (draggedNodeState) {
      // Сохраняем финальную позицию в React state только при отпускании мыши
      if (draggedNodePositionRef.current) {
        flowchartStore.updateNode(draggedNodeState.nodeId, {
          x: draggedNodePositionRef.current.x,
          y: draggedNodePositionRef.current.y
        });
        draggedNodePositionRef.current = null;
      }
      
      stopDraggingNode(draggedNodeState.nodeId);
      setDraggedNodeState(null);
      
      // Обновляем соединения после перемещения узла
      renderFlowchart();
    }
    if (panState) {
      setPanState(stopCanvasPan(panState, canvasWrapperRef.current));
    }
  }, [draggedNodeState, panState]);

  // Устанавливаем обработчики мыши
  useEffect(() => {
    const wrapper = canvasWrapperRef.current;
    if (!wrapper) return;

    wrapper.addEventListener('mousemove', handleMouseMove);
    wrapper.addEventListener('mouseup', handleMouseUp);

    return () => {
      wrapper.removeEventListener('mousemove', handleMouseMove);
      wrapper.removeEventListener('mouseup', handleMouseUp);
    };
  }, [handleMouseMove, handleMouseUp]);

  // Обработка клика по пустому месту для panning
  const handleCanvasMouseDown = useCallback((e: React.MouseEvent) => {
    const wrapper = canvasWrapperRef.current;
    if (!wrapper) return;

    const panState = startCanvasPan(e.nativeEvent, wrapper);
    if (panState) {
      setPanState(panState);
      wrapper.style.cursor = 'grabbing';
    }
  }, []);

  // Добавление узла
  const handleAddNode = useCallback((type: NodeType) => {
    const newNode = createNode(type, flowchartStore.nodes);
    flowchartStore.addNode(newNode);
    flowchartStore.addHistoryEntry(addHistoryEntry(
      `Добавлен блок: ${newNode.text}`,
      flowchartStore.nodes,
      flowchartStore.connections,
      flowchartStore.history
    )[flowchartStore.history.length]);
    showToast('Блок добавлен');
    // renderFlowchart() вызовется автоматически через useEffect при изменении nodes.length
  }, [flowchartStore]);

  // Генерация блок-схемы из кода
  // Флаг для предотвращения множественных одновременных вызовов генерации
  const isGeneratingRef = useRef(false);

  const handleGenerateFlowchart = useCallback(async () => {
    // Предотвращаем множественные вызовы
    if (isGeneratingRef.current) {
      console.log('[GENERATE] Already generating, skipping...');
      return;
    }

    isGeneratingRef.current = true;
    const codeEditor = document.getElementById('code-editor') as HTMLTextAreaElement | null;
    const languageSelect = document.getElementById('language-select') as HTMLSelectElement | null;
    const generateBtn = document.getElementById('generate-btn') as HTMLButtonElement | null;

    if (!codeEditor || !languageSelect) return;

    const code = codeEditor.value;
    const language = languageSelect.value as 'pascal' | 'c' | 'cpp';

    // Показываем индикатор загрузки
    const originalText = generateBtn ? generateBtn.innerHTML : '';
    if (generateBtn) {
      generateBtn.disabled = true;
      generateBtn.innerHTML = 'Генерация...';
    }

    try {
      flowchartStore.setSourceCode(code);
      
      const result = await generator.generate(code, language);
      
      if (result) {
        console.log('[GENERATE] Setting nodes:', result.nodes.length, 'connections:', result.connections.length);
        
        // Сохраняем историю до обновления состояния
        const currentHistory = [...flowchartStore.history];
        
        // Обновляем состояние - React state обновится автоматически через subscribe
        flowchartStore.setNodes(result.nodes);
        flowchartStore.setConnections(result.connections);
        flowchartStore.clearSelection();
        
        // Добавляем в историю с использованием новых значений
        flowchartStore.addHistoryEntry(addHistoryEntry(
          'Загружен исходный код и сгенерирована схема через парсер',
          result.nodes, // Используем новые узлы
          result.connections, // Используем новые соединения
          currentHistory
        )[currentHistory.length]);
        
        showToast('Блок-схема сгенерирована из кода', 'success');
        // renderFlowchart() вызовется автоматически через useEffect при изменении nodes.length
      }
    } catch (error) {
      console.error('Ошибка при генерации блок-схемы:', error);
      showToast('Не удалось сгенерировать блок-схему', 'error');
    } finally {
      isGeneratingRef.current = false;
      if (generateBtn) {
        generateBtn.disabled = false;
        generateBtn.innerHTML = originalText;
      }
    }
  }, [generator, flowchartStore, renderFlowchart]);

  // Инициализация обработчиков кнопок
  useEffect(() => {
    // Кнопки добавления узлов
    document.querySelectorAll('.tool-btn').forEach(btn => {
      const type = (btn as HTMLElement).dataset.type;
      if (type) {
        btn.addEventListener('click', () => handleAddNode(type as NodeType));
      }
    });

    // Кнопки зума
    const zoomInBtn = document.getElementById('zoom-in');
    const zoomOutBtn = document.getElementById('zoom-out');
    const zoomValue = document.getElementById('zoom-value');

    if (zoomInBtn) {
      zoomInBtn.addEventListener('click', () => {
        flowchartStore.setZoom(zoomIn(flowchartStore.zoom));
        if (zoomValue) zoomValue.textContent = Math.round(flowchartStore.zoom * 100).toString();
        // renderFlowchart() вызовется автоматически через useEffect при изменении zoom
      });
    }

    if (zoomOutBtn) {
      zoomOutBtn.addEventListener('click', () => {
        flowchartStore.setZoom(zoomOut(flowchartStore.zoom));
        if (zoomValue) zoomValue.textContent = Math.round(flowchartStore.zoom * 100).toString();
        // renderFlowchart() вызовется автоматически через useEffect при изменении zoom
      });
    }

    // Кнопка генерации блок-схемы
    const generateBtn = document.getElementById('generate-btn');
    if (generateBtn) {
      generateBtn.addEventListener('click', handleGenerateFlowchart);
    }

    // Кнопка сохранения узла
    const saveNodeBtn = document.getElementById('save-node-btn');
    if (saveNodeBtn) {
      saveNodeBtn.addEventListener('click', () => {
        if (!flowchartStore.selectedNodeId) return;

        const textInput = document.getElementById('node-text-input') as HTMLTextAreaElement | null;
        const codeInput = document.getElementById('node-code-input') as HTMLTextAreaElement | null;

        if (textInput) {
          flowchartStore.updateNode(flowchartStore.selectedNodeId, { text: textInput.value });
        }
        if (codeInput) {
          flowchartStore.updateNode(flowchartStore.selectedNodeId, { codeReference: codeInput.value });
        }

        showToast('Изменения сохранены');
        // renderFlowchart() вызовется автоматически через useEffect при изменении nodes
      });
    }

    // Экспорт
    const exportSvgBtn = document.getElementById('export-svg');
    const exportPngBtn = document.getElementById('export-png');
    
    if (exportSvgBtn) {
      exportSvgBtn.addEventListener('click', () => {
        try {
          const svgContent = exportToSVG(
            canvasWrapperRef.current!,
            flowchartStore.nodes,
            flowchartStore.connections
          );
          downloadSVG(svgContent, 'flowchart.svg');
          showToast('Экспорт SVG выполнен');
        } catch (error) {
          showToast('Ошибка экспорта SVG', 'error');
          console.error('Export SVG error:', error);
        }
      });
    }
    
    if (exportPngBtn) {
      exportPngBtn.addEventListener('click', async () => {
        try {
          const dataUrl = await exportToPNG(
            canvasWrapperRef.current!,
            flowchartStore.nodes,
            flowchartStore.connections,
            flowchartStore.zoom
          );
          downloadPNG(dataUrl, 'flowchart.png');
          showToast('Экспорт PNG выполнен');
        } catch (error) {
          showToast('Ошибка экспорта PNG', 'error');
          console.error('Export PNG error:', error);
        }
      });
    }

    // Загрузка примера
    const loadExampleBtn = document.getElementById('load-example-btn');
    if (loadExampleBtn) {
      loadExampleBtn.addEventListener('click', async () => {
        const result = await loadExampleCode('pascal.txt');
        if (result) {
          const codeEditor = document.getElementById('code-editor') as HTMLTextAreaElement | null;
          const languageSelect = document.getElementById('language-select') as HTMLSelectElement | null;
          
          if (codeEditor) codeEditor.value = result.code;
          if (languageSelect) languageSelect.value = result.language;
          
          showToast('Пример загружен');
        } else {
          // Загружаем встроенный пример
          const exampleCode = `int main() {
  int x = 0;
  int y = 10;
  
  if (x < y) {
    cout << "x меньше y";
    x = x + 1;
  }
  
  for (int i = 0; i < 5; i++) {
    cout << i;
  }
  
  return 0;
}`;
          const codeEditor = document.getElementById('code-editor') as HTMLTextAreaElement | null;
          const languageSelect = document.getElementById('language-select') as HTMLSelectElement | null;
          if (codeEditor) codeEditor.value = exampleCode;
          if (languageSelect) languageSelect.value = 'cpp';
        }
      });
    }

    // Загрузка файла
    initializeFileUpload(() => {
      // Файл уже установлен в редактор внутри initializeFileUpload
    });

    // Комментарии
    initializeComments(() => {
      if (!flowchartStore.selectedNodeId) return;
      
      const commentInput = document.getElementById('new-comment') as HTMLTextAreaElement | null;
      if (!commentInput) return;
      
      const commentText = commentInput.value.trim();
      if (!commentText) return;
      
      const success = addCommentToNode(
        flowchartStore.selectedNodeId,
        commentText,
        flowchartStore.nodes
      );
      
      if (success) {
        commentInput.value = '';
        flowchartStore.addHistoryEntry(addHistoryEntry(
          'Добавлен комментарий',
          flowchartStore.nodes,
          flowchartStore.connections,
          flowchartStore.history
        )[flowchartStore.history.length]);
        showToast('Комментарий добавлен');
        // renderFlowchart() вызовется автоматически через useEffect при изменении nodes
      }
    });

    // Переключение вкладок
    initializeTabs();

    // Escape для отмены соединения
    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.key === 'Escape' && flowchartStore.connectingFrom) {
        flowchartStore.setConnectingFrom(null, null);
        showToast('Соединение отменено');
        // renderFlowchart() вызовется автоматически через useEffect при изменении connectingFrom
      }
    };
    
    document.addEventListener('keydown', handleKeyDown);

    // Клик по пустому месту для очистки выбора
    const handleCanvasClick = (e: MouseEvent) => {
      const target = e.target as HTMLElement;
      if (
        target === canvasWrapperRef.current ||
        target.id === 'flowchart-canvas' ||
        target.id === 'connections-canvas' ||
        target.id === 'connections-svg' ||
        target.tagName === 'svg' ||
        target.tagName === 'path'
      ) {
        flowchartStore.clearSelection();
        // renderFlowchart() вызовется автоматически через useEffect при изменении selectedNodeId
      }
    };

    const wrapper = canvasWrapperRef.current;
    if (wrapper) {
      wrapper.addEventListener('click', handleCanvasClick);
    }

    return () => {
      document.removeEventListener('keydown', handleKeyDown);
      if (wrapper) {
        wrapper.removeEventListener('click', handleCanvasClick);
      }
    };
  }, [flowchartStore, handleAddNode, handleGenerateFlowchart, renderFlowchart]);

  // Инициализация темы
  useEffect(() => {
    const initialIsDark = initTheme();
    setIsDark(initialIsDark);
  }, []);


  // Рендерим временную линию соединения
  useEffect(() => {
    if (flowchartStore.connectingFrom && mousePos) {
      const fromNode = flowchartStore.nodes.find(n => n.id === flowchartStore.connectingFrom);
      if (fromNode && connectionsCanvasRef.current) {
        renderTemporaryConnection(
          connectionsCanvasRef.current,
          fromNode,
          flowchartStore.connectingFromPort || 'bottom',
          mousePos.x,
          mousePos.y,
          flowchartStore.zoom
        );
      }
    }
  }, [flowchartStore.connectingFrom, flowchartStore.connectingFromPort, mousePos, flowchartStore.nodes, flowchartStore.zoom]);

  return (
    <div className="app-container" ref={editorRef}>
      {/* Левая панель - Инструменты */}
      <aside className="sidebar-left">
        <h2>Панель инструментов</h2>
        
        <div className="toolbar-section">
          <h3>Добавить блок</h3>
          <div className="toolbar-buttons">
            <button className="tool-btn" data-type="start">
              <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                <circle cx="12" cy="12" r="10"></circle>
              </svg>
              <span>Начало/Конец</span>
            </button>
            <button className="tool-btn" data-type="process">
              <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                <rect x="3" y="3" width="18" height="18" rx="2"></rect>
              </svg>
              <span>Процесс</span>
            </button>
            <button className="tool-btn" data-type="decision">
              <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                <path d="M12 2 L22 12 L12 22 L2 12 Z"></path>
              </svg>
              <span>Условие</span>
            </button>
            <button className="tool-btn" data-type="input">
              <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                <path d="M12 5v14M19 12l-7 7-7-7"></path>
              </svg>
              <span>Ввод</span>
            </button>
            <button className="tool-btn" data-type="output">
              <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                <path d="M12 19V5M5 12l7-7 7 7"></path>
              </svg>
              <span>Вывод</span>
            </button>
          </div>
        </div>

        <div className="toolbar-section">
          <h3>Управление</h3>
          <button className="control-btn" id="zoom-in">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
              <circle cx="11" cy="11" r="8"></circle>
              <path d="m21 21-4.35-4.35M11 8v6M8 11h6"></path>
            </svg>
            Увеличить
          </button>
          <button className="control-btn" id="zoom-out">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
              <circle cx="11" cy="11" r="8"></circle>
              <path d="m21 21-4.35-4.35M8 11h6"></path>
            </svg>
            Уменьшить
          </button>
          <button className="control-btn" id="export-svg">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
              <path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4M7 10l5 5 5-5M12 15V3"></path>
            </svg>
            Экспорт SVG
          </button>
          <button className="control-btn" id="export-png">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
              <path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4M7 10l5 5 5-5M12 15V3"></path>
            </svg>
            Экспорт PNG
          </button>
        </div>
      </aside>

      {/* Центральная область - Canvas */}
      <main className="main-content">
        <header className="top-bar">
          <h1>Редактор блок-схем</h1>
          <div style={{ display: 'flex', alignItems: 'center', gap: '1rem' }}>
            <button 
              id="theme-toggle" 
              className="control-btn"
              title="Переключить тему"
              onClick={() => {
                const newIsDark = toggleThemeUtil();
                setIsDark(newIsDark);
              }}
            >
              <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                <circle cx="12" cy="12" r="5"></circle>
                <line x1="12" y1="1" x2="12" y2="3"></line>
                <line x1="12" y1="21" x2="12" y2="23"></line>
                <line x1="4.22" y1="4.22" x2="5.64" y2="5.64"></line>
                <line x1="18.36" y1="18.36" x2="19.78" y2="19.78"></line>
                <line x1="1" y1="12" x2="3" y2="12"></line>
                <line x1="21" y1="12" x2="23" y2="12"></line>
                <line x1="4.22" y1="19.78" x2="5.64" y2="18.36"></line>
                <line x1="18.36" y1="5.64" x2="19.78" y2="4.22"></line>
              </svg>
              <span className="theme-toggle-text">{isDark ? 'Тёмная' : 'Светлая'}</span>
            </button>
            <div className="zoom-indicator">
              Масштаб: <span id="zoom-value">100</span>%
            </div>
          </div>
        </header>
        <div 
          className="canvas-wrapper" 
          id="canvas-wrapper" 
          ref={canvasWrapperRef}
          onMouseDown={handleCanvasMouseDown}
        >
          <canvas id="connections-canvas" ref={connectionsCanvasRef}></canvas>
          <svg id="connections-svg" ref={connectionsSvgRef}></svg>
          <div className="flowchart-canvas" id="flowchart-canvas" ref={flowchartCanvasRef}></div>
        </div>
      </main>

      {/* Правая панель */}
      <aside className="sidebar-right">
        <div className="tabs">
          <button className="tab-btn active" data-tab="info">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
              <path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"></path>
              <path d="M14 2v6h6M16 13H8M16 17H8M10 9H8"></path>
            </svg>
          </button>
          <button className="tab-btn" data-tab="code">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
              <path d="m18 16 4-4-4-4M6 8l-4 4 4 4M14.5 4l-5 16"></path>
            </svg>
          </button>
          <button className="tab-btn" data-tab="comments">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
              <path d="M21 15a2 2 0 0 1-2 2H7l-4 4V5a2 2 0 0 1 2-2h14a2 2 0 0 1 2 2z"></path>
            </svg>
          </button>
          <button className="tab-btn" data-tab="history">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
              <path d="M3 12a9 9 0 1 0 9-9 9.75 9.75 0 0 0-6.74 2.74L3 8"></path>
              <path d="M3 3v5h5M12 7v5l4 2"></path>
            </svg>
          </button>
        </div>

        <div className="tab-content active" id="tab-info">
          <div className="empty-state" id="info-empty">
            <p>Выберите элемент блок-схемы для просмотра информации</p>
          </div>
          <div className="info-panel" id="info-panel" style={{ display: 'none', flexDirection: 'column' }}>
            <h3>Информация о блоке</h3>
            <div className="form-group">
              <label>Тип блока</label>
              <div className="node-type-badge" id="node-type-badge"></div>
            </div>
            <div className="form-group">
              <label htmlFor="node-id-input">ID блока</label>
              <input type="text" id="node-id-input" readOnly />
            </div>
            <div className="form-group">
              <label htmlFor="node-text-input">Текст блока</label>
              <textarea id="node-text-input" rows={6}></textarea>
            </div>
            <div className="form-group">
              <label htmlFor="node-x-input">Позиция X</label>
              <input type="text" id="node-x-input" readOnly />
            </div>
            <div className="form-group">
              <label htmlFor="node-y-input">Позиция Y</label>
              <input type="text" id="node-y-input" readOnly />
            </div>
            <div className="form-group">
              <label htmlFor="node-code-input">Фрагмент кода</label>
              <textarea id="node-code-input" rows={4} placeholder="Связанный фрагмент кода..."></textarea>
            </div>
            <button className="primary-btn" id="save-node-btn">
              <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                <path d="M19 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11l5 5v11a2 2 0 0 1-2 2z"></path>
                <polyline points="17 21 17 13 7 13 7 21"></polyline>
                <polyline points="7 3 7 8 15 8"></polyline>
              </svg>
              Сохранить изменения
            </button>
            <div className="info-hint">
              <p><strong>Подсказка:</strong></p>
              <ul>
                <li>Двойной клик на блоке для быстрого редактирования</li>
                <li>Перетаскивайте блоки для изменения позиции</li>
              </ul>
            </div>
          </div>
        </div>

        <div className="tab-content" id="tab-code">
          <h3>Загрузка исходного кода</h3>
          <p className="tab-description">Загрузите код или введите его вручную для автоматической генерации блок-схемы</p>
          <div className="code-controls">
            <label htmlFor="file-upload" className="file-upload-btn">
              <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                <path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4M17 8l-5-5-5 5M12 3v12"></path>
              </svg>
              Загрузить файл
            </label>
            <input type="file" id="file-upload" accept=".cpp,.c,.pas,.p,.pp,.java,.py,.js,.ts" style={{ display: 'none' }} />
            <button className="control-btn" id="load-example-btn">
              <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                <path d="m18 16 4-4-4-4M6 8l-4 4 4 4M14.5 4l-5 16"></path>
              </svg>
              Загрузить пример
            </button>
          </div>
          <div className="form-group">
            <label htmlFor="language-select">Язык программирования:</label>
            <select id="language-select" className="form-control" style={{ width: '100%', padding: '8px', marginBottom: '10px' }}>
              <option value="pascal">Pascal</option>
              <option value="c">C</option>
              <option value="cpp">C++</option>
            </select>
          </div>
          <div className="form-group">
            <label htmlFor="code-editor">Исходный код:</label>
            <textarea id="code-editor" rows={10} placeholder="Вставьте или введите код здесь..."></textarea>
          </div>
          <button className="primary-btn" id="generate-btn">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
              <path d="m18 16 4-4-4-4M6 8l-4 4 4 4M14.5 4l-5 16"></path>
            </svg>
            Сгенерировать блок-схему
          </button>
          <div className="info-hint">
            <p><strong>Поддерживаемые конструкции:</strong></p>
            <ul>
              <li>if, while, for - условия и циклы</li>
              <li>print, cout, console.log - вывод</li>
              <li>input, cin, scanf - ввод</li>
              <li>Прочие операторы - процессы</li>
            </ul>
          </div>
        </div>

        <div className="tab-content" id="tab-comments">
          <div className="empty-state" id="comments-empty">
            <svg width="48" height="48" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" style={{ opacity: 0.5, margin: '0 auto 1rem' }}>
              <path d="M21 15a2 2 0 0 1-2 2H7l-4 4V5a2 2 0 0 1 2-2h14a2 2 0 0 1 2 2z"></path>
            </svg>
            <p>Выберите элемент для просмотра и добавления комментариев</p>
          </div>
          <div className="comments-panel" id="comments-panel" style={{ display: 'none' }}>
            <h3>Комментарии</h3>
            <p className="selected-node-name">Блок: <span id="comment-node-name"></span></p>
            <div className="comments-list" id="comments-list"></div>
            <div className="comment-form">
              <textarea id="new-comment" rows={3} placeholder="Введите комментарий..."></textarea>
              <button className="primary-btn" id="add-comment-btn">
                <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                  <path d="m22 2-7 20-4-9-9-4Z"></path>
                  <path d="M22 2 11 13"></path>
                </svg>
                Добавить комментарий
              </button>
              <p className="hint-text">Ctrl + Enter для быстрой отправки</p>
            </div>
          </div>
        </div>

        <div className="tab-content" id="tab-history">
          <h3>История изменений</h3>
          <p className="history-count">Всего записей: <span id="history-count">0</span></p>
          <div className="history-list" id="history-list"></div>
          <div className="info-hint">
            <p><strong>Информация:</strong></p>
            <ul>
              <li>История сохраняется автоматически при изменениях</li>
              <li>Нажмите на кнопку восстановления для возврата к версии</li>
            </ul>
          </div>
        </div>
      </aside>
      
      {/* Toast Container */}
      <div id="toast-container" className="toast-container"></div>
    </div>
  );
}

export default FlowchartEditorRefactored;

