// Конвертер блок-схемы обратно в JSON (AST/SPR формат)
import type { FlowchartNode, Connection } from '../types/flowchart';
import type { ASTProgram, PascalProgram, ASTStatement } from '../types/parser';

/**
 * Определяет язык программирования на основе metadata узлов
 */
function detectLanguage(nodes: FlowchartNode[]): 'pascal' | 'c' | 'cpp' | null {
    for (const node of nodes) {
        if (node.metadata?.language) {
            return node.metadata.language;
        }
    }
    return null;
}

/**
 * Восстанавливает JSON из блок-схемы
 */
export function flowchartToJSON(
    nodes: FlowchartNode[],
    connections: Connection[]
): ASTProgram | PascalProgram | null {
    if (nodes.length === 0) {
        return null;
    }

    const language = detectLanguage(nodes);
    if (!language) {
        console.warn('[flowchartToJSON] Не удалось определить язык программирования');
        return null;
    }

    if (language === 'pascal') {
        return restorePascalSPR(nodes, connections);
    } else if (language === 'c' || language === 'cpp') {
        return restoreCAST(nodes, connections);
    }

    return null;
}

/**
 * Восстанавливает Pascal SPR формат из блок-схемы
 */
function restorePascalSPR(
    nodes: FlowchartNode[],
    connections: Connection[]
): PascalProgram | null {
    // Фильтруем только узлы с metadata (игнорируем start/end)
    const nodesWithMetadata = nodes.filter(
        node => node.metadata?.astElement && node.type !== 'start' && node.type !== 'end'
    );

    if (nodesWithMetadata.length === 0) {
        console.warn('[restorePascalSPR] Нет узлов с метаданными');
        return null;
    }

    // Строим граф связей для определения порядка
    const connectionMap = new Map<string, Connection[]>();
    const reverseConnectionMap = new Map<string, Connection[]>();
    for (const conn of connections) {
        if (!connectionMap.has(conn.from)) {
            connectionMap.set(conn.from, []);
        }
        connectionMap.get(conn.from)!.push(conn);
        
        if (!reverseConnectionMap.has(conn.to)) {
            reverseConnectionMap.set(conn.to, []);
        }
        reverseConnectionMap.get(conn.to)!.push(conn);
    }

    // Находим узлы верхнего уровня (без входящих связей или только от start)
    const hasIncoming = new Set<string>();
    for (const conn of connections) {
        if (conn.from !== 'start') {
            hasIncoming.add(conn.to);
        }
    }

    // Группируем узлы по секциям на основе их типа в metadata и порядка
    const sections: {
        functionBlock?: Record<string, any>;
        constantBlock?: Record<string, any>;
        variableBlock?: Record<string, any>;
        mainBlock?: Record<string, any>;
    } = {
        functionBlock: {},
        constantBlock: {},
        variableBlock: {},
        mainBlock: {}
    };

    // Сортируем узлы по Y-координате для стабильного порядка
    const sortedNodes = [...nodesWithMetadata].sort((a, b) => a.y - b.y);

    // Определяем, какие узлы являются top-level
    // Top-level узлы - это функции, константы, переменные и элементы mainBlock,
    // которые не являются частью вложенных структур
    const topLevelNodes = new Set<string>();
    
    // Сначала определяем функции, константы и переменные как top-level на основе их типа
    for (const node of sortedNodes) {
        const nodeType = node.metadata?.nodeType;
        const astElement = node.metadata?.astElement;
        if (!astElement) continue;
        
        // Функции и процедуры всегда top-level
        if (nodeType === 'function' || nodeType === 'procedure') {
            topLevelNodes.add(node.id);
            continue;
        }
        
        // Константы и переменные определяем по их содержимому
        const isStringElement = typeof astElement === 'string';
        const nodeText = node.text || '';
        
        if (isStringElement || nodeType === 'string') {
            // Проверяем, является ли это константой или переменной
            if (isPascalConstant(isStringElement ? astElement : nodeText)) {
                topLevelNodes.add(node.id);
                continue;
            } else if ((isStringElement ? astElement : nodeText).includes(':') && !isPascalConstant(isStringElement ? astElement : nodeText)) {
                topLevelNodes.add(node.id);
                continue;
            }
        }
    }
    
    // Затем находим узлы mainBlock, которые соединены от top-level узлов
    // но не являются частью вложенных структур
    // Используем итеративный подход: добавляем узлы mainBlock, которые соединены от уже определенных top-level узлов
    let changed = true;
    while (changed) {
        changed = false;
        for (const node of sortedNodes) {
            if (topLevelNodes.has(node.id)) continue; // Уже добавлен
            
            const nodeType = node.metadata?.nodeType;
            // Пропускаем вложенные структуры (if, while, for, caseOf) - они не top-level
            if (nodeType === 'if' || nodeType === 'while' || nodeType === 'for' || nodeType === 'until' || nodeType === 'caseOf') {
                continue;
            }
            
            const incoming = reverseConnectionMap.get(node.id) || [];
            // Проверяем, все ли входящие связи идут от top-level узлов или start
            const allFromTopLevel = incoming.length > 0 && incoming.every(c => {
                if (c.from === 'start') return true;
                return topLevelNodes.has(c.from);
            });
            
            // Проверяем, нет ли исходящих связей с метками 'true'/'false' (это вложенная структура)
            const outgoing = connectionMap.get(node.id) || [];
            const hasLabeledConnections = outgoing.some(c => c.label === 'true' || c.label === 'false');
            
            // Если все входящие связи от top-level узлов и нет меток - это top-level узел
            if (allFromTopLevel && !hasLabeledConnections && !hasIncoming.has(node.id)) {
                topLevelNodes.add(node.id);
                changed = true; // Продолжаем итерацию, так как этот узел может быть источником для других
            }
        }
    }

    // Определяем узлы, которые являются частью вложенных структур (if-else, циклы, caseOf)
    // Эти узлы не должны быть в top-level секциях
    const nodesInBlocks = new Set<string>();
    
    // Рекурсивно помечаем все узлы, которые являются частью вложенных структур
    // Помечаем узлы, которые соединены через любые метки (true/false для if, метки веток для caseOf) и их продолжения
    function markNodesInBlocks(nodeId: string, visited: Set<string>, parentNodeType?: string) {
        if (visited.has(nodeId)) return;
        visited.add(nodeId);
        
        const outgoing = connectionMap.get(nodeId) || [];
        for (const conn of outgoing) {
            const targetNode = sortedNodes.find(n => n.id === conn.to);
            if (!targetNode) continue;
            
            const targetNodeType = targetNode.metadata?.nodeType;
            
            // Если связь имеет метку (true/false для if, метки веток для caseOf), это часть вложенной структуры
            if (conn.label) {
                // Для caseOf метки могут быть "1 2 3", "4 5", "else" и т.д.
                // Для if метки - "true" или "false"
                // Для циклов меток обычно нет, но они тоже вложенные структуры
                if (parentNodeType === 'caseOf' || conn.label === 'true' || conn.label === 'false') {
                    nodesInBlocks.add(conn.to);
                    // Рекурсивно помечаем все узлы, которые соединены от этого узла
                    markNodesInBlocks(conn.to, visited, parentNodeType);
                }
            }
            
            // Также помечаем узлы, которые соединены без метки от узлов в блоках
            // (это продолжение тела вложенной структуры)
            // НО: если текущий узел - это сам caseOf/if/while/for/until (родительская структура),
            // то выход из него без метки - это выход из структуры, а не продолжение тела
            if (nodesInBlocks.has(nodeId) && !conn.label) {
                // Проверяем, является ли текущий узел самим caseOf/if/while/for/until (родительской структурой)
                const currentNode = sortedNodes.find(n => n.id === nodeId);
                const currentNodeType = currentNode?.metadata?.nodeType;
                
                // Если текущий узел - это родительская структура (caseOf, if, while, for, until),
                // то выход из него без метки - это выход из структуры, а не продолжение тела
                // НЕ помечаем следующие узлы как часть блока
                if (currentNodeType === 'caseOf' || currentNodeType === 'if' || currentNodeType === 'while' || currentNodeType === 'for' || currentNodeType === 'until') {
                    // Это выход из структуры, не помечаем следующие узлы
                    continue;
                }
                
                // Не помечаем родительские структуры (if, while, for, caseOf) как часть блока
                if (targetNodeType !== 'if' && targetNodeType !== 'while' && targetNodeType !== 'for' && targetNodeType !== 'until' && targetNodeType !== 'caseOf' && targetNodeType !== 'else') {
                    nodesInBlocks.add(conn.to);
                    markNodesInBlocks(conn.to, visited, parentNodeType);
                }
            }
        }
    }
    
    // Помечаем все узлы, которые являются частью вложенных структур
    for (const node of sortedNodes) {
        const astElement = node.metadata?.astElement;
        if (!astElement) continue;
        
        const nodeType = node.metadata?.nodeType;
        // Если узел является условной конструкцией, циклом или caseOf, помечаем все его дочерние узлы
        // НЕ помечаем else как часть блока, так как else - это отдельный элемент в JSON
        if (nodeType === 'if' || nodeType === 'while' || nodeType === 'for' || nodeType === 'until' || nodeType === 'caseOf') {
            markNodesInBlocks(node.id, new Set(), nodeType);
        }
    }
    
    console.log('[restorePascalSPR] After marking blocks, nodesInBlocks size:', nodesInBlocks.size);

    // Разделяем узлы на секции
    // Порядок в Pascal: functionBlock -> constantBlock -> variableBlock -> mainBlock
    const functionNodes: { node: FlowchartNode; y: number }[] = [];
    const constantNodes: { node: FlowchartNode; y: number }[] = [];
    const variableNodes: { node: FlowchartNode; y: number }[] = [];
    const mainNodes: { node: FlowchartNode; y: number }[] = [];

    for (const node of sortedNodes) {
        const astElement = node.metadata?.astElement;
        if (!astElement) continue;

        const nodeType = node.metadata?.nodeType;
        const isTopLevel = topLevelNodes.has(node.id);
        const isInBlock = nodesInBlocks.has(node.id);
        const isStringElement = typeof astElement === 'string';
        const isObjectElement = typeof astElement === 'object' && astElement !== null;

        // Пропускаем узлы, которые являются частью вложенных структур (кроме самих структур и else)
        // else не пропускаем, так как он отдельный элемент в JSON структуре
        if (isInBlock && nodeType !== 'if' && nodeType !== 'while' && nodeType !== 'for' && nodeType !== 'until' && nodeType !== 'caseOf' && nodeType !== 'else') {
            continue; // Эти узлы будут восстановлены как часть родительской структуры
        }

        // Определяем секцию на основе типа узла, типа astElement и позиции
        if (nodeType === 'function' || nodeType === 'procedure') {
            // Функция или процедура - всегда в functionBlock
            functionNodes.push({ node, y: node.y });
        } else if (isTopLevel && !isInBlock) {
            // Только top-level узлы могут быть в constantBlock или variableBlock
            // Проверяем по тексту узла, так как astElement может быть строкой или объектом
            const nodeText = node.text || '';
            
            if (isStringElement && isPascalConstant(astElement)) {
                // Константа (top-level, astElement - строка, содержит ":" и "=" с типом между ними)
                constantNodes.push({ node, y: node.y });
            } else if (isStringElement && astElement.includes(':') && !isPascalConstant(astElement)) {
                // Переменная (top-level, astElement - строка, содержит ":" но не является константой)
                variableNodes.push({ node, y: node.y });
            } else if (isObjectElement && nodeType === 'string' && isPascalConstant(nodeText)) {
                // Константа (top-level, astElement - объект, текст содержит константу)
                constantNodes.push({ node, y: node.y });
            } else if (isObjectElement && nodeType === 'string' && nodeText.includes(':') && !isPascalConstant(nodeText)) {
                // Переменная (top-level, astElement - объект, текст содержит ":" но не является константой)
                variableNodes.push({ node, y: node.y });
            } else if (isPascalConstant(nodeText)) {
                // Константа (определяем по тексту узла)
                constantNodes.push({ node, y: node.y });
            } else if (nodeText.includes(':') && !isPascalConstant(nodeText) && nodeType === 'string') {
                // Переменная (определяем по тексту узла)
                variableNodes.push({ node, y: node.y });
            } else {
                // Основной блок программы (top-level, но не константа/переменная)
                mainNodes.push({ node, y: node.y });
            }
        } else {
            // Узлы, которые не являются top-level
            // Проверяем, не являются ли они частью вложенных структур
            if (isInBlock) {
                // Если это родительская структура (if, while, for, caseOf, else), добавляем в mainBlock
                // else добавляем, так как он отдельный элемент в JSON структуре
                if (nodeType === 'if' || nodeType === 'while' || nodeType === 'for' || nodeType === 'until' || nodeType === 'caseOf' || nodeType === 'else') {
                    mainNodes.push({ node, y: node.y });
                }
                // Остальные узлы (дочерние элементы вложенных структур) пропускаем - они уже в astElement родителя
            } else {
                // Узлы, которые не top-level и не в блоке - это узлы mainBlock, которые соединены от других узлов mainBlock
                // Добавляем их в mainBlock (включая else)
                mainNodes.push({ node, y: node.y });
            }
        }
    }

    // Сортируем каждую секцию по Y-координате
    functionNodes.sort((a, b) => a.y - b.y);
    constantNodes.sort((a, b) => a.y - b.y);
    variableNodes.sort((a, b) => a.y - b.y);
    mainNodes.sort((a, b) => a.y - b.y);

    // Заполняем секции
    let exprIndex = 0;

    // functionBlock
    for (const { node } of functionNodes) {
        const astElement = node.metadata?.astElement;
        if (!astElement) continue;
        const exprName = `expr${exprIndex++}`;
        const originalElement = JSON.parse(JSON.stringify(astElement));
        if (node.text !== originalElement.declaration) {
            originalElement.declaration = node.text;
        }
        sections.functionBlock![exprName] = originalElement;
    }

    // constantBlock
    for (const { node } of constantNodes) {
        const exprName = `expr${exprIndex++}`;
        const astElement = node.metadata?.astElement;
        // Если astElement - строка, используем её, иначе используем текст узла
        const constText = typeof astElement === 'string' ? astElement : node.text;
        sections.constantBlock![exprName] = constText;
    }

    // variableBlock
    for (const { node } of variableNodes) {
        const exprName = `expr${exprIndex++}`;
        const astElement = node.metadata?.astElement;
        // Если astElement - строка, используем её, иначе используем текст узла
        const varText = typeof astElement === 'string' ? astElement : node.text;
        sections.variableBlock![exprName] = varText;
    }

    // mainBlock - восстанавливаем все элементы в правильном порядке
    // Вложенные элементы уже содержатся в astElement родительских структур
    // Используем Set для отслеживания уже добавленных узлов, чтобы избежать дублирования
    const addedNodeIds = new Set<string>();
    
    console.log('[restorePascalSPR] mainNodes count:', mainNodes.length);
    console.log('[restorePascalSPR] mainNodes:', mainNodes.map(m => ({ 
        id: m.node.id, 
        text: m.node.text.substring(0, 50), 
        nodeType: m.node.metadata?.nodeType, 
        isTopLevel: topLevelNodes.has(m.node.id), 
        isInBlock: nodesInBlocks.has(m.node.id) 
    })));
    console.log('[restorePascalSPR] nodesInBlocks:', Array.from(nodesInBlocks));
    
    for (let idx = 0; idx < mainNodes.length; idx++) {
        const { node } = mainNodes[idx];
        const astElement = node.metadata?.astElement;
        if (!astElement) {
            console.warn('[restorePascalSPR] Skipping node without astElement:', node.id, node.text);
            continue;
        }
        
        // Пропускаем узлы, которые уже были добавлены
        if (addedNodeIds.has(node.id)) {
            console.log('[restorePascalSPR] Skipping already added node:', node.id, node.text);
            continue;
        }
        
        const nodeType = node.metadata?.nodeType;
        const isInBlock = nodesInBlocks.has(node.id);
        
        // Пропускаем узлы, которые являются частью вложенных структур
        // (они уже содержатся в astElement родительских узлов)
        if (isInBlock) {
            // Проверяем, является ли этот узел родительской структурой (if, while, for, caseOf)
            if (nodeType !== 'if' && nodeType !== 'while' && nodeType !== 'for' && nodeType !== 'until' && nodeType !== 'caseOf' && nodeType !== 'else') {
                console.log('[restorePascalSPR] Skipping child node in block:', node.id, node.text.substring(0, 50), 'nodeType:', nodeType);
                continue; // Пропускаем дочерние узлы, они уже в astElement родителя
            }
        }
        
        // Проверяем, является ли следующий узел else для этого if
        let nextNode: FlowchartNode | null = null;
        if (idx + 1 < mainNodes.length) {
            nextNode = mainNodes[idx + 1].node;
        }
        
        const exprName = `expr${exprIndex++}`;
        const originalElement = JSON.parse(JSON.stringify(astElement));
        
        // Обновляем value/condition если текст был изменен
        if (node.type === 'decision' && originalElement.condition) {
            originalElement.condition = extractConditionFromText(node.text);
        } else if (node.type === 'process' && originalElement.value) {
            originalElement.value = node.text;
        } else if (node.type === 'input' || node.type === 'output') {
            originalElement.value = node.text;
        }
        
        console.log('[restorePascalSPR] Adding to mainBlock:', exprName, 'node:', node.id, 'text:', node.text.substring(0, 50), 'nodeType:', nodeType, 'isInBlock:', isInBlock, 'isTopLevel:', topLevelNodes.has(node.id));
        
        // Для вложенных структур (if, while, for, caseOf) используем оригинальный astElement,
        // который уже содержит body. Не нужно восстанавливать body из связей, так как
        // оригинальный astElement уже содержит правильную структуру.
        sections.mainBlock![exprName] = originalElement;
        addedNodeIds.add(node.id);
        
        // Если это if и следующий узел - это else, пропускаем else в следующей итерации
        // так как он уже обработан вместе с if (но в JSON структуре else - отдельный элемент)
        if (nodeType === 'if' && nextNode && nextNode.metadata?.nodeType === 'else') {
            // Пропускаем else в следующей итерации, так как он уже обработан
            // Но в JSON структуре else должен быть отдельным элементом
            // Поэтому создаем else элемент из следующего узла
            const elseExprName = `expr${exprIndex++}`;
            const elseOriginalElement = JSON.parse(JSON.stringify(nextNode.metadata?.astElement));
            sections.mainBlock![elseExprName] = elseOriginalElement;
            addedNodeIds.add(nextNode.id);
            idx++; // Пропускаем else в следующей итерации
        }
        
        // Если это вложенная структура (if, while, for, caseOf), НЕ помечаем дочерние узлы как добавленные,
        // так как они уже содержатся в astElement родительской структуры и не должны добавляться отдельно.
        // Дочерние узлы будут пропущены при проверке isInBlock выше.
    }
    
    console.log('[restorePascalSPR] Final mainBlock keys:', Object.keys(sections.mainBlock || {}));

    // Определяем имя программы
    // Пытаемся извлечь из metadata startNode (где оно было сохранено при парсинге)
    let programName = 'program';
    
    const startNode = nodes.find(n => n.type === 'start');
    if (startNode && startNode.metadata?.astElement?.programName) {
        programName = startNode.metadata.astElement.programName;
    }

    return {
        program: {
            name: programName,
            sections: sections as any
        }
    };
}

// Вспомогательная: извлекает условие из текста (для Pascal/decision узлов)
function extractConditionFromText(text: string): string {
    return text
        .replace(/^if\s*\(/i, '')
        .replace(/\)\s*$/, '')
        .trim();
}

// Вспомогательная: проверяет, является ли строка константой Pascal
function isPascalConstant(text: string): boolean {
    const colonIndex = text.indexOf(':');
    const equalsIndex = text.indexOf('=');
    
    if (colonIndex < 0 || equalsIndex < 0 || equalsIndex <= colonIndex) {
        return false;
    }
    
    // Берем текст между ":" и "="
    const between = text.substring(colonIndex + 1, equalsIndex).trim();
    // Если между ":" и "=" есть текст (тип данных) - это константа
    // Если между ":" и "=" пусто - это присваивание ":="
    return between.length > 0;
}

/**
 * Восстанавливает C/C++ AST формат из блок-схемы
 */
function restoreCAST(
    nodes: FlowchartNode[],
    connections: Connection[]
): ASTProgram | null {
    // Фильтруем только узлы с metadata (игнорируем start/end)
    const nodesWithMetadata = nodes.filter(
        node => node.metadata?.astElement && node.type !== 'start' && node.type !== 'end'
    );

    if (nodesWithMetadata.length === 0) {
        console.warn('[restoreCAST] Нет узлов с метаданными');
        return null;
    }

    // Строим граф связей
    const connectionMap = new Map<string, Connection[]>();
    const reverseConnectionMap = new Map<string, Connection[]>();
    for (const conn of connections) {
        if (!connectionMap.has(conn.from)) {
            connectionMap.set(conn.from, []);
        }
        connectionMap.get(conn.from)!.push(conn);
        
        if (!reverseConnectionMap.has(conn.to)) {
            reverseConnectionMap.set(conn.to, []);
        }
        reverseConnectionMap.get(conn.to)!.push(conn);
    }

    const nodesById = new Map<string, FlowchartNode>();
    for (const node of nodesWithMetadata) {
        nodesById.set(node.id, node);
    }

    // Находим узлы верхнего уровня (без входящих связей или только от start)
    const hasIncoming = new Set<string>();
    for (const conn of connections) {
        if (conn.from !== 'start') {
            hasIncoming.add(conn.to);
        }
    }

    // Восстанавливаем statements в правильном порядке
    // Используем оригинальный AST из metadata, который уже содержит правильную структуру
    const statements: ASTStatement[] = [];

    // Используем оригинальный AST из metadata, который уже содержит правильную структуру
    // Собираем все statements верхнего уровня, игнорируя узлы, которые являются частью тел блоков
    
    // Используем оригинальный AST из metadata, который уже содержит правильную структуру.
    // Собираем все statements верхнего уровня, избегая дублирования:
    //  - PreprocessorDirective
    //  - StructDecl / TypedefDecl
    //  - Глобальные VarDeclStmt (не входящие в тело main)
    //  - FunctionDecl (предпочитаем версии с body, если есть)

    const topLevelStatements: ASTStatement[] = [];
    const functionByName = new Map<string, { 
        stmt: ASTStatement; 
        y: number;
        declaration?: ASTStatement | null; // Объявление без body (перед main)
        declarationY?: number; // Y-координата объявления
        definition?: ASTStatement | null; // Определение с body (после main)
        definitionY?: number; // Y-координата определения
    }>();
    const globalVars: { stmt: ASTStatement; y: number }[] = [];
    const preprocessors: { stmt: ASTStatement; y: number }[] = [];
    const structs: { stmt: ASTStatement; y: number }[] = [];
    const typedefs: { stmt: ASTStatement; y: number }[] = [];

    // Узел main (для проверки локальных переменных)
    const mainNode = nodesWithMetadata.find(n => n.metadata?.astElement?.type === 'FunctionDecl' && (n.metadata.astElement as any).name === 'main');
    const mainAST = mainNode?.metadata?.astElement;

    // Помощник: проверяем, находится ли VarDeclStmt внутри main (по оригинальному AST main)
    function isVarInMain(varName: string): boolean {
        // Если main не найден, переменная не может быть внутри main
        if (!mainAST || !mainAST.body || mainAST.body.type !== 'Block') {
            return false;
        }

        // Рекурсивно ищем переменную в теле main
        function dfs(stmts: any[]): boolean {
            if (!Array.isArray(stmts)) return false;
            
            for (const stmt of stmts) {
                if (!stmt || typeof stmt !== 'object') continue;
                
                // Проверяем, является ли это объявлением переменной с нужным именем
                if (stmt.type === 'VarDeclStmt' && stmt.name === varName) {
                    return true;
                }
                
                // Рекурсивно проверяем вложенные блоки
                if (stmt.type === 'Block' && Array.isArray(stmt.statements)) {
                    if (dfs(stmt.statements)) return true;
                }
                if (stmt.thenBranch && stmt.thenBranch.statements && Array.isArray(stmt.thenBranch.statements)) {
                    if (dfs(stmt.thenBranch.statements)) return true;
                }
                if (stmt.elseBranch && stmt.elseBranch.statements && Array.isArray(stmt.elseBranch.statements)) {
                    if (dfs(stmt.elseBranch.statements)) return true;
                }
                if (stmt.body && stmt.body.statements && Array.isArray(stmt.body.statements)) {
                    if (dfs(stmt.body.statements)) return true;
                }
            }
            return false;
        }

        return dfs(mainAST.body.statements || []);
    }

    // Сканируем все узлы (по Y для стабильности)
    const sortedNodes = [...nodesWithMetadata].sort((a, b) => a.y - b.y);

    for (const node of sortedNodes) {
        const astElement = node.metadata?.astElement;
        if (!astElement) continue;
        const stmtType = astElement.type;

        if (stmtType === 'PreprocessorDirective') {
            preprocessors.push({ stmt: JSON.parse(JSON.stringify(astElement)), y: node.y });
        } else if (stmtType === 'StructDecl') {
            structs.push({ stmt: JSON.parse(JSON.stringify(astElement)), y: node.y });
        } else if (stmtType === 'TypedefDecl') {
            typedefs.push({ stmt: JSON.parse(JSON.stringify(astElement)), y: node.y });
        } else if (stmtType === 'VarDeclStmt') {
            const varName = (astElement as any).name;
            // Проверяем, является ли переменная глобальной (не внутри main)
            // Важно: проверяем по оригинальному AST main, а не по связям в блок-схеме
            const inMain = isVarInMain(varName);
            if (!inMain) {
                globalVars.push({ stmt: JSON.parse(JSON.stringify(astElement)), y: node.y });
            }
        } else if (stmtType === 'FunctionDecl') {
            const funcName = (astElement as any).name || '';
            const hasBody = !!(astElement as any).body;
            
            // Создаем копию AST элемента
            const astCopy = JSON.parse(JSON.stringify(astElement));
            
            // Для функций нужно сохранять и объявления (без body), и определения (с body)
            // Используем Map с ключом funcName, но храним обе версии: объявление и определение
            const existing = functionByName.get(funcName);
            
            if (!existing) {
                // Первая встреча функции - сохраняем
                functionByName.set(funcName, { 
                    stmt: astCopy, 
                    y: node.y,
                    declaration: hasBody ? null : astCopy, // Объявление (без body)
                    declarationY: hasBody ? undefined : node.y, // Y объявления
                    definition: hasBody ? astCopy : null,   // Определение (с body)
                    definitionY: hasBody ? node.y : undefined // Y определения
                });
            } else {
                const existingHasBody = !!(existing.stmt as any).body;
                const existingHasDeclaration = !!existing.declaration;
                
                // Обновляем объявление или определение
                if (hasBody && !existingHasBody) {
                    // Нашли определение - сохраняем его
                    existing.definition = astCopy;
                    existing.definitionY = node.y;
                    // Если объявления еще нет, сохраняем stmt как объявление
                    if (!existingHasDeclaration) {
                        existing.declaration = existing.stmt;
                        existing.declarationY = existing.y;
                    }
                    existing.y = node.y; // Обновляем Y для правильной сортировки
                } else if (!hasBody && existingHasBody) {
                    // Нашли объявление, но определение уже есть - сохраняем объявление
                    existing.declaration = astCopy;
                    existing.declarationY = node.y;
                    // Y не обновляем, так как определение важнее для позиции
                } else if (hasBody && existingHasBody) {
                    // Обе версии имеют body - предпочитаем ту, что идет позже (более полная)
                    if (node.y > existing.y) {
                        existing.definition = astCopy;
                        existing.definitionY = node.y;
                        existing.y = node.y;
                    }
                } else {
                    // Обе версии без body - предпочитаем ту, что идет раньше (первое объявление)
                    if (node.y < existing.y) {
                        existing.declaration = astCopy;
                        existing.declarationY = node.y;
                        existing.stmt = astCopy;
                        existing.y = node.y;
                    }
                }
            }
        }
    }

    // Собираем в порядке появления (по Y)
    preprocessors.sort((a, b) => a.y - b.y);
    structs.sort((a, b) => a.y - b.y);
    typedefs.sort((a, b) => a.y - b.y);
    globalVars.sort((a, b) => a.y - b.y);

    // Функции: сортируем по Y, main ставим после глобальных, но сохраняем Y для остальных
    const functionsArray = Array.from(functionByName.values()).sort((a, b) => a.y - b.y);
    const mainFunc = functionsArray.find(f => (f.stmt as any).name === 'main') || null;
    const otherFuncs = functionsArray.filter(f => (f.stmt as any).name !== 'main');

    // Итоговый порядок: директивы -> структуры -> typedef -> глобальные переменные -> объявления функций до main -> main -> определения функций после main
    topLevelStatements.push(...preprocessors.map(p => p.stmt));
    topLevelStatements.push(...structs.map(s => s.stmt));
    topLevelStatements.push(...typedefs.map(t => t.stmt));
    topLevelStatements.push(...globalVars.map(g => g.stmt));
    
    // Объявления функций (без body) перед main
    // Используем declarationY для определения позиции объявления относительно main
    const funcDeclarationsBeforeMain = otherFuncs
        .filter(f => {
            // Если есть declarationY, используем его, иначе используем y
            const declarationY = f.declarationY ?? f.y;
            return declarationY < (mainFunc?.y ?? Infinity);
        })
        .map(f => f.declaration) // Используем объявление
        .filter((stmt): stmt is ASTStatement => stmt !== null && stmt !== undefined);
    topLevelStatements.push(...funcDeclarationsBeforeMain);
    
    // main функция
    if (mainFunc) {
        topLevelStatements.push(mainFunc.stmt);
    }
    
    // Определения функций (с body) после main
    // Используем definitionY для определения позиции определения относительно main
    const funcDefinitionsAfterMain = otherFuncs
        .filter(f => {
            // Если есть definitionY, используем его, иначе используем y
            const definitionY = f.definitionY ?? f.y;
            return definitionY > (mainFunc?.y ?? -Infinity);
        })
        .map(f => f.definition) // Используем определение
        .filter((stmt): stmt is ASTStatement => stmt !== null && stmt !== undefined);
    topLevelStatements.push(...funcDefinitionsAfterMain);

    statements.push(...topLevelStatements);

    return {
        type: 'Program',
        name: 'program',
        body: {
            type: 'Block',
            statements: statements
        }
    };
}

// (Парсинг текста обратно в AST для C не используется здесь,
//  так как мы берем оригинальный AST из metadata.)
