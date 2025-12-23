// Prevent duplicate loading
if (window._appJsLoaded) {
    console.log('app.js already loaded, skipping...');
} else {
window._appJsLoaded = true;

// Application State
const state = {
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

// Utility Functions
function showToast(message, type = 'success') {
    const container = document.getElementById('toast-container');
    if (!container) {
        // Create toast container if it doesn't exist
        const toastContainer = document.createElement('div');
        toastContainer.id = 'toast-container';
        toastContainer.className = 'toast-container';
        document.body.appendChild(toastContainer);
    }
    const toast = document.createElement('div');
    toast.className = `toast ${type}`;
    
    const icon = type === 'success' 
        ? '<svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M22 11.08V12a10 10 0 1 1-5.93-9.14"></path><polyline points="22 4 12 14.01 9 11.01"></polyline></svg>'
        : '<svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"></circle><line x1="15" y1="9" x2="9" y2="15"></line><line x1="9" y1="9" x2="15" y2="15"></line></svg>';
    
    toast.innerHTML = `
        <div class="toast-icon">${icon}</div>
        <div class="toast-message">${message}</div>
    `;
    
    document.getElementById('toast-container').appendChild(toast);
    
    setTimeout(() => {
        toast.style.animation = 'slideIn 0.3s ease-out reverse';
        setTimeout(() => toast.remove(), 300);
    }, 3000);
}

function addToHistory(description) {
    const entry = {
        id: `h${Date.now()}`,
        timestamp: new Date(),
        description,
        nodes: JSON.parse(JSON.stringify(state.nodes)),
        connections: JSON.parse(JSON.stringify(state.connections))
    };
    state.history.push(entry);
    renderHistory();
}

function getDefaultText(type) {
    const texts = {
        start: 'Начало',
        end: 'Конец',
        process: 'Процесс',
        decision: 'Условие',
        input: 'Ввод',
        output: 'Вывод'
    };
    return texts[type] || 'Блок';
}

function getTypeLabel(type) {
    const labels = {
        start: 'Начало',
        end: 'Конец',
        process: 'Процесс',
        decision: 'Условие',
        input: 'Ввод',
        output: 'Вывод'
    };
    return labels[type] || 'Неизвестно';
}

// Get port position
function getPortPosition(node, port) {
    const positions = {
        top: { x: node.x + node.width / 2, y: node.y },
        right: { x: node.x + node.width, y: node.y + node.height / 2 },
        bottom: { x: node.x + node.width / 2, y: node.y + node.height },
        left: { x: node.x, y: node.y + node.height / 2 }
    };
    return positions[port] || positions.bottom;
}

// Canvas Drawing
function drawConnections() {
    const canvas = document.getElementById('connections-canvas');
    const svg = document.getElementById('connections-svg');
    const wrapper = document.getElementById('canvas-wrapper');
    
    if (!canvas || !svg || !wrapper) return;
    
    canvas.width = wrapper.scrollWidth;
    canvas.height = wrapper.scrollHeight;
    
    // SVG should match canvas dimensions - scale coordinates manually (more reliable)
    const svgWidth = wrapper.scrollWidth;
    const svgHeight = wrapper.scrollHeight;
    svg.setAttribute('width', svgWidth);
    svg.setAttribute('height', svgHeight);
    // Remove viewBox - we'll scale coordinates manually to match block positions exactly
    svg.removeAttribute('viewBox');
    svg.removeAttribute('preserveAspectRatio');
    
    const ctx = canvas.getContext('2d');
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    
    // Clear SVG
    svg.innerHTML = '';
    
    // Add arrow marker definition first - scale by zoom
    const defs = document.createElementNS('http://www.w3.org/2000/svg', 'defs');
    const marker = document.createElementNS('http://www.w3.org/2000/svg', 'marker');
    const arrowSize = 10 * state.zoom;
    marker.setAttribute('id', 'arrowhead');
    marker.setAttribute('markerWidth', arrowSize.toString());
    marker.setAttribute('markerHeight', arrowSize.toString());
    marker.setAttribute('refX', (arrowSize * 0.9).toString());
    marker.setAttribute('refY', (arrowSize * 0.3).toString());
    marker.setAttribute('orient', 'auto');
    marker.setAttribute('markerUnits', 'userSpaceOnUse');
    const polygon = document.createElementNS('http://www.w3.org/2000/svg', 'polygon');
    polygon.setAttribute('points', `0 0, ${arrowSize} ${arrowSize * 0.3}, 0 ${arrowSize * 0.6}`);
    polygon.setAttribute('fill', state.selectedConnectionId ? '#3b82f6' : '#64748b');
    marker.appendChild(polygon);
    defs.appendChild(marker);
    svg.appendChild(defs);
    
    ctx.save();
    ctx.scale(state.zoom, state.zoom);
    
    // Draw connections as SVG for interactivity
    state.connections.forEach(conn => {
        const fromNode = state.nodes.find(n => n.id === conn.from);
        const toNode = state.nodes.find(n => n.id === conn.to);
        
        if (fromNode && toNode) {
            const fromPort = getPortPosition(fromNode, conn.fromPort || 'bottom');
            const toPort = getPortPosition(toNode, conn.toPort || 'top');
            
            const fromX = fromPort.x;
            const fromY = fromPort.y;
            const toX = toPort.x;
            const toY = toPort.y;
            
            // Calculate control points for bezier curve
            const dx = Math.abs(toX - fromX);
            const dy = Math.abs(toY - fromY);
            const controlOffset = Math.min(dx, dy) * 0.5;
            
            let cp1x = fromX, cp1y = fromY;
            let cp2x = toX, cp2y = toY;
            
            if (conn.fromPort === 'bottom') {
                cp1y = fromY + controlOffset;
            } else if (conn.fromPort === 'top') {
                cp1y = fromY - controlOffset;
            } else if (conn.fromPort === 'right') {
                cp1x = fromX + controlOffset;
            } else if (conn.fromPort === 'left') {
                cp1x = fromX - controlOffset;
            }
            
            if (conn.toPort === 'top') {
                cp2y = toY - controlOffset;
            } else if (conn.toPort === 'bottom') {
                cp2y = toY + controlOffset;
            } else if (conn.toPort === 'right') {
                cp2x = toX + controlOffset;
            } else if (conn.toPort === 'left') {
                cp2x = toX - controlOffset;
            }
            
            // Create SVG path - scale coordinates manually to match block positions exactly
            const path = document.createElementNS('http://www.w3.org/2000/svg', 'path');
            // Scale all coordinates by zoom to match the actual pixel positions of blocks
            const scaledFromX = fromX * state.zoom;
            const scaledFromY = fromY * state.zoom;
            const scaledToX = toX * state.zoom;
            const scaledToY = toY * state.zoom;
            const scaledCp1x = cp1x * state.zoom;
            const scaledCp1y = cp1y * state.zoom;
            const scaledCp2x = cp2x * state.zoom;
            const scaledCp2y = cp2y * state.zoom;
            const d = `M ${scaledFromX} ${scaledFromY} C ${scaledCp1x} ${scaledCp1y}, ${scaledCp2x} ${scaledCp2y}, ${scaledToX} ${scaledToY}`;
            path.setAttribute('d', d);
            path.setAttribute('class', `connection-line ${state.selectedConnectionId === conn.id ? 'selected' : ''}`);
            path.setAttribute('data-connection-id', conn.id);
            path.setAttribute('fill', 'none');
            const strokeColor = state.selectedConnectionId === conn.id ? '#3b82f6' : '#64748b';
            path.setAttribute('stroke', strokeColor);
            path.setAttribute('stroke-width', (3 * state.zoom).toString());
            path.setAttribute('marker-end', 'url(#arrowhead)');
            path.style.cursor = 'pointer';
            path.style.pointerEvents = 'stroke';
            
            path.addEventListener('click', (e) => {
                e.stopPropagation();
                selectConnection(conn.id);
            });
            
            svg.appendChild(path);
        }
    });
    
    // Draw temporary connection line
    if (state.connectingFrom) {
        const fromNode = state.nodes.find(n => n.id === state.connectingFrom);
        if (fromNode && state.mousePos) {
            const fromPort = getPortPosition(fromNode, state.connectingFromPort || 'bottom');
            const mouseX = state.mousePos.x / state.zoom;
            const mouseY = state.mousePos.y / state.zoom;
            
            ctx.strokeStyle = '#3b82f6';
            ctx.lineWidth = 2;
            ctx.setLineDash([5, 5]);
            ctx.beginPath();
            ctx.moveTo(fromPort.x, fromPort.y);
            
            // Use bezier curve for temporary line too
            const dx = Math.abs(mouseX - fromPort.x);
            const dy = Math.abs(mouseY - fromPort.y);
            const controlOffset = Math.min(dx, dy) * 0.5;
            
            let cp1x = fromPort.x, cp1y = fromPort.y;
            if (state.connectingFromPort === 'bottom') {
                cp1y = fromPort.y + controlOffset;
            } else if (state.connectingFromPort === 'top') {
                cp1y = fromPort.y - controlOffset;
            } else if (state.connectingFromPort === 'right') {
                cp1x = fromPort.x + controlOffset;
            } else if (state.connectingFromPort === 'left') {
                cp1x = fromPort.x - controlOffset;
            }
            
            ctx.bezierCurveTo(cp1x, cp1y, mouseX, mouseY, mouseX, mouseY);
            ctx.stroke();
            ctx.setLineDash([]);
        }
    }
    
    ctx.restore();
}

// Render Functions
function renderNodes() {
    const canvas = document.getElementById('flowchart-canvas');
    if (!canvas) return;
    
    canvas.innerHTML = '';
    
    state.nodes.forEach(node => {
        const nodeEl = document.createElement('div');
        nodeEl.className = `flowchart-node node-${node.type}`;
        if (state.selectedNodeId === node.id) {
            nodeEl.classList.add('selected');
        }
        if (state.connectingFrom) {
            nodeEl.classList.add('connecting');
        }
        
        nodeEl.style.left = `${node.x}px`;
        nodeEl.style.top = `${node.y}px`;
        nodeEl.style.width = `${node.width}px`;
        nodeEl.style.height = `${node.height}px`;
        nodeEl.dataset.id = node.id;
        
        nodeEl.innerHTML = `<div class="node-text">${node.text}</div>`;
        
        // Add connection points
        const ports = ['top', 'right', 'bottom', 'left'];
        ports.forEach(port => {
            const point = document.createElement('div');
            point.className = 'node-connection-point';
            point.dataset.port = port;
            point.dataset.nodeId = node.id;
            
            const positions = {
                top: { left: '50%', top: '0' },
                right: { left: '100%', top: '50%' },
                bottom: { left: '50%', top: '100%' },
                left: { left: '0', top: '50%' }
            };
            
            const pos = positions[port];
            point.style.left = pos.left;
            point.style.top = pos.top;
            
            if (state.connectingFrom === node.id && state.connectingFromPort === port) {
                point.classList.add('connecting');
            }
            
            point.addEventListener('click', (e) => {
                e.stopPropagation();
                if (state.connectingFrom) {
                    // Complete connection to this port
                    handleCompleteConnection(node.id, port);
                } else {
                    // Start connection from this port
                    startConnectionFromPort(node.id, port);
                }
            });
            
            nodeEl.appendChild(point);
        });
        
        // Event listeners
        nodeEl.addEventListener('click', (e) => {
            // Don't trigger if clicking on connection point
            if (e.target.classList.contains('node-connection-point')) {
                return;
            }
            e.stopPropagation();
            if (state.connectingFrom) {
                // If clicking on a connection point, use that port
                const clickedPoint = e.target.closest('.node-connection-point');
                if (clickedPoint) {
                    handleCompleteConnection(node.id, clickedPoint.dataset.port);
                } else {
                    // Determine best port based on click position
                    const rect = nodeEl.getBoundingClientRect();
                    const wrapper = document.getElementById('canvas-wrapper');
                    const wrapperRect = wrapper.getBoundingClientRect();
                    const clickX = (e.clientX - wrapperRect.left + wrapper.scrollLeft) / state.zoom;
                    const clickY = (e.clientY - wrapperRect.top + wrapper.scrollTop) / state.zoom;
                    
                    const nodeCenterX = node.x + node.width / 2;
                    const nodeCenterY = node.y + node.height / 2;
                    
                    const dx = clickX - nodeCenterX;
                    const dy = clickY - nodeCenterY;
                    
                    let toPort = 'top';
                    if (Math.abs(dy) > Math.abs(dx)) {
                        toPort = dy > 0 ? 'bottom' : 'top';
                    } else {
                        toPort = dx > 0 ? 'right' : 'left';
                    }
                    
                    handleCompleteConnection(node.id, toPort);
                }
            } else {
                selectNode(node.id);
            }
        });
        
        nodeEl.addEventListener('dblclick', (e) => {
            e.stopPropagation();
            startEditingNode(node.id);
        });
        
        nodeEl.addEventListener('mousedown', (e) => {
            if (e.target.tagName === 'TEXTAREA' || e.target.classList.contains('node-connection-point')) return;
            startDragging(node.id, e);
        });
        
        canvas.appendChild(nodeEl);
        
        // Render controls for selected node
        if (state.selectedNodeId === node.id) {
            renderNodeControls(node);
        }
    });
    
    drawConnections();
}

function renderNodeControls(node) {
    const existingControls = document.querySelector('.node-controls');
    if (existingControls) existingControls.remove();
    
    const canvas = document.getElementById('flowchart-canvas');
    if (!canvas) return;
    
    const controls = document.createElement('div');
    controls.className = 'node-controls';
    controls.style.left = `${node.x + node.width + 10}px`;
    controls.style.top = `${node.y}px`;
    
    controls.innerHTML = `
        <button class="node-control-btn" data-action="connect" title="Создать связь">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <path d="M10 13a5 5 0 0 0 7.54.54l3-3a5 5 0 0 0-7.07-7.07l-1.72 1.71"></path>
                <path d="M14 11a5 5 0 0 0-7.54-.54l-3 3a5 5 0 0 0 7.07 7.07l1.71-1.71"></path>
            </svg>
        </button>
        <button class="node-control-btn delete" data-action="delete" title="Удалить">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <path d="M3 6h18M19 6v14c0 1-1 2-2 2H7c-1 0-2-1-2-2V6M8 6V4c0-1 1-2 2-2h4c1 0 2 1 2 2v2"></path>
            </svg>
        </button>
        ${node.comments.length > 0 ? `
            <div class="node-comment-badge">
                <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                    <path d="M21 15a2 2 0 0 1-2 2H7l-4 4V5a2 2 0 0 1 2-2h14a2 2 0 0 1 2 2z"></path>
                </svg>
                ${node.comments.length}
            </div>
        ` : ''}
    `;
    
    controls.addEventListener('click', (e) => {
        e.stopPropagation();
        const action = e.target.closest('[data-action]')?.dataset.action;
        if (action === 'connect') {
            startConnection(node.id);
        } else if (action === 'delete') {
            deleteNode(node.id);
        }
    });
    
    canvas.appendChild(controls);
}

function renderInfoPanel() {
    const emptyState = document.getElementById('info-empty');
    const infoPanel = document.getElementById('info-panel');
    
    if (!emptyState || !infoPanel) {
        console.warn('Info panel elements not found');
        return;
    }
    
    if (!state.selectedNodeId) {
        emptyState.style.display = 'flex';
        infoPanel.style.display = 'none';
        return;
    }
    
    const node = state.nodes.find(n => n.id === state.selectedNodeId);
    if (!node) {
        emptyState.style.display = 'flex';
        infoPanel.style.display = 'none';
        return;
    }
    
    emptyState.style.display = 'none';
    infoPanel.style.display = 'flex';
    
    const badge = document.getElementById('node-type-badge');
    if (badge) {
        badge.textContent = getTypeLabel(node.type);
        badge.className = `node-type-badge badge-${node.type}`;
    }
    
    const idInput = document.getElementById('node-id-input');
    const textInput = document.getElementById('node-text-input');
    const xInput = document.getElementById('node-x-input');
    const yInput = document.getElementById('node-y-input');
    const codeInput = document.getElementById('node-code-input');
    
    if (idInput) idInput.value = node.id || '';
    if (textInput) textInput.value = node.text || '';
    if (xInput) xInput.value = Math.round(node.x || 0);
    if (yInput) yInput.value = Math.round(node.y || 0);
    if (codeInput) codeInput.value = node.codeReference || '';
}

function renderComments() {
    const emptyState = document.getElementById('comments-empty');
    const commentsPanel = document.getElementById('comments-panel');
    
    if (!emptyState || !commentsPanel) return;
    
    if (!state.selectedNodeId) {
        emptyState.style.display = 'flex';
        commentsPanel.style.display = 'none';
        return;
    }
    
    const node = state.nodes.find(n => n.id === state.selectedNodeId);
    if (!node) {
        emptyState.style.display = 'flex';
        commentsPanel.style.display = 'none';
        return;
    }
    
    emptyState.style.display = 'none';
    commentsPanel.style.display = 'flex';
    
    const nodeName = document.getElementById('comment-node-name');
    if (nodeName) nodeName.textContent = node.text;
    
    const commentsList = document.getElementById('comments-list');
    if (!commentsList) return;
    
    if (node.comments.length === 0) {
        commentsList.innerHTML = '<div class="empty-state"><p style="font-size: 0.875rem;">Комментариев пока нет<br><small>Добавьте первый комментарий ниже</small></p></div>';
    } else {
        commentsList.innerHTML = node.comments.map(comment => `
            <div class="comment-item">
                <div class="comment-header">
                    <div class="comment-author">${comment.author}</div>
                    <div class="comment-date">${formatDate(comment.timestamp)}</div>
                </div>
                <div class="comment-text">${comment.text}</div>
            </div>
        `).join('');
    }
}

function renderHistory() {
    const historyList = document.getElementById('history-list');
    const historyCount = document.getElementById('history-count');
    
    if (historyCount) historyCount.textContent = state.history.length;
    if (!historyList) return;
    
    if (state.history.length === 0) {
        historyList.innerHTML = '<div class="empty-state"><p>История изменений пуста</p></div>';
        return;
    }
    
    historyList.innerHTML = [...state.history].reverse().map((entry, index) => `
        <div class="history-item">
            <div class="history-header">
                <div class="history-description">${entry.description}</div>
                ${index > 0 ? `
                    <button class="history-restore-btn" data-id="${entry.id}">
                        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                            <path d="M3 12a9 9 0 1 0 9-9 9.75 9.75 0 0 0-6.74 2.74L3 8"></path>
                            <path d="M3 3v5h5M12 7v5l4 2"></path>
                        </svg>
                    </button>
                ` : ''}
            </div>
            <div class="history-timestamp">
                ${formatDate(entry.timestamp)}<br>
                ${getTimeAgo(entry.timestamp)}
            </div>
            ${entry.nodes.length > 0 ? `
                <div class="history-stats">
                    Блоков: ${entry.nodes.length} | Связей: ${entry.connections.length}
                </div>
            ` : ''}
        </div>
    `).join('');
    
    // Add event listeners
    historyList.querySelectorAll('.history-restore-btn').forEach(btn => {
        btn.addEventListener('click', () => {
            const entry = state.history.find(e => e.id === btn.dataset.id);
            if (entry) restoreHistory(entry);
        });
    });
}

// Node Operations
function addNode(type) {
    const node = {
        id: `node-${Date.now()}`,
        type,
        x: 400,
        y: 100 + state.nodes.length * 150,
        width: type === 'decision' ? 180 : (type === 'start' || type === 'end') ? 120 : 180,
        height: type === 'decision' ? 100 : (type === 'start' || type === 'end') ? 60 : 80,
        text: getDefaultText(type),
        codeReference: '',
        comments: []
    };
    
    state.nodes.push(node);
    addToHistory(`Добавлен блок: ${getDefaultText(type)}`);
    renderNodes();
    showToast('Блок добавлен');
}

function deleteNode(nodeId) {
    const node = state.nodes.find(n => n.id === nodeId);
    state.nodes = state.nodes.filter(n => n.id !== nodeId);
    state.connections = state.connections.filter(c => c.from !== nodeId && c.to !== nodeId);
    
    if (state.selectedNodeId === nodeId) {
        state.selectedNodeId = null;
    }
    
    addToHistory(`Удален блок: ${node?.text || 'Без названия'}`);
    renderNodes();
    renderInfoPanel();
    renderComments();
    renderConnectionControls(null);
    showToast('Блок удален');
}

function selectNode(nodeId) {
    state.selectedNodeId = nodeId;
    state.selectedConnectionId = null;
    renderNodes();
    renderInfoPanel();
    renderComments();
    renderConnectionControls(null);
}

function startEditingNode(nodeId) {
    const nodeEl = document.querySelector(`[data-id="${nodeId}"]`);
    if (!nodeEl) {
        console.warn('Node element not found for editing:', nodeId);
        return;
    }
    
    const textEl = nodeEl.querySelector('.node-text');
    if (!textEl) {
        console.warn('Text element not found in node:', nodeId);
        return;
    }
    
    const node = state.nodes.find(n => n.id === nodeId);
    if (!node) {
        console.warn('Node not found in state:', nodeId);
        return;
    }
    
    const textarea = document.createElement('textarea');
    textarea.className = 'node-edit';
    textarea.value = node.text || '';
    
    textEl.replaceWith(textarea);
    textarea.focus();
    textarea.select();
    
    const finishEdit = () => {
        if (!textarea.parentNode) return; // Already replaced
        
        node.text = textarea.value;
        const newTextEl = document.createElement('div');
        newTextEl.className = 'node-text';
        newTextEl.textContent = node.text;
        textarea.replaceWith(newTextEl);
        renderInfoPanel();
        renderNodes(); // Re-render to update display
    };
    
    const handleBlur = () => {
        finishEdit();
        textarea.removeEventListener('blur', handleBlur);
    };
    
    textarea.addEventListener('blur', handleBlur);
    textarea.addEventListener('keydown', (e) => {
        if (e.key === 'Enter' && !e.shiftKey) {
            e.preventDefault();
            finishEdit();
        } else if (e.key === 'Escape') {
            e.preventDefault();
            const newTextEl = document.createElement('div');
            newTextEl.className = 'node-text';
            newTextEl.textContent = node.text;
            textarea.replaceWith(newTextEl);
        }
    });
}

function updateNode() {
    if (!state.selectedNodeId) return;
    
    const node = state.nodes.find(n => n.id === state.selectedNodeId);
    if (!node) return;
    
    const textInput = document.getElementById('node-text-input');
    const codeInput = document.getElementById('node-code-input');
    
    if (textInput) node.text = textInput.value;
    if (codeInput) node.codeReference = codeInput.value;
    
    renderNodes();
    showToast('Изменения сохранены');
}

// Drag and Drop
function startDragging(nodeId, e) {
    const node = state.nodes.find(n => n.id === nodeId);
    const nodeEl = document.querySelector(`[data-id="${nodeId}"]`);
    
    if (!node || !nodeEl) return;
    
    state.draggedNode = nodeId;
    state.dragOffset = {
        x: e.clientX - node.x * state.zoom,
        y: e.clientY - node.y * state.zoom
    };
    
    nodeEl.classList.add('dragging');
    e.preventDefault();
}

function handleMouseMove(e) {
    if (state.draggedNode) {
        const node = state.nodes.find(n => n.id === state.draggedNode);
        if (node) {
            node.x = (e.clientX - state.dragOffset.x) / state.zoom;
            node.y = (e.clientY - state.dragOffset.y) / state.zoom;
            renderNodes();
        }
    }
    
    // Track mouse position for connection line
    const wrapper = document.getElementById('canvas-wrapper');
    if (wrapper) {
        const rect = wrapper.getBoundingClientRect();
        state.mousePos = {
            x: e.clientX - rect.left + wrapper.scrollLeft,
            y: e.clientY - rect.top + wrapper.scrollTop
        };
    }
    
    if (state.connectingFrom) {
        drawConnections();
    }
}

function handleMouseUp() {
    if (state.draggedNode) {
        const nodeEl = document.querySelector(`[data-id="${state.draggedNode}"]`);
        if (nodeEl) nodeEl.classList.remove('dragging');
        state.draggedNode = null;
        renderInfoPanel();
    }
}

// Connections
function startConnection(nodeId) {
    // Legacy function - start from bottom port by default
    startConnectionFromPort(nodeId, 'bottom');
}

function startConnectionFromPort(nodeId, port) {
    state.connectingFrom = nodeId;
    state.connectingFromPort = port;
    state.selectedNodeId = null;
    state.selectedConnectionId = null;
    renderNodes();
    showToast('Выберите точку подключения на другом блоке', 'success');
}

function handleCompleteConnection(toNodeId, toPort = null) {
    if (!state.connectingFrom || state.connectingFrom === toNodeId) {
        state.connectingFrom = null;
        state.connectingFromPort = null;
        renderNodes();
        return;
    }
    
    // If toPort not specified, try to determine best port
    if (!toPort) {
        const fromNode = state.nodes.find(n => n.id === state.connectingFrom);
        const toNode = state.nodes.find(n => n.id === toNodeId);
        
        if (fromNode && toNode) {
            const fromPortPos = getPortPosition(fromNode, state.connectingFromPort);
            const toNodeCenter = { x: toNode.x + toNode.width / 2, y: toNode.y + toNode.height / 2 };
            
            // Determine best port based on relative position
            const dx = toNodeCenter.x - fromPortPos.x;
            const dy = toNodeCenter.y - fromPortPos.y;
            
            if (Math.abs(dy) > Math.abs(dx)) {
                toPort = dy > 0 ? 'top' : 'bottom';
            } else {
                toPort = dx > 0 ? 'left' : 'right';
            }
        } else {
            toPort = 'top';
        }
    }
    
    const connection = {
        id: `conn-${Date.now()}`,
        from: state.connectingFrom,
        to: toNodeId,
        fromPort: state.connectingFromPort,
        toPort: toPort
    };
    
    state.connections.push(connection);
    state.connectingFrom = null;
    state.connectingFromPort = null;
    
    addToHistory('Добавлена связь между блоками');
    renderNodes();
    showToast('Связь добавлена');
}

function selectConnection(connectionId) {
    state.selectedConnectionId = connectionId;
    state.selectedNodeId = null;
    renderNodes();
    renderConnectionControls(connectionId);
}

function deleteConnection(connectionId) {
    const connection = state.connections.find(c => c.id === connectionId);
    state.connections = state.connections.filter(c => c.id !== connectionId);
    
    if (state.selectedConnectionId === connectionId) {
        state.selectedConnectionId = null;
    }
    
    addToHistory('Удалена связь между блоками');
    renderNodes();
    renderConnectionControls(null);
    showToast('Связь удалена');
}

function renderConnectionControls(connectionId) {
    const existingControls = document.querySelector('.connection-controls');
    if (existingControls) existingControls.remove();
    
    if (!connectionId) return;
    
    const connection = state.connections.find(c => c.id === connectionId);
    if (!connection) return;
    
    const fromNode = state.nodes.find(n => n.id === connection.from);
    const toNode = state.nodes.find(n => n.id === connection.to);
    
    if (!fromNode || !toNode) return;
    
    const fromPort = getPortPosition(fromNode, connection.fromPort);
    const toPort = getPortPosition(toNode, connection.toPort);
    const midX = (fromPort.x + toPort.x) / 2;
    const midY = (fromPort.y + toPort.y) / 2;
    
    const canvas = document.getElementById('flowchart-canvas');
    if (!canvas) return;
    
    const controls = document.createElement('div');
    controls.className = 'connection-controls';
    controls.style.left = `${midX + 20}px`;
    controls.style.top = `${midY - 20}px`;
    
    controls.innerHTML = `
        <button class="node-control-btn delete" data-action="delete-connection" title="Удалить связь">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <path d="M3 6h18M19 6v14c0 1-1 2-2 2H7c-1 0-2-1-2-2V6M8 6V4c0-1 1-2 2-2h4c1 0 2 1 2 2v2"></path>
            </svg>
        </button>
    `;
    
    controls.addEventListener('click', (e) => {
        e.stopPropagation();
        const action = e.target.closest('[data-action]')?.dataset.action;
        if (action === 'delete-connection') {
            deleteConnection(connectionId);
        }
    });
    
    canvas.appendChild(controls);
}

// Comments
function addComment() {
    if (!state.selectedNodeId) return;
    
    const commentInput = document.getElementById('new-comment');
    if (!commentInput) return;
    
    const commentText = commentInput.value.trim();
    if (!commentText) return;
    
    const node = state.nodes.find(n => n.id === state.selectedNodeId);
    if (!node) return;
    
    const comment = {
        id: `comment-${Date.now()}`,
        author: 'Пользователь',
        text: commentText,
        timestamp: new Date()
    };
    
    node.comments.push(comment);
    commentInput.value = '';
    
    addToHistory('Добавлен комментарий');
    renderComments();
    renderNodes();
    showToast('Комментарий добавлен');
}

// Code Generation
function parseCodeToFlowchart(code) {
    const nodes = [];
    let yPosition = 50;
    
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
        } else if (trimmed.includes('print') || trimmed.includes('console.log') || trimmed.includes('cout')) {
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
        } else if (trimmed.includes('input') || trimmed.includes('scanf') || trimmed.includes('cin')) {
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

function generateFlowchart() {
    const codeEditor = document.getElementById('code-editor');
    if (!codeEditor) return;
    
    const code = codeEditor.value;
    if (!code.trim()) {
        showToast('Введите код для генерации', 'error');
        return;
    }
    
    state.sourceCode = code;
    state.nodes = parseCodeToFlowchart(code);
    state.connections = [];
    state.selectedNodeId = null;
    
    addToHistory('Загружен исходный код и сгенерирована схема');
    renderNodes();
    renderInfoPanel();
    renderComments();
    showToast('Блок-схема сгенерирована из кода');
}

// History
function restoreHistory(entry) {
    state.nodes = JSON.parse(JSON.stringify(entry.nodes));
    state.connections = JSON.parse(JSON.stringify(entry.connections));
    state.selectedNodeId = null;
    
    renderNodes();
    renderInfoPanel();
    renderComments();
    showToast('Версия восстановлена');
}

// Utility Functions
function formatDate(date) {
    return new Intl.DateTimeFormat('ru-RU', {
        day: '2-digit',
        month: '2-digit',
        year: 'numeric',
        hour: '2-digit',
        minute: '2-digit',
        second: '2-digit'
    }).format(new Date(date));
}

function getTimeAgo(date) {
    const seconds = Math.floor((Date.now() - new Date(date).getTime()) / 1000);
    
    if (seconds < 60) return `${seconds} сек. назад`;
    const minutes = Math.floor(seconds / 60);
    if (minutes < 60) return `${minutes} мин. назад`;
    const hours = Math.floor(minutes / 60);
    if (hours < 24) return `${hours} ч. назад`;
    const days = Math.floor(hours / 24);
    return `${days} дн. назад`;
}

// Zoom Controls
function zoomIn() {
    state.zoom = Math.min(state.zoom + 0.1, 2);
    const zoomValue = document.getElementById('zoom-value');
    if (zoomValue) zoomValue.textContent = Math.round(state.zoom * 100);
    renderNodes();
}

function zoomOut() {
    state.zoom = Math.max(state.zoom - 0.1, 0.5);
    const zoomValue = document.getElementById('zoom-value');
    if (zoomValue) zoomValue.textContent = Math.round(state.zoom * 100);
    renderNodes();
}

// Tab Management
function switchTab(tabName) {
    document.querySelectorAll('.tab-btn').forEach(btn => {
        btn.classList.toggle('active', btn.dataset.tab === tabName);
    });
    document.querySelectorAll('.tab-content').forEach(content => {
        content.classList.toggle('active', content.id === `tab-${tabName}`);
    });
}

// Initialize editor when DOM is ready
function initializeEditor() {
    // Check if required elements exist
    const canvasWrapper = document.getElementById('canvas-wrapper');
    if (!canvasWrapper) {
        console.warn('Canvas wrapper not found, retrying in 100ms...');
        setTimeout(initializeEditor, 100);
        return;
    }
    
    // Remove existing event listeners to avoid duplicates
    const toolButtons = document.querySelectorAll('.tool-btn');
    toolButtons.forEach(btn => {
        const newBtn = btn.cloneNode(true);
        btn.parentNode.replaceChild(newBtn, btn);
    });
    
    // Toolbar buttons
    document.querySelectorAll('.tool-btn').forEach(btn => {
        btn.addEventListener('click', (e) => {
            e.preventDefault();
            e.stopPropagation();
            if (btn.dataset.type) {
                addNode(btn.dataset.type);
            }
        });
    });
    
    // Control buttons
    const zoomInBtn = document.getElementById('zoom-in');
    const zoomOutBtn = document.getElementById('zoom-out');
    const exportSvgBtn = document.getElementById('export-svg');
    const exportPngBtn = document.getElementById('export-png');
    
    if (zoomInBtn) {
        zoomInBtn.onclick = zoomIn;
    }
    if (zoomOutBtn) {
        zoomOutBtn.onclick = zoomOut;
    }
    if (exportSvgBtn) {
        exportSvgBtn.onclick = () => {
            showToast('Экспорт в SVG (функция в разработке)');
        };
    }
    if (exportPngBtn) {
        exportPngBtn.onclick = () => {
            showToast('Экспорт в PNG (функция в разработке)');
        };
    }
    
    // Canvas events
    if (canvasWrapper) {
        canvasWrapper.addEventListener('click', (e) => {
            if (e.target === canvasWrapper || e.target.id === 'flowchart-canvas' || e.target.id === 'connections-canvas' || e.target.id === 'connections-svg') {
                state.selectedNodeId = null;
                state.selectedConnectionId = null;
                state.connectingFrom = null;
                state.connectingFromPort = null;
                renderNodes();
                renderInfoPanel();
                renderComments();
                renderConnectionControls(null);
            }
        });
    }
    
    // Global mouse events (only add once)
    if (!window._editorMouseEventsInitialized) {
        document.addEventListener('mousemove', handleMouseMove);
        document.addEventListener('mouseup', handleMouseUp);
        window._editorMouseEventsInitialized = true;
    }
    
    // Cancel connection on Escape key
    if (!window._editorKeyEventsInitialized) {
        document.addEventListener('keydown', (e) => {
            if (e.key === 'Escape' && state.connectingFrom) {
                state.connectingFrom = null;
                state.connectingFromPort = null;
                renderNodes();
                showToast('Соединение отменено');
            }
        });
        window._editorKeyEventsInitialized = true;
    }
    
    // Info panel
    const saveNodeBtn = document.getElementById('save-node-btn');
    if (saveNodeBtn) {
        saveNodeBtn.onclick = updateNode;
    }
    
    // Code editor
    const generateBtn = document.getElementById('generate-btn');
    const loadExampleBtn = document.getElementById('load-example-btn');
    const fileUpload = document.getElementById('file-upload');
    
    if (generateBtn) {
        generateBtn.onclick = generateFlowchart;
    }
    if (loadExampleBtn) {
        loadExampleBtn.onclick = () => {
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
            const codeEditor = document.getElementById('code-editor');
            if (codeEditor) codeEditor.value = exampleCode;
        };
    }
    
    if (fileUpload) {
        fileUpload.onchange = (e) => {
            const file = e.target.files[0];
            if (file) {
                const reader = new FileReader();
                reader.onload = (event) => {
                    const codeEditor = document.getElementById('code-editor');
                    if (codeEditor) codeEditor.value = event.target.result;
                };
                reader.readAsText(file);
            }
        };
    }
    
    // Comments
    const addCommentBtn = document.getElementById('add-comment-btn');
    const newComment = document.getElementById('new-comment');
    
    if (addCommentBtn) {
        addCommentBtn.onclick = addComment;
    }
    if (newComment) {
        newComment.onkeydown = (e) => {
            if (e.key === 'Enter' && (e.ctrlKey || e.metaKey)) {
                e.preventDefault();
                addComment();
            }
        };
    }
    
    // Tabs
    document.querySelectorAll('.tab-btn').forEach(btn => {
        btn.onclick = () => {
            switchTab(btn.dataset.tab);
        };
    });
    
    // Initial render
    renderNodes();
    renderInfoPanel();
    renderComments();
    renderHistory();
    
    console.log('Editor initialized successfully');
}

// Make initializeEditor available globally for React component
window.initializeEditor = initializeEditor;

// Auto-initialize when script loads (for non-React usage)
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', () => {
        // Only auto-initialize if not in React context
        if (!document.getElementById('root')) {
            initializeEditor();
        }
    });
} else {
    // DOM already loaded - only auto-initialize if not in React context
    if (!document.getElementById('root')) {
        setTimeout(initializeEditor, 100);
    }
}

} // End of duplicate loading check
