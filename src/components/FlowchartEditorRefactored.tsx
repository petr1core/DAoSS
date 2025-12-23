// Рефакторенная версия FlowchartEditor с использованием новых модулей
import { useEffect, useRef, useState, useCallback } from 'react';
import { useSearchParams } from 'react-router-dom';
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
import { initializeFileUpload, initializeComments, initializeTabs } from '../utils/editorInitializer';
import { renderTemporaryConnection } from '../rendering/connectionRenderer';
import type { NodeType, PortType, FlowchartNode, Connection } from '../types/flowchart';
import { flowchartToJSON } from '../converters';
import { api } from '../services/api';

interface ParserNotification {
  type: 'error' | 'warning' | 'success';
  message: string;
}

function FlowchartEditorRefactored() {
  const editorRef = useRef<HTMLDivElement>(null);
  const canvasWrapperRef = useRef<HTMLDivElement>(null);
  const connectionsCanvasRef = useRef<HTMLCanvasElement>(null);
  const connectionsSvgRef = useRef<SVGSVGElement>(null);
  const flowchartCanvasRef = useRef<HTMLDivElement>(null);
  
  const [searchParams] = useSearchParams();
  const [fileLoaded, setFileLoaded] = useState(false);
  const [fileName, setFileName] = useState<string | null>(null);
  const [notification, setNotification] = useState<ParserNotification | null>(null);
  const [isLoadingFile, setIsLoadingFile] = useState(false);
  

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

  // Создаем контейнер для toast
  useEffect(() => {
    if (!document.getElementById('toast-container')) {
      const toastContainer = document.createElement('div');
      toastContainer.id = 'toast-container';
      toastContainer.className = 'toast-container';
      document.body.appendChild(toastContainer);
    }
  }, []);

  // Загрузка файла из URL параметров
  useEffect(() => {
    const fileId = searchParams.get('fileId');
    const projectId = searchParams.get('projectId');

    if (fileId && projectId && !fileLoaded) {
      // Ждем, пока DOM элементы будут готовы
      const checkAndLoad = () => {
        const codeEditor = document.getElementById('code-editor');
        if (codeEditor) {
          loadFileAndGenerateDiagram(projectId, fileId);
        } else {
          // Повторяем проверку через 100мс
          setTimeout(checkAndLoad, 100);
        }
      };
      
      // Небольшая задержка для первоначального рендера
      setTimeout(checkAndLoad, 200);
    }
  }, [searchParams, fileLoaded]);

  const loadFileAndGenerateDiagram = async (projectId: string, fileId: string) => {
    setIsLoadingFile(true);
    setNotification(null);
    
    try {
      // Получаем информацию о файле
      const file = await api.getSourceFile(projectId, fileId);
      setFileName(file.path);

      // Получаем версии файла
      const versions = await api.getSourceFileVersions(projectId, fileId);
      if (versions.length === 0) {
        setNotification({ type: 'warning', message: 'Файл не имеет версий' });
        setIsLoadingFile(false);
        return;
      }

      // Находим последнюю версию
      const latestVersion = versions.reduce((latest, current) => 
        current.versionIndex > latest.versionIndex ? current : latest
      );

      // Получаем содержимое последней версии
      const version = await api.getSourceFileVersion(projectId, fileId, latestVersion.id);
      const code = version.content;

      // Определяем язык по расширению файла
      const extension = file.path.split('.').pop()?.toLowerCase();
      let language: 'pascal' | 'c' | 'cpp' = 'cpp';
      if (extension === 'pas') language = 'pascal';
      else if (extension === 'c') language = 'c';

      // Устанавливаем код в textarea
      const codeEditor = document.getElementById('code-editor') as HTMLTextAreaElement | null;
      const languageSelect = document.getElementById('language-select') as HTMLSelectElement | null;
      
      if (codeEditor) {
        codeEditor.value = code;
        console.log('[FILE LOAD] Код установлен в textarea, длина:', code.length);
      } else {
        console.warn('[FILE LOAD] code-editor не найден!');
      }
      
      if (languageSelect) {
        languageSelect.value = language;
        console.log('[FILE LOAD] Язык установлен:', language);
      }

      // Автоматически запускаем генерацию диаграммы
      // Небольшая задержка для обновления DOM
      setTimeout(() => {
        const generateBtn = document.getElementById('generate-btn') as HTMLButtonElement | null;
        if (generateBtn) {
          console.log('[FILE LOAD] Запускаем генерацию диаграммы...');
          generateBtn.click();
          setNotification({ type: 'success', message: 'Файл загружен, диаграмма генерируется...' });
        } else {
          console.warn('[FILE LOAD] Кнопка генерации не найдена!');
          setNotification({ 
            type: 'warning', 
            message: 'Код загружен. Нажмите "Сгенерировать блок-схему" для создания диаграммы.'
          });
        }
      }, 100);

      setFileLoaded(true);
    } catch (error) {
      console.error('Ошибка при загрузке файла:', error);
      setNotification({ 
        type: 'error', 
        message: `Не удалось загрузить файл: ${error instanceof Error ? error.message : 'неизвестная ошибка'}`
      });
    } finally {
      setIsLoadingFile(false);
    }
  };

  const dismissNotification = () => {
    setNotification(null);
  };

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

  // Ref для функции renderFlowchart, чтобы она была доступна в обработчиках
  const renderFlowchartFnRef = useRef<(() => void) | null>(null);
  
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

    // Применяем zoom к контейнеру
    flowchartCanvas.style.transform = `scale(${currentState.zoom})`;
    flowchartCanvas.style.transformOrigin = 'top left';
    
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
          event.preventDefault();
          
          // Получаем актуальное состояние напрямую из store
          const currentState = flowchartStore.store.getState();
          const connectingFrom = currentState.connectingFrom;
          const connectingFromPort = currentState.connectingFromPort;
          
          if (connectingFrom) {
            // Проверяем, не кликаем ли мы на тот же узел, откуда начали соединение
            if (connectingFrom === nodeId) {
              // Если кликнули на тот же узел, но другой порт - отменяем режим соединения
              if (connectingFromPort !== port) {
                flowchartStore.setConnectingFrom(null, null);
                showToast('Соединение отменено', 'info');
                renderFlowchart();
              }
              return;
            }
            
            // Завершаем соединение
            const newConnection = createConnection(
              connectingFrom,
              connectingFromPort || 'bottom',
              nodeId,
              port,
              currentState.nodes,
              currentState.connections
            );
            
            if (newConnection) {
              flowchartStore.addConnection(newConnection);
              // Очищаем режим соединения ПЕРЕД добавлением в историю
              flowchartStore.setConnectingFrom(null, null);
              
              // Получаем обновленное состояние после добавления соединения
              const updatedState = flowchartStore.store.getState();
              flowchartStore.addHistoryEntry(addHistoryEntry(
                'Добавлена связь между блоками',
                updatedState.nodes,
                updatedState.connections,
                updatedState.history
              )[updatedState.history.length]);
              
              showToast('Связь добавлена', 'success');
              renderFlowchart();
            } else {
              // Если соединение не создалось (например, уже существует), очищаем режим
              flowchartStore.setConnectingFrom(null, null);
              renderFlowchart();
            }
          } else {
            // Начинаем соединение
            flowchartStore.setConnectingFrom(nodeId, port);
            showToast('Выберите точку подключения на другом блоке', 'info');
            renderFlowchart();
          }
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

    // Добавляем обработчики для полей ввода после рендеринга панели
    // Это нужно, потому что поля могут пересоздаваться при ререндере
    const textInputAfterRender = document.getElementById('node-text-input') as HTMLTextAreaElement | null;
    const codeInputAfterRender = document.getElementById('node-code-input') as HTMLTextAreaElement | null;

    if (textInputAfterRender && selectedNode) {
      // Удаляем старый обработчик, если есть
      const oldHandler = (textInputAfterRender as any)._blurHandler;
      if (oldHandler) {
        textInputAfterRender.removeEventListener('blur', oldHandler);
      }
      
      // Создаем новый обработчик
      const handleTextInputBlur = () => {
        if (!selectedNode.id) return;
        
        // Сохраняем текущее состояние для истории
        const currentState = flowchartStore.store.getState();
        const oldText = selectedNode.text;
        const newText = textInputAfterRender.value;
        
        // Обновляем только если текст действительно изменился
        if (oldText !== newText) {
          flowchartStore.updateNode(selectedNode.id, { text: newText });
          
          // Добавляем в историю после обновления
          const updatedState = flowchartStore.store.getState();
          flowchartStore.addHistoryEntry(addHistoryEntry(
            `Изменен текст блока: "${oldText}" → "${newText}"`,
            updatedState.nodes,
            updatedState.connections,
            updatedState.history
          )[updatedState.history.length]);
          
          // Явно вызываем ререндер после изменения текста
          setTimeout(() => {
            if (renderFlowchartFnRef.current) {
              renderFlowchartFnRef.current();
            }
          }, 0);
        }
      };
      
      // Сохраняем ссылку для последующего удаления
      (textInputAfterRender as any)._blurHandler = handleTextInputBlur;
      textInputAfterRender.addEventListener('blur', handleTextInputBlur);
    }

    // Обработчик для переключателя прототипа функции
    const prototypeToggle = document.getElementById('function-prototype-toggle') as HTMLInputElement | null;
    if (prototypeToggle && selectedNode && (selectedNode as any).isFunction) {
      // Удаляем старый обработчик, если есть
      const oldHandler = (prototypeToggle as any)._changeHandler;
      if (oldHandler) {
        prototypeToggle.removeEventListener('change', oldHandler);
      }
      
      const handlePrototypeToggle = () => {
        if (!selectedNode.id) return;
        
        const isPrototype = prototypeToggle.checked;
        const currentState = flowchartStore.store.getState();
        
        // Создаем новый массив узлов с обновленным флагом isPrototype
        const updatedNodes = currentState.nodes.map(node => {
          if (node.id === selectedNode.id) {
            const updatedNode = { ...node };
            (updatedNode as any).isPrototype = isPrototype;
            return updatedNode;
          }
          return node;
        });
        
        // Обновляем узлы в store
        flowchartStore.setNodes(updatedNodes);
        
        // Обновляем readOnly состояние поля кода
        if (codeInputAfterRender) {
          codeInputAfterRender.readOnly = isPrototype;
          codeInputAfterRender.placeholder = isPrototype 
            ? 'Прототип функции не имеет тела (только для чтения)'
            : 'Тело функции';
        }
        
        // Добавляем в историю
        const updatedState = flowchartStore.store.getState();
        flowchartStore.addHistoryEntry(addHistoryEntry(
          `Функция "${selectedNode.text}" изменена на ${isPrototype ? 'прототип' : 'реализацию'}`,
          updatedState.nodes,
          updatedState.connections,
          updatedState.history
        )[updatedState.history.length]);
        
        // Вызываем ререндер
        setTimeout(() => {
          if (renderFlowchartFnRef.current) {
            renderFlowchartFnRef.current();
          }
        }, 0);
      };
      
      // Сохраняем ссылку для последующего удаления
      (prototypeToggle as any)._changeHandler = handlePrototypeToggle;
      prototypeToggle.addEventListener('change', handlePrototypeToggle);
    }

    if (codeInputAfterRender && selectedNode && !codeInputAfterRender.readOnly) {
      // Удаляем старый обработчик, если есть
      const oldHandler = (codeInputAfterRender as any)._blurHandler;
      if (oldHandler) {
        codeInputAfterRender.removeEventListener('blur', oldHandler);
      }
      
      // Создаем новый обработчик
      const handleCodeInputBlur = () => {
        if (!selectedNode.id) return;
        
        // Сохраняем текущее состояние для истории
        const currentState = flowchartStore.store.getState();
        const oldCodeRef = selectedNode.codeReference;
        const newCodeRef = codeInputAfterRender.value;
        
        // Обновляем только если код действительно изменился
        if (oldCodeRef !== newCodeRef) {
          flowchartStore.updateNode(selectedNode.id, { codeReference: newCodeRef });
          
          // Добавляем в историю после обновления
          const updatedState = flowchartStore.store.getState();
          flowchartStore.addHistoryEntry(addHistoryEntry(
            `Изменен фрагмент кода блока "${selectedNode.text}"`,
            updatedState.nodes,
            updatedState.connections,
            updatedState.history
          )[updatedState.history.length]);
          
          // Явно вызываем ререндер после изменения кода
          setTimeout(() => {
            if (renderFlowchartFnRef.current) {
              renderFlowchartFnRef.current();
            }
          }, 0);
        }
      };
      
      // Сохраняем ссылку для последующего удаления
      (codeInputAfterRender as any)._blurHandler = handleCodeInputBlur;
      codeInputAfterRender.addEventListener('blur', handleCodeInputBlur);
    }

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
          },
          () => {
            // Открываем диалог для редактирования метки
            const currentLabel = connection.label || '';
            const newLabel = prompt('Введите метку для связи (например: true, false, или ваша метка):', currentLabel);
            if (newLabel !== null) {
              flowchartStore.updateConnection(connection.id, { label: newLabel.trim() || undefined });
              flowchartStore.addHistoryEntry(addHistoryEntry(
                `Обновлена метка связи: ${newLabel.trim() || '(удалена)'}`,
                flowchartStore.nodes,
                flowchartStore.connections,
                flowchartStore.history
              )[flowchartStore.history.length]);
              showToast('Метка обновлена');
            }
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

  // Горячие клавиши для undo/redo (отдельный useEffect на уровне компонента)
  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if ((e.ctrlKey || e.metaKey) && e.key === 'z' && !e.shiftKey) {
        e.preventDefault();
        if (flowchartStore.canUndo()) {
          flowchartStore.undo();
          showToast('Изменение отменено');
          // Явно вызываем ререндер после undo
          setTimeout(() => {
            if (renderFlowchartFnRef.current) {
              renderFlowchartFnRef.current();
            }
          }, 0);
        }
      } else if ((e.ctrlKey || e.metaKey) && (e.key === 'y' || (e.key === 'z' && e.shiftKey))) {
        e.preventDefault();
        if (flowchartStore.canRedo()) {
          flowchartStore.redo();
          showToast('Изменение повторено');
          // Явно вызываем ререндер после redo
          setTimeout(() => {
            if (renderFlowchartFnRef.current) {
              renderFlowchartFnRef.current();
            }
          }, 0);
        }
      }
    };
    
    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, [flowchartStore]);

  // Zoom колесиком мыши (как в script.js) - отдельный useEffect на уровне компонента
  useEffect(() => {
    const wrapper = canvasWrapperRef.current;
    if (!wrapper) return;
    
    const handleWheel = (e: WheelEvent) => {
      // Проверяем, что зажат Ctrl или Cmd (для zoom)
      if (e.ctrlKey || e.metaKey) {
        e.preventDefault();
        const delta = e.deltaY > 0 ? -0.1 : 0.1;
        const newZoom = Math.max(0.1, Math.min(2, flowchartStore.zoom + delta));
        flowchartStore.setZoom(newZoom);
        const zoomValueEl = document.getElementById('zoom-value');
        if (zoomValueEl) zoomValueEl.textContent = Math.round(newZoom * 100).toString();
      }
    };
    
    wrapper.addEventListener('wheel', handleWheel, { passive: false });
    return () => wrapper.removeEventListener('wheel', handleWheel);
  }, [flowchartStore, zoom]);

  // Сохраняем renderFlowchart в ref для доступа из обработчиков
  useEffect(() => {
    renderFlowchartFnRef.current = renderFlowchart;
  }, [renderFlowchart]);

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
      
      // Создаем копию нод с обновленной позицией перетаскиваемой ноды
      // для корректного рендеринга соединений в реальном времени
      const nodesWithUpdatedPosition = currentNodes.map(node => 
        node.id === draggedNodeState.nodeId 
          ? { ...node, x: newPos.x, y: newPos.y }
          : node
      );
      
      // Обновляем соединения во время перетаскивания для плавного ререндеринга
      // Используем requestAnimationFrame для оптимизации
      requestAnimationFrame(() => {
        renderConnections(
          nodesWithUpdatedPosition,
          flowchartStore.store.getState().connections,
          connectionsCanvasRef.current!,
          connectionsSvgRef.current!,
          wrapper,
          {
            zoom: currentZoom,
            selectedConnectionId: flowchartStore.selectedConnectionId,
            onConnectionClick: (connectionId) => {
              flowchartStore.selectConnection(connectionId);
            }
          }
        );
      });
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
        const node = flowchartStore.nodes.find(n => n.id === draggedNodeState.nodeId);
        const oldX = node?.x || 0;
        const oldY = node?.y || 0;
        const newX = draggedNodePositionRef.current.x;
        const newY = draggedNodePositionRef.current.y;
        
        // Обновляем только если позиция действительно изменилась
        if (oldX !== newX || oldY !== newY) {
          flowchartStore.updateNode(draggedNodeState.nodeId, {
            x: newX,
            y: newY
          });
          
          // Добавляем в историю после обновления
          const updatedState = flowchartStore.store.getState();
          flowchartStore.addHistoryEntry(addHistoryEntry(
            `Перемещен блок "${node?.text || draggedNodeState.nodeId}"`,
            updatedState.nodes,
            updatedState.connections,
            updatedState.history
          )[updatedState.history.length]);
          
          // Рендерим соединения с актуальными данными из store
          // (renderFlowchartRef может содержать устаревшие данные)
          const wrapper = canvasWrapperRef.current;
          if (wrapper && connectionsCanvasRef.current && connectionsSvgRef.current) {
            renderConnections(
              updatedState.nodes,
              updatedState.connections,
              connectionsCanvasRef.current,
              connectionsSvgRef.current,
              wrapper,
              {
                zoom: updatedState.zoom,
                selectedConnectionId: flowchartStore.selectedConnectionId,
                onConnectionClick: (connectionId) => {
                  flowchartStore.selectConnection(connectionId);
                }
              }
            );
          }
        }
        
        draggedNodePositionRef.current = null;
      }
      
      stopDraggingNode(draggedNodeState.nodeId);
      setDraggedNodeState(null);
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
  const handleAddNode = useCallback((type: NodeType | 'function' | 'main') => {
    const options = type === 'function' ? { isFunction: true } : type === 'main' ? { isMainFunction: true } : undefined;
    const newNode = createNode(type, flowchartStore.nodes, 100, options);
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
    // Создаем обработчики один раз и сохраняем ссылки для cleanup
    const toolBtnHandlers: Map<HTMLElement, () => void> = new Map();
    
    // Кнопки добавления узлов
    document.querySelectorAll('.tool-btn').forEach(btn => {
      const type = (btn as HTMLElement).dataset.type;
      if (type) {
        const handler = () => handleAddNode(type as NodeType | 'function' | 'main');
        toolBtnHandlers.set(btn as HTMLElement, handler);
        btn.addEventListener('click', handler);
      }
    });

    // Обработчики для undo/redo
    const handleUndo = () => {
      const success = flowchartStore.undo();
      if (success) {
        showToast('Изменение отменено');
        const zoomValueEl = document.getElementById('zoom-value');
        if (zoomValueEl) zoomValueEl.textContent = Math.round(flowchartStore.zoom * 100).toString();
        
        // Восстанавливаем JSON из блок-схемы
        const restoredJSON = flowchartStore.getRestoredJSON();
        if (restoredJSON) {
          console.log('[UNDO] Восстановленный JSON:', restoredJSON);
          // TODO: Можно обновить sourceCode или показать восстановленный JSON
        }
        
        // Явно вызываем ререндер после undo
        setTimeout(() => {
          if (renderFlowchartFnRef.current) {
            renderFlowchartFnRef.current();
          }
        }, 0);
      } else {
        showToast('Нечего отменять', 'warning');
      }
    };

    const handleRedo = () => {
      const success = flowchartStore.redo();
      if (success) {
        showToast('Изменение повторено');
        const zoomValueEl = document.getElementById('zoom-value');
        if (zoomValueEl) zoomValueEl.textContent = Math.round(flowchartStore.zoom * 100).toString();
        
        // Восстанавливаем JSON из блок-схемы
        const restoredJSON = flowchartStore.getRestoredJSON();
        if (restoredJSON) {
          console.log('[REDO] Восстановленный JSON:', restoredJSON);
          // TODO: Можно обновить sourceCode или показать восстановленный JSON
        }
        
        // Явно вызываем ререндер после redo
        setTimeout(() => {
          if (renderFlowchartFnRef.current) {
            renderFlowchartFnRef.current();
          }
        }, 0);
      } else {
        showToast('Нечего повторять', 'warning');
      }
    };

    // Кнопки undo/redo
    const undoBtn = document.getElementById('undo-btn');
    const redoBtn = document.getElementById('redo-btn');
    
    if (undoBtn) {
      undoBtn.addEventListener('click', handleUndo);
    }
    
    if (redoBtn) {
      redoBtn.addEventListener('click', handleRedo);
    }

    // Обработчики для zoom
    const handleZoomIn = () => {
      flowchartStore.setZoom(zoomIn(flowchartStore.zoom));
      const zoomValue = document.getElementById('zoom-value');
      if (zoomValue) zoomValue.textContent = Math.round(flowchartStore.zoom * 100).toString();
    };

    const handleZoomOut = () => {
      flowchartStore.setZoom(zoomOut(flowchartStore.zoom));
      const zoomValue = document.getElementById('zoom-value');
      if (zoomValue) zoomValue.textContent = Math.round(flowchartStore.zoom * 100).toString();
    };

    const handleZoomReset = () => {
      flowchartStore.setZoom(1);
      const zoomValue = document.getElementById('zoom-value');
      if (zoomValue) zoomValue.textContent = '100';
    };

    // Кнопки зума
    const zoomInBtn = document.getElementById('zoom-in');
    const zoomOutBtn = document.getElementById('zoom-out');
    const zoomResetBtn = document.getElementById('zoom-reset');

    if (zoomInBtn) {
      zoomInBtn.addEventListener('click', handleZoomIn);
    }

    if (zoomOutBtn) {
      zoomOutBtn.addEventListener('click', handleZoomOut);
    }
    
    if (zoomResetBtn) {
      zoomResetBtn.addEventListener('click', handleZoomReset);
    }

    // Кнопка генерации блок-схемы
    const generateBtn = document.getElementById('generate-btn');
    if (generateBtn) {
      generateBtn.addEventListener('click', handleGenerateFlowchart);
    }

    // Обработчики для полей ввода узла (автоматическое сохранение при потере фокуса)
    const handleTextInputChange = () => {
      if (!flowchartStore.selectedNodeId) return;
      const textInput = document.getElementById('node-text-input') as HTMLTextAreaElement | null;
      if (textInput) {
        flowchartStore.updateNode(flowchartStore.selectedNodeId, { text: textInput.value });
        // renderFlowchart() вызовется автоматически через useEffect при изменении nodes
      }
    };

    const handleCodeInputChange = () => {
      if (!flowchartStore.selectedNodeId) return;
      const codeInput = document.getElementById('node-code-input') as HTMLTextAreaElement | null;
      if (codeInput && !codeInput.readOnly) { // Не обновляем, если поле только для чтения (для функций)
        flowchartStore.updateNode(flowchartStore.selectedNodeId, { codeReference: codeInput.value });
        // renderFlowchart() вызовется автоматически через useEffect при изменении nodes
      }
    };

    // Добавляем обработчики для автоматического сохранения при потере фокуса
    const textInput = document.getElementById('node-text-input') as HTMLTextAreaElement | null;
    const codeInput = document.getElementById('node-code-input') as HTMLTextAreaElement | null;

    if (textInput) {
      textInput.addEventListener('blur', handleTextInputChange);
      // Также можно обновлять при вводе с debounce, но для простоты используем blur
    }

    if (codeInput) {
      codeInput.addEventListener('blur', handleCodeInputChange);
    }

    // Кнопка сохранения узла (для явного сохранения, если нужно)
    const handleSaveNode = () => {
      if (!flowchartStore.selectedNodeId) return;

      const textInputEl = document.getElementById('node-text-input') as HTMLTextAreaElement | null;
      const codeInputEl = document.getElementById('node-code-input') as HTMLTextAreaElement | null;

      if (textInputEl) {
        flowchartStore.updateNode(flowchartStore.selectedNodeId, { text: textInputEl.value });
      }
      if (codeInputEl && !codeInputEl.readOnly) {
        flowchartStore.updateNode(flowchartStore.selectedNodeId, { codeReference: codeInputEl.value });
      }

      showToast('Изменения сохранены');
      // Явно вызываем ререндер после сохранения
      setTimeout(() => {
        if (renderFlowchartFnRef.current) {
          renderFlowchartFnRef.current();
        }
      }, 0);
    };

    const saveNodeBtn = document.getElementById('save-node-btn');
    if (saveNodeBtn) {
      saveNodeBtn.addEventListener('click', handleSaveNode);
    }

    // Экспорт
    const handleExportSvg = () => {
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
    };

    const handleExportPng = async () => {
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
    };

    const exportSvgBtn = document.getElementById('export-svg');
    const exportPngBtn = document.getElementById('export-png');
    
    if (exportSvgBtn) {
      exportSvgBtn.addEventListener('click', handleExportSvg);
    }
    
    if (exportPngBtn) {
      exportPngBtn.addEventListener('click', handleExportPng);
    }

    // Cleanup функция для удаления всех обработчиков
    return () => {
      // Удаляем обработчики с кнопок добавления узлов
      toolBtnHandlers.forEach((handler, btn) => {
        btn.removeEventListener('click', handler);
      });
      
      // Удаляем остальные обработчики
      if (undoBtn) undoBtn.removeEventListener('click', handleUndo);
      if (redoBtn) redoBtn.removeEventListener('click', handleRedo);
      if (zoomInBtn) zoomInBtn.removeEventListener('click', handleZoomIn);
      if (zoomOutBtn) zoomOutBtn.removeEventListener('click', handleZoomOut);
      if (zoomResetBtn) zoomResetBtn.removeEventListener('click', handleZoomReset);
      if (generateBtn) generateBtn.removeEventListener('click', handleGenerateFlowchart);
      if (saveNodeBtn) saveNodeBtn.removeEventListener('click', handleSaveNode);
      if (exportSvgBtn) exportSvgBtn.removeEventListener('click', handleExportSvg);
      if (exportPngBtn) exportPngBtn.removeEventListener('click', handleExportPng);
      
      // Удаляем обработчики полей ввода
      if (textInput) textInput.removeEventListener('blur', handleTextInputChange);
      if (codeInput) codeInput.removeEventListener('blur', handleCodeInputChange);
    };
  }, [handleAddNode, flowchartStore, handleGenerateFlowchart]);

  // Инициализация обработчиков для загрузки примера, файла, комментариев и т.д.
  useEffect(() => {
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
  }, [flowchartStore]);

  // Обработчик для кнопки "Экспорт в код"
  useEffect(() => {
    const exportCodeBtn = document.getElementById('export-code-btn');
    const exportCodeEditor = document.getElementById('export-code-editor') as HTMLTextAreaElement | null;

    const handleExportCode = async () => {
      if (!exportCodeEditor) return;

      try {
        // Показываем индикатор загрузки
        exportCodeBtn?.setAttribute('disabled', 'true');
        exportCodeEditor.value = 'Генерация кода...';
        exportCodeEditor.style.color = 'var(--text-secondary)';

        // Восстанавливаем JSON из блок-схемы
        const restoredJSON = flowchartStore.getRestoredJSON();
        if (!restoredJSON) {
          throw new Error('Не удалось восстановить JSON из блок-схемы. Убедитесь, что блок-схема была создана из кода.');
        }

        // Определяем язык из metadata первого узла
        const language = flowchartStore.nodes.find(n => n.metadata?.language)?.metadata?.language || 'pascal';
        
        console.log('[EXPORT] Восстановленный JSON:', restoredJSON);
        console.log('[EXPORT] Язык:', language);

        // Вызываем API для генерации кода
        const result = await api.generateCode(restoredJSON, language);
        
        if (result.success && result.code) {
          exportCodeEditor.value = result.code;
          exportCodeEditor.style.color = 'var(--text-primary)';
          showToast('Код успешно сгенерирован', 'success');
        } else {
          throw new Error(result.error || 'Не удалось сгенерировать код');
        }
      } catch (error) {
        console.error('[EXPORT] Ошибка генерации кода:', error);
        const errorMessage = error instanceof Error ? error.message : 'Неизвестная ошибка';
        exportCodeEditor.value = `Ошибка: ${errorMessage}`;
        exportCodeEditor.style.color = 'var(--error, #ef4444)';
        showToast('Ошибка генерации кода', 'error');
      } finally {
        exportCodeBtn?.removeAttribute('disabled');
      }
    };

    if (exportCodeBtn) {
      exportCodeBtn.addEventListener('click', handleExportCode);
    }

    return () => {
      if (exportCodeBtn) {
        exportCodeBtn.removeEventListener('click', handleExportCode);
      }
    };
  }, [flowchartStore]);


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
      {/* Уведомление о парсере */}
      {notification && (
        <div className={`parser-notification ${notification.type}`}>
          <div className="notification-content">
            {notification.type === 'error' && (
              <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                <circle cx="12" cy="12" r="10"></circle>
                <line x1="15" y1="9" x2="9" y2="15"></line>
                <line x1="9" y1="9" x2="15" y2="15"></line>
              </svg>
            )}
            {notification.type === 'warning' && (
              <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                <path d="M10.29 3.86L1.82 18a2 2 0 0 0 1.71 3h16.94a2 2 0 0 0 1.71-3L13.71 3.86a2 2 0 0 0-3.42 0z"></path>
                <line x1="12" y1="9" x2="12" y2="13"></line>
                <line x1="12" y1="17" x2="12.01" y2="17"></line>
              </svg>
            )}
            {notification.type === 'success' && (
              <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                <path d="M22 11.08V12a10 10 0 1 1-5.93-9.14"></path>
                <polyline points="22 4 12 14.01 9 11.01"></polyline>
              </svg>
            )}
            <span>{notification.message}</span>
          </div>
          <button className="notification-close" onClick={dismissNotification}>
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
              <line x1="18" y1="6" x2="6" y2="18"></line>
              <line x1="6" y1="6" x2="18" y2="18"></line>
            </svg>
          </button>
        </div>
      )}
      
      {/* Индикатор загрузки файла */}
      {isLoadingFile && (
        <div className="loading-overlay">
          <div className="loading-spinner"></div>
          <span>Загрузка файла и генерация диаграммы...</span>
        </div>
      )}
      
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
                <path d="M3 9l3-3 3 3M9 7v10M21 15l-3 3-3-3M15 17V7"></path>
                <path d="M12 3v18"></path>
              </svg>
              <span>Ввод/Вывод</span>
            </button>
            <button className="tool-btn" data-type="function" title="Функция/Процедура">
              <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                <path d="M8 2v4M16 2v4M3 10h18M5 4h14a2 2 0 0 1 2 2v14a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V6a2 2 0 0 1 2-2z"></path>
                <path d="M12 14v4M10 16h4"></path>
              </svg>
              <span>Функция/Процедура</span>
            </button>
            <button className="tool-btn" data-type="main" title="Main функция (только для C/C++)">
              <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                <rect x="3" y="3" width="18" height="18" rx="2"></rect>
              </svg>
              <span>Main (C/C++)</span>
            </button>
          </div>
        </div>

        <div className="toolbar-section">
          <h3>История</h3>
          <button className="control-btn" id="undo-btn" title="Undo (Ctrl+Z)">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
              <path d="M3 7v6h6M21 17a10 10 0 0 0-10-10 8 8 0 0 0-8 8 8 8 0 0 0 8 8c4.478 0 8.22-2.736 9.65-6.6"></path>
            </svg>
            Undo
          </button>
          <button className="control-btn" id="redo-btn" title="Redo (Ctrl+Y)">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
              <path d="M21 7v6h-6M3 17a10 10 0 0 1 10-10 8 8 0 0 1 8 8 8 8 0 0 1-8 8c-4.478 0-8.22-2.736-9.65-6.6"></path>
            </svg>
            Redo
          </button>
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
          <button className="control-btn" id="zoom-reset">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
              <path d="M3 12a9 9 0 0 1 9-9 9.75 9.75 0 0 1 6.74 2.74L21 8M3 12a9 9 0 0 0 9 9 9.75 9.75 0 0 0 6.74-2.74L21 16M21 12H3"></path>
            </svg>
            Сброс
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
          <h1>
            Редактор блок-схем
            {fileName && <span className="file-name-indicator"> - {fileName}</span>}
          </h1>
          <div className="zoom-indicator">
            Масштаб: <span id="zoom-value">100</span>%
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
            История
          </button>
          <button className="tab-btn" data-tab="export">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
              <path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4M17 8l-5-5-5 5M12 3v12"></path>
            </svg>
            Экспорт в код
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
            <div className="form-group" id="function-prototype-toggle-group" style={{ display: 'none' }}>
              <label htmlFor="function-prototype-toggle">
                <input type="checkbox" id="function-prototype-toggle" />
                <span style={{ marginLeft: '0.5rem' }}>Прототип функции</span>
              </label>
              <small style={{ display: 'block', marginTop: '0.25rem', color: 'var(--text-secondary)', fontSize: '0.75rem' }}>
                Прототип не имеет тела функции
              </small>
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
        
        <div className="tab-content" id="tab-export">
          <h3>Экспорт в код</h3>
          <p className="tab-description">Восстановите исходный код из текущей блок-схемы</p>
          <div className="form-group">
            <label htmlFor="export-code-editor">Сгенерированный код:</label>
            <textarea 
              id="export-code-editor" 
              rows={15} 
              placeholder="Нажмите 'Экспорт в код' для генерации..."
              readOnly
              style={{ fontFamily: 'Courier New, monospace', fontSize: '0.8125rem' }}
            ></textarea>
          </div>
          <button className="primary-btn" id="export-code-btn">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
              <path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4M17 8l-5-5-5 5M12 3v12"></path>
            </svg>
            Экспорт в код
          </button>
        </div>
      </aside>
      
      {/* Toast Container */}
      <div id="toast-container" className="toast-container"></div>
    </div>
  );
}

export default FlowchartEditorRefactored;

