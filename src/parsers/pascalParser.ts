// Парсер Pascal (SPR формат) в блок-схему
import type { FlowchartNode, Connection, NodeExitPoint, ProcessBlockResult } from '../types/flowchart';
import type { PascalProgram, PascalExpression, PascalFunction, PascalProcedure } from '../types/parser';
import type { NodeCreator, ConnectionCreator } from './parserUtils';
import { 
    createParserFactories, 
    normalizeExitNode, 
    getIOType, 
    truncateText, 
    sortExpressionKeys
} from './parserUtils';

/**
 * Проверяет, является ли строка константой Pascal (содержит ":" и "=" с типом между ними)
 */
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
 * Обрабатывает блок Pascal выражений
 */
function processPascalBlock(
    block: Record<string, PascalExpression>,
    parentNode: FlowchartNode | null,
    createNode: NodeCreator,
    createConnection: ConnectionCreator
): ProcessBlockResult {
    if (!block || typeof block !== 'object') {
        return { nodes: [], exitNodes: parentNode ? [parentNode] : [] };
    }
    
    const blockNodes: FlowchartNode[] = [];
    let previousExitNodes: (FlowchartNode | NodeExitPoint)[] = parentNode ? [parentNode] : [];
    
    // Получаем все выражения в правильном порядке
    const expressions = sortExpressionKeys(Object.entries(block));
    
    // Обрабатываем все выражения в блоке
    for (let i = 0; i < expressions.length; i++) {
        const [key, expr] = expressions[i];
        if (!expr || typeof expr !== 'object') continue;
        
        let currentNode: FlowchartNode | null = null;
        let exitNodes: (FlowchartNode | NodeExitPoint)[] = [];
        
        // Обработка разных типов выражений
        if (expr.type === 'io') {
            // Ввод/вывод
            const ioText = expr.value || '';
            const nodeType = getIOType(ioText);
            const displayText = truncateText(ioText, 30);
            currentNode = createNode(nodeType, displayText, ioText, undefined, undefined, undefined, {
                astElement: expr,
                language: 'pascal',
                nodeType: expr.type
            });
            
            // Соединяем с предыдущими элементами
            previousExitNodes.forEach(prevExit => {
                const normalized = normalizeExitNode(prevExit);
                const prevNode = normalized.node;
                if (prevNode && prevNode !== currentNode && prevNode !== parentNode) {
                    const fromPort = normalized.port || 'bottom';
                    const label = normalized.label || '';
                    createConnection(prevNode, currentNode, fromPort, 'top', label);
                }
            });
            
            exitNodes = [currentNode];
        } else if (expr.type === 'assign') {
            // Присваивание
            const text = expr.value || 'Присваивание';
            currentNode = createNode('process', truncateText(text, 30), expr.value || '', undefined, undefined, undefined, {
                astElement: expr,
                language: 'pascal',
                nodeType: expr.type
            });
            
            // Соединяем с предыдущими элементами
            previousExitNodes.forEach(prevExit => {
                const normalized = normalizeExitNode(prevExit);
                const prevNode = normalized.node;
                if (prevNode && prevNode !== currentNode && prevNode !== parentNode) {
                    const fromPort = normalized.port || 'bottom';
                    const label = normalized.label || '';
                    createConnection(prevNode, currentNode, fromPort, 'top', label);
                }
            });
            
            exitNodes = [currentNode];
        } else if (expr.type === 'if' || expr.type === 'If') {
            // Условие if
            const conditionText = expr.condition || 'Условие';
            currentNode = createNode('decision', truncateText(conditionText, 30), expr.condition || '', undefined, undefined, undefined, {
                astElement: expr,
                language: 'pascal',
                nodeType: expr.type
            });
            
            // Соединяем условие с предыдущими элементами
            previousExitNodes.forEach(prevExit => {
                const normalized = normalizeExitNode(prevExit);
                const prevNode = normalized.node;
                if (prevNode && prevNode !== currentNode && prevNode !== parentNode) {
                    const fromPort = normalized.port || 'bottom';
                    const label = normalized.label || '';
                    createConnection(prevNode, currentNode, fromPort, 'top', label);
                }
            });
            
            let trueExitNode: FlowchartNode | FlowchartNode[] | null = null;
            
            // Обрабатываем тело if (ветка true)
            if (expr.body && typeof expr.body === 'object') {
                const bodyResult = processPascalBlock(expr.body, currentNode, createNode, createConnection);
                const bodyNodes = bodyResult.nodes || [];
                const bodyExitNodes = bodyResult.exitNodes || [];
                
                if (bodyNodes.length > 0) {
                    // Соединяем условие с первой нодой тела (ветка "true" - слева)
                    createConnection(currentNode, bodyNodes[0], 'left', 'top', 'true');
                    // Используем exitNodes из processBlock
                    if (bodyExitNodes.length > 0) {
                        const normalizedExits = bodyExitNodes.map(exit => {
                            const normalized = normalizeExitNode(exit);
                            return normalized.node;
                        });
                        trueExitNode = normalizedExits.length === 1 ? normalizedExits[0] : normalizedExits;
                    } else {
                        trueExitNode = bodyNodes[bodyNodes.length - 1];
                    }
                } else {
                    trueExitNode = currentNode;
                }
            } else {
                trueExitNode = currentNode;
            }
            
            // Проверяем, есть ли следующий элемент - else
            const nextExpr = i + 1 < expressions.length ? expressions[i + 1][1] : null;
            if (nextExpr && (nextExpr.type === 'else' || nextExpr.type === 'Else')) {
                // Обрабатываем else
                i++; // Пропускаем else в основном цикле, он обработается здесь
                
                // Создаем else-узел как отдельный узел для возможности восстановления
                // Используем правильный порядок параметров: type, text, codeRef, width, height, functionBody, metadata
                const elseNode = createNode('process', 'else', 'else', null, null, undefined, {
                    astElement: nextExpr,
                    language: 'pascal',
                    nodeType: 'else'
                });
                
                // Соединяем if с else-узлом (для визуализации связи)
                createConnection(currentNode, elseNode, 'right', 'top', 'false');
                
                let falseExitNode: FlowchartNode | FlowchartNode[] | null = null;
                if (nextExpr.body && typeof nextExpr.body === 'object') {
                    const elseResult = processPascalBlock(nextExpr.body, elseNode, createNode, createConnection);
                    const elseBodyNodes = elseResult.nodes || [];
                    const elseExitNodes = elseResult.exitNodes || [];
                    
                    if (elseBodyNodes.length > 0) {
                        // Соединяем else-узел с первой нодой else body
                        createConnection(elseNode, elseBodyNodes[0], 'bottom', 'top');
                        if (elseExitNodes.length > 0) {
                            const normalizedExits = elseExitNodes.map(exit => {
                                const normalized = normalizeExitNode(exit);
                                return normalized.node;
                            });
                            falseExitNode = normalizedExits.length === 1 ? normalizedExits[0] : normalizedExits;
                        } else {
                            falseExitNode = elseBodyNodes[elseBodyNodes.length - 1];
                        }
                    } else {
                        falseExitNode = elseNode;
                    }
                } else {
                    falseExitNode = elseNode;
                }
                
                // Для if/else выходы - это оба exitNode'а веток
                exitNodes = [];
                const trueExits = Array.isArray(trueExitNode) ? trueExitNode : [trueExitNode];
                trueExits.forEach(exit => {
                    if (exit && exit !== currentNode) exitNodes.push(exit);
                });
                const falseExits = Array.isArray(falseExitNode) ? falseExitNode : [falseExitNode];
                falseExits.forEach(exit => {
                    if (exit && exit !== currentNode && exit !== elseNode) exitNodes.push(exit);
                });
                if (exitNodes.length === 0) exitNodes = [elseNode];
            } else {
                // Нет else - выходы: ветка true и ветка false (само условие через правый порт)
                exitNodes = [];
                const trueExits = Array.isArray(trueExitNode) ? trueExitNode : [trueExitNode];
                trueExits.forEach(exit => {
                    if (exit && exit !== currentNode) exitNodes.push(exit);
                });
                exitNodes.push({ node: currentNode, port: 'right', label: 'false' });
            }
        } else if (expr.type === 'else' || expr.type === 'Else') {
            // Else уже обработан вместе с if выше, пропускаем
            continue;
        } else if (expr.type === 'while' || expr.type === 'While') {
            // Цикл while
            const conditionText = expr.condition || 'Условие цикла';
            currentNode = createNode('decision', truncateText(conditionText, 30), expr.condition || '', undefined, undefined, undefined, {
                astElement: expr,
                language: 'pascal',
                nodeType: expr.type
            });
            
            // Соединяем условие с предыдущими элементами
            previousExitNodes.forEach(prevExit => {
                const normalized = normalizeExitNode(prevExit);
                const prevNode = normalized.node;
                if (prevNode && prevNode !== currentNode && prevNode !== parentNode) {
                    const fromPort = normalized.port || 'bottom';
                    const label = normalized.label || '';
                    createConnection(prevNode, currentNode, fromPort, 'top', label);
                }
            });
            
            // Обрабатываем тело цикла
            if (expr.body && typeof expr.body === 'object') {
                const bodyResult = processPascalBlock(expr.body, currentNode, createNode, createConnection);
                const bodyNodes = bodyResult.nodes || [];
                const bodyExitNodes = bodyResult.exitNodes || [];
                
                if (bodyNodes.length > 0) {
                    // Соединяем условие с первой нодой тела (ветка "true" - слева)
                    createConnection(currentNode, bodyNodes[0], 'left', 'top', 'true');
                    // Соединяем последнюю ноду/ноды тела обратно к условию (продолжение цикла - снизу)
                    const lastNodes = bodyExitNodes.length > 0
                        ? bodyExitNodes.map(exit => normalizeExitNode(exit).node)
                        : [bodyNodes[bodyNodes.length - 1]];
                    lastNodes.forEach(lastNode => {
                        if (lastNode && lastNode !== currentNode) {
                            createConnection(lastNode, currentNode, 'bottom', 'top');
                        }
                    });
                }
            }
            // Выход из цикла - это условие через правый порт (ветка "false")
            exitNodes = [{ node: currentNode, port: 'right', label: 'false' }];
        } else if (expr.type === 'for' || expr.type === 'For') {
            // Цикл for
            const conditionText = expr.condition || 'Цикл for';
            currentNode = createNode('decision', truncateText(conditionText, 30), expr.condition || '', undefined, undefined, undefined, {
                astElement: expr,
                language: 'pascal',
                nodeType: expr.type
            });
            
            // Соединяем условие с предыдущими элементами
            previousExitNodes.forEach(prevExit => {
                const normalized = normalizeExitNode(prevExit);
                const prevNode = normalized.node;
                if (prevNode && prevNode !== currentNode && prevNode !== parentNode) {
                    const fromPort = normalized.port || 'bottom';
                    const label = normalized.label || '';
                    createConnection(prevNode, currentNode, fromPort, 'top', label);
                }
            });
            
            // Обрабатываем тело цикла
            if (expr.body && typeof expr.body === 'object') {
                const bodyResult = processPascalBlock(expr.body, currentNode, createNode, createConnection);
                const bodyNodes = bodyResult.nodes || [];
                const bodyExitNodes = bodyResult.exitNodes || [];
                
                if (bodyNodes.length > 0) {
                    // Соединяем условие с первой нодой тела (ветка "true" - слева)
                    createConnection(currentNode, bodyNodes[0], 'left', 'top', 'true');
                    // Соединяем последнюю ноду/ноды тела обратно к условию (продолжение цикла - снизу)
                    const lastNodes = bodyExitNodes.length > 0
                        ? bodyExitNodes.map(exit => normalizeExitNode(exit).node)
                        : [bodyNodes[bodyNodes.length - 1]];
                    lastNodes.forEach(lastNode => {
                        if (lastNode && lastNode !== currentNode) {
                            createConnection(lastNode, currentNode, 'bottom', 'top');
                        }
                    });
                }
            }
            // Выход из цикла - это условие через правый порт (ветка "false")
            exitNodes = [{ node: currentNode, port: 'right', label: 'false' }];
        } else if (expr.type === 'until') {
            // Цикл repeat-until (особый случай - тело сначала, потом условие)
            const conditionText = expr.condition || 'Условие until';
            
            // Сначала обрабатываем тело цикла
            let firstBodyNode: FlowchartNode | null = null;
            let lastBodyNodes: FlowchartNode[] = [];
            
            if (expr.body && typeof expr.body === 'object') {
                const parentForBody = previousExitNodes[0] 
                    ? normalizeExitNode(previousExitNodes[0]).node 
                    : parentNode;
                const bodyResult = processPascalBlock(expr.body, parentForBody, createNode, createConnection);
                const bodyNodes = bodyResult.nodes || [];
                const bodyExitNodes = bodyResult.exitNodes || [];
                
                if (bodyNodes.length > 0) {
                    firstBodyNode = bodyNodes[0];
                    if (bodyExitNodes.length > 0) {
                        lastBodyNodes = bodyExitNodes.map(exit => normalizeExitNode(exit).node);
                    } else {
                        lastBodyNodes = [bodyNodes[bodyNodes.length - 1]];
                    }
                    blockNodes.push(...bodyNodes);
                    
                    // Соединяем с предыдущими элементами
                    previousExitNodes.forEach(prevExit => {
                        const normalized = normalizeExitNode(prevExit);
                        const prevNode = normalized.node;
                        if (prevNode && prevNode !== firstBodyNode && prevNode !== parentNode) {
                            const fromPort = normalized.port || 'bottom';
                            const label = normalized.label || '';
                            createConnection(prevNode, firstBodyNode, fromPort, 'top', label);
                        }
                    });
                }
            }
            
            // Создаем ноду условия (после тела)
            currentNode = createNode('decision', truncateText(conditionText, 30), expr.condition || '', undefined, undefined, undefined, {
                astElement: expr,
                language: 'pascal',
                nodeType: expr.type
            });
            blockNodes.push(currentNode);
            
            // Соединяем последние элементы тела к условию (безымянная ветка)
            if (lastBodyNodes.length > 0) {
                lastBodyNodes.forEach(lastNode => {
                    if (lastNode && lastNode !== currentNode) {
                        createConnection(lastNode, currentNode);
                    }
                });
            } else if (previousExitNodes.length > 0) {
                const prevNode = normalizeExitNode(previousExitNodes[0]).node;
                if (prevNode && prevNode !== parentNode) {
                    createConnection(prevNode, currentNode);
                }
            }
            
            // В repeat-until:
            // - Из условия until должна идти ветка true в первый элемент тела (цикл продолжается)
            // - Ветка false из первого элемента тела идет в элемент за рамками цикла (выход из цикла)
            if (firstBodyNode) {
                createConnection(currentNode, firstBodyNode, 'left', 'top', 'true');
                exitNodes = [{ node: firstBodyNode, port: 'right', label: 'false' }];
            } else {
                exitNodes = [{ node: currentNode, port: 'left', label: 'true' }];
            }
        } else if (expr.type === 'caseOf') {
            // Case of
            const caseText = `Case: ${expr.compareValue || ''}`;
            currentNode = createNode('decision', truncateText(caseText, 30), expr.compareValue || '', undefined, undefined, undefined, {
                astElement: expr,
                language: 'pascal',
                nodeType: expr.type
            });
            
            // Соединяем условие с предыдущими элементами
            previousExitNodes.forEach(prevExit => {
                const normalized = normalizeExitNode(prevExit);
                const prevNode = normalized.node;
                if (prevNode && prevNode !== currentNode && prevNode !== parentNode) {
                    const fromPort = normalized.port || 'bottom';
                    const label = normalized.label || '';
                    createConnection(prevNode, currentNode, fromPort, 'top', label);
                }
            });
            
            // Собираем все выходы веток
            const branchExits: FlowchartNode[] = [];
            
            // Обрабатываем ветки - они находятся в expr.body
            if (expr.body && typeof expr.body === 'object') {
                const branchKeys = Object.keys(expr.body).sort();
                branchKeys.forEach((branchKey) => {
                    const branch = (expr.body as any)[branchKey];
                    if (branch && branch.todo && typeof branch.todo === 'object') {
                        const branchResult = processPascalBlock(branch.todo, currentNode, createNode, createConnection);
                        const branchNodes = branchResult.nodes || [];
                        const branchExitNodes = branchResult.exitNodes || [];
                        
                        // Добавляем узлы ветки в общий список узлов блока
                        branchNodes.forEach(node => {
                            if (!blockNodes.includes(node)) {
                                blockNodes.push(node);
                            }
                        });
                        
                        if (branchNodes.length > 0) {
                            // Все именные ветки из caseOf должны выходить из нижней точки
                            const port = 'bottom';
                            const branchLabel = branch.conditionValues || branchKey;
                            createConnection(currentNode, branchNodes[0], port, 'top', branchLabel);
                            
                            if (branchExitNodes.length > 0) {
                                branchExitNodes.forEach(exit => {
                                    branchExits.push(normalizeExitNode(exit).node);
                                });
                            } else {
                                branchExits.push(branchNodes[branchNodes.length - 1]);
                            }
                        }
                    }
                });
            }
            
            exitNodes = branchExits.length > 0 ? branchExits : [currentNode];
        } else if (expr.value) {
            // Простое выражение
            const text = expr.value;
            currentNode = createNode('process', truncateText(text, 30), expr.value, undefined, undefined, undefined, {
                astElement: expr,
                language: 'pascal',
                nodeType: expr.type
            });
            
            // Соединяем с предыдущими элементами
            previousExitNodes.forEach(prevExit => {
                const normalized = normalizeExitNode(prevExit);
                const prevNode = normalized.node;
                if (prevNode && prevNode !== currentNode && prevNode !== parentNode) {
                    const fromPort = normalized.port || 'bottom';
                    const label = normalized.label || '';
                    createConnection(prevNode, currentNode, fromPort, 'top', label);
                }
            });
            
            exitNodes = [currentNode];
        }
        
        if (currentNode) {
            blockNodes.push(currentNode);
            
            // Обновляем previousExitNodes для следующей итерации
            previousExitNodes = exitNodes.map(exit => {
                if (typeof exit === 'object' && 'node' in exit) {
                    return exit;
                }
                return exit;
            });
        }
    }
    
    return {
        nodes: blockNodes.length > 0 
            ? blockNodes 
            : (previousExitNodes.length > 0 && previousExitNodes[0] && previousExitNodes[0] !== parentNode 
                ? [normalizeExitNode(previousExitNodes[0]).node] 
                : []),
        exitNodes: previousExitNodes
    };
}

/**
 * Парсит Pascal программу (SPR формат) в блок-схему
 */
export function parsePascalToFlowchart(
    jsonData: PascalProgram,
    nodes: FlowchartNode[],
    connections: Connection[]
): { nodes: FlowchartNode[]; connections: Connection[] } {
    let nodeIdCounter = 0;
    let yPosition = 50;
    
    const getNextNodeId = () => nodeIdCounter++;
    const getNextYPosition = () => yPosition;
    const updateYPosition = (delta: number) => { yPosition += delta; };
    
    const { createNode, createConnection } = createParserFactories(
        nodes,
        connections,
        getNextNodeId,
        getNextYPosition,
        updateYPosition
    );
    
    const program = jsonData.program;
    // Сохраняем имя программы в metadata startNode для последующего восстановления
    const programName = program.name || 'program';
    const startNode = createNode('start', 'Начало', '', undefined, undefined, undefined, {
        astElement: { programName },
        language: 'pascal',
        nodeType: 'program'
    });
    let lastMainNode: FlowchartNode = startNode;
    
    const sections = program.sections || {};
    
    // Обрабатываем functionBlock - функции и процедуры
    if (sections.functionBlock && typeof sections.functionBlock === 'object') {
        const functionExprs = sortExpressionKeys(Object.entries(sections.functionBlock));
        
        for (const [key, funcExpr] of functionExprs) {
            if (!funcExpr || typeof funcExpr !== 'object') continue;
            
            const funcDecl = funcExpr as PascalFunction | PascalProcedure;
            const declText = funcDecl.declaration || 'Без объявления';
            const funcNode = createNode('process', truncateText(declText, 50), declText, undefined, undefined, undefined, {
                astElement: funcExpr,
                language: 'pascal',
                nodeType: funcDecl.type
            });
            
            // Соединяем с предыдущей нодой
            if (lastMainNode !== startNode) {
                createConnection(lastMainNode, funcNode);
            } else {
                createConnection(startNode, funcNode);
            }
            lastMainNode = funcNode;
        }
    }
    
    // Обрабатываем constantBlock - константы как assign ноды
    if (sections.constantBlock && typeof sections.constantBlock === 'object') {
        const constantExprs = sortExpressionKeys(Object.entries(sections.constantBlock));
        
        for (const [key, constExpr] of constantExprs) {
            let constText = 'Константа';
            
            if (typeof constExpr === 'string') {
                constText = constExpr;
            } else if (constExpr && typeof constExpr === 'object') {
                if (constExpr.type === 'assign') {
                    continue; // Пропускаем присваивания
                }
                constText = (constExpr as any).value || 'Константа';
            }
            
            // Фильтруем только настоящие константы
            if (!isPascalConstant(constText)) {
                continue;
            }
            
            const constNode = createNode('process', truncateText(constText, 30), constText, undefined, undefined, undefined, {
                astElement: constExpr,
                language: 'pascal',
                nodeType: typeof constExpr === 'string' ? 'string' : (constExpr && typeof constExpr === 'object' ? (constExpr as any).type || 'assign' : 'string')
            });
            
            if (lastMainNode) {
                createConnection(lastMainNode, constNode);
            } else {
                createConnection(startNode, constNode);
            }
            lastMainNode = constNode;
        }
    }
    
    // Обрабатываем variableBlock - переменные как assign ноды
    if (sections.variableBlock && typeof sections.variableBlock === 'object') {
        const variableExprs = sortExpressionKeys(Object.entries(sections.variableBlock));
        
        for (const [key, varExpr] of variableExprs) {
            let varText = 'Переменная';
            
            if (typeof varExpr === 'string') {
                varText = varExpr;
            } else if (varExpr && typeof varExpr === 'object') {
                varText = (varExpr as any).value || 'Переменная';
            }
            
            const varNode = createNode('process', truncateText(varText, 30), varText, undefined, undefined, undefined, {
                astElement: varExpr,
                language: 'pascal',
                nodeType: typeof varExpr === 'string' ? 'string' : 'assign'
            });
            
            if (lastMainNode !== startNode) {
                createConnection(lastMainNode, varNode);
            } else {
                createConnection(startNode, varNode);
            }
            lastMainNode = varNode;
        }
    }
    
    // Обрабатываем mainBlock - основной блок программы
    if (sections.mainBlock && typeof sections.mainBlock === 'object') {
        const mainResult = processPascalBlock(
            sections.mainBlock as Record<string, PascalExpression>,
            lastMainNode !== startNode ? lastMainNode : startNode,
            createNode,
            createConnection
        );
        const mainNodes = mainResult.nodes || [];
        
        if (mainNodes.length > 0) {
            if (lastMainNode !== startNode) {
                createConnection(lastMainNode, mainNodes[0]);
            } else {
                createConnection(startNode, mainNodes[0]);
            }
            lastMainNode = mainNodes[mainNodes.length - 1];
        }
    }
    
    // Создаем конечную ноду и соединяем с последней нодой программы
    const endNode = createNode('end', 'Конец', '');
    if (lastMainNode.id !== endNode.id) {
        createConnection(lastMainNode, endNode);
    }
    
    return { nodes, connections };
}

