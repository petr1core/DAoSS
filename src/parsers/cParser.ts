// Парсер C/C++ (AST формат) в блок-схему
import type { FlowchartNode, Connection, NodeExitPoint, ProcessBlockResult } from '../types/flowchart';
import type { ASTProgram, ASTBlock, ASTStatement } from '../types/parser';
import type { NodeCreator, ConnectionCreator } from './parserUtils';
import { convertExprToString } from './expressionConverter';
import {
    createParserFactories,
    normalizeExitNode,
    getIOType,
    truncateText
} from './parserUtils';

/**
 * Преобразует токен оператора в символ (ASSIGN -> =, PLUSASSIGN -> += и т.д.)
 */
function convertOperatorToSymbol(operator: string): string {
    const operatorMap: Record<string, string> = {
        'ASSIGN': '=',
        'PLUSASSIGN': '+=',
        'MINUSASSIGN': '-=',
        'MULTIASSIGN': '*=',
        'DIVASSIGN': '/=',
        'MODASSIGN': '%=',
        'INCREMENT': '++',
        'DECREMENT': '--'
    };
    return operatorMap[operator] || operator;
}

/**
 * Вычисляет размеры узла на основе длины текста
 */
function calculateNodeSize(text: string, nodeType: string): { width: number; height: number } {
    const minWidth = nodeType === 'decision' ? 180 : nodeType === 'start' || nodeType === 'end' ? 120 : 180;
    const minHeight = nodeType === 'decision' ? 100 : nodeType === 'start' || nodeType === 'end' ? 60 : 80;
    
    // Примерная ширина символа (10px) + отступы (40px)
    const estimatedWidth = Math.max(minWidth, Math.min(600, text.length * 8 + 40));
    // Примерная высота строки (20px) + отступы (40px)
    const lines = text.split('\n').length || 1;
    const estimatedHeight = Math.max(minHeight, Math.min(400, lines * 25 + 40));
    
    return { width: estimatedWidth, height: estimatedHeight };
}

/**
 * Преобразует AST блок в строку (для тела функции)
 */
function convertASTBlockToString(block: ASTBlock): string {
    if (!block || typeof block !== 'object' || block.type !== 'Block') {
        return '';
    }
    
    const statements = block.statements || [];
    if (!Array.isArray(statements)) {
        return '';
    }
    
    return statements
        .map(stmt => {
            if (!stmt || typeof stmt !== 'object') return '';
            const stmtText = convertASTStatementToString(stmt);
            // Добавляем отступ для вложенных блоков
            return stmtText.split('\n').map(line => '  ' + line).join('\n');
        })
        .filter(text => text.length > 0)
        .join('\n');
}

/**
 * Преобразует AST statement в строку (для for init и других случаев)
 */
function convertASTStatementToString(stmt: ASTStatement | null | undefined): string {
    if (!stmt || typeof stmt !== 'object') return '';
    
    if (stmt.type === 'VarDeclStmt') {
        const varType = stmt.varType || '';
        const varName = stmt.name || '';
        let text = `${varType} ${varName}`;
        if (stmt.initializer) {
            text += ` = ${convertExprToString(stmt.initializer)}`;
        }
        return text + ';';
    } else if (stmt.type === 'AssignStmt') {
        const target = stmt.target || '';
        const operator = convertOperatorToSymbol(stmt.operator || '=');
        const value = convertExprToString(stmt.value);
        return `${target} ${operator} ${value};`;
    } else if (stmt.type === 'ExprStmt' && stmt.expression) {
        return convertExprToString(stmt.expression) + ';';
    } else if (stmt.type === 'ReturnStmt') {
        const valueText = stmt.value ? convertExprToString(stmt.value) : '';
        return valueText ? `return ${valueText};` : 'return;';
    } else if (stmt.type === 'IfStmt') {
        const conditionText = convertExprToString(stmt.condition);
        let text = `if(${conditionText}) {\n`;
        if (stmt.thenBranch && typeof stmt.thenBranch === 'object' && stmt.thenBranch.type === 'Block') {
            text += convertASTBlockToString(stmt.thenBranch);
        }
        text += '}';
        if (stmt.elseBranch) {
            text += ' else {\n';
            if (typeof stmt.elseBranch === 'object' && stmt.elseBranch.type === 'Block') {
                text += convertASTBlockToString(stmt.elseBranch);
            }
            text += '}';
        }
        return text;
    } else if (stmt.type === 'WhileStmt') {
        const conditionText = convertExprToString(stmt.condition);
        let text = `while(${conditionText}) {\n`;
        if (stmt.body && typeof stmt.body === 'object' && stmt.body.type === 'Block') {
            text += convertASTBlockToString(stmt.body);
        }
        text += '}';
        return text;
    } else if (stmt.type === 'ForStmt') {
        let text = 'for(';
        if (stmt.init) {
            text += convertASTStatementToString(stmt.init);
        }
        text += '; ';
        if (stmt.condition) {
            text += convertExprToString(stmt.condition);
        }
        text += '; ';
        if (stmt.increment) {
            text += convertExprToString(stmt.increment);
        }
        text += ') {\n';
        if (stmt.body && typeof stmt.body === 'object' && stmt.body.type === 'Block') {
            text += convertASTBlockToString(stmt.body);
        }
        text += '}';
        return text;
    }
    
    return '';
}

/**
 * Обрабатывает AST блок (массив statements)
 */
function processASTBlock(
    block: ASTBlock,
    parentNode: FlowchartNode | null,
    createNode: NodeCreator,
    createConnection: ConnectionCreator
): ProcessBlockResult {
    if (!block || typeof block !== 'object' || block.type !== 'Block') {
        return { nodes: [], exitNodes: parentNode ? [parentNode] : [] };
    }
    
    const statements = block.statements || [];
    if (!Array.isArray(statements)) {
        return { nodes: [], exitNodes: parentNode ? [parentNode] : [] };
    }
    
    const blockNodes: FlowchartNode[] = [];
    let previousExitNodes: (FlowchartNode | NodeExitPoint)[] = parentNode ? [parentNode] : [];
    
    // Обрабатываем каждый statement в порядке массива
    for (let i = 0; i < statements.length; i++) {
        const stmt = statements[i];
        if (!stmt || typeof stmt !== 'object') continue;
        
        const result = processASTStatement(stmt, previousExitNodes, parentNode, createNode, createConnection);
        if (result.nodes && result.nodes.length > 0) {
            blockNodes.push(...result.nodes);
            // Используем exitNodes, если они есть, иначе последнюю ноду
            // Важно: даже если exitNodes содержит NodeExitPoint (как у while цикла), его нужно использовать
            if (result.exitNodes && result.exitNodes.length > 0) {
                previousExitNodes = result.exitNodes;
            } else {
                previousExitNodes = [result.nodes[result.nodes.length - 1]];
            }
        } else if (result.exitNodes && result.exitNodes.length > 0) {
            // Даже если nodes пустой, но есть exitNodes (редкий случай), обновляем previousExitNodes
            previousExitNodes = result.exitNodes;
        }
    }
    
    return {
        nodes: blockNodes,
        exitNodes: previousExitNodes
    };
}

/**
 * Обрабатывает statement или блок
 */
function processASTStatementOrBlock(
    stmtOrBlock: ASTStatement | ASTBlock,
    parentNode: FlowchartNode | null,
    createNode: NodeCreator,
    createConnection: ConnectionCreator
): ProcessBlockResult {
    if (!stmtOrBlock || typeof stmtOrBlock !== 'object') {
        return { nodes: [], exitNodes: parentNode ? [parentNode] : [] };
    }
    
    // Если это Block, обрабатываем как блок
    if (stmtOrBlock.type === 'Block') {
        return processASTBlock(stmtOrBlock, parentNode, createNode, createConnection);
    }
    
    // Иначе обрабатываем как statement
    return processASTStatement(
        stmtOrBlock as ASTStatement,
        parentNode ? [parentNode] : [],
        parentNode,
        createNode,
        createConnection
    );
}

/**
 * Обрабатывает один AST statement
 */
function processASTStatement(
    stmt: ASTStatement | ASTBlock,
    previousExitNodes: (FlowchartNode | NodeExitPoint)[],
    parentNode: FlowchartNode | null,
    createNode: NodeCreator,
    createConnection: ConnectionCreator
): ProcessBlockResult {
    if (!stmt || typeof stmt !== 'object') {
        return { nodes: [], exitNodes: previousExitNodes };
    }
    
    // Если это Block, обрабатываем его
    if (stmt.type === 'Block') {
        return processASTBlock(stmt as ASTBlock, parentNode, createNode, createConnection);
    }
    
    const stmtType = stmt.type;
    let currentNode: FlowchartNode | null = null;
    let exitNodes: (FlowchartNode | NodeExitPoint)[] = [];
    
    if (stmtType === 'VarDeclStmt') {
        // Объявление переменной: int x = 5; или const float PI = 3.14;
        const varType = stmt.varType || '';
        const varName = stmt.name || '';
        // Проверяем, есть ли const в типе (может быть "const float" или просто "float")
        let text = varType ? `${varType} ${varName}` : varName;
        if (stmt.initializer) {
            const initText = convertExprToString(stmt.initializer);
            text += ` = ${initText}`;
        }
        text += ';';
        
        const size = calculateNodeSize(text, 'process');
        currentNode = createNode('process', truncateText(text, 50), text, size.width, size.height, undefined, {
            astElement: stmt,
            language: 'c',
            nodeType: stmtType
        });
        
        // Соединяем с предыдущими
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
    } else if (stmtType === 'AssignStmt') {
        // Присваивание: x = 5; или x += 5; или x++;
        const target = stmt.target || '';
        const operator = convertOperatorToSymbol(stmt.operator || '=');
        const value = convertExprToString(stmt.value);
        
        // Для инкремента/декремента формат: x++ или ++x
        let text: string;
        if (operator === '++' || operator === '--') {
            // Проверяем, постфиксная или префиксная операция
            if (stmt.value && typeof stmt.value === 'object' && stmt.value.type === 'UnaryOp') {
                const unaryOp = stmt.value;
                const isPostfix = unaryOp.postfix || false;
                text = isPostfix ? `${target}${operator};` : `${operator}${target};`;
            } else {
                text = `${target}${operator};`;
            }
        } else {
            text = `${target} ${operator} ${value};`;
        }
        
        const size = calculateNodeSize(text, 'process');
        currentNode = createNode('process', truncateText(text, 50), text, size.width, size.height, undefined, {
            astElement: stmt,
            language: 'c',
            nodeType: stmtType
        });
        
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
    } else if (stmtType === 'IfStmt') {
        // Условие if
        const conditionText = convertExprToString(stmt.condition);
        const displayText = `if(${conditionText})`;
        const size = calculateNodeSize(displayText, 'decision');
        currentNode = createNode('decision', truncateText(displayText, 50), displayText, size.width, size.height, undefined, {
            astElement: stmt,
            language: 'c',
            nodeType: stmtType
        });
        
        // Соединяем условие с предыдущими
        previousExitNodes.forEach(prevExit => {
            const normalized = normalizeExitNode(prevExit);
            const prevNode = normalized.node;
            if (prevNode && prevNode !== currentNode && prevNode !== parentNode) {
                const fromPort = normalized.port || 'bottom';
                const label = normalized.label || '';
                createConnection(prevNode, currentNode, fromPort, 'top', label);
            }
        });
        
        let trueExitNode: FlowchartNode | NodeExitPoint | null = currentNode;
        let falseExitNode: FlowchartNode | NodeExitPoint | null = currentNode;
        
        // Обрабатываем thenBranch
        if (stmt.thenBranch && typeof stmt.thenBranch === 'object') {
            const thenResult = processASTStatementOrBlock(stmt.thenBranch, currentNode, createNode, createConnection);
            const thenNodes = thenResult.nodes || [];
            const thenExitNodes = thenResult.exitNodes || [];
            
            if (thenNodes.length > 0) {
                createConnection(currentNode, thenNodes[0], 'left', 'top', 'true');
                trueExitNode = thenExitNodes.length > 0 ? thenExitNodes[0] : thenNodes[thenNodes.length - 1];
                exitNodes.push(...(thenExitNodes.length > 0 ? thenExitNodes : [trueExitNode]));
            }
        }
        
        // Обрабатываем elseBranch
        if (stmt.elseBranch && typeof stmt.elseBranch === 'object') {
            const elseResult = processASTStatementOrBlock(stmt.elseBranch, currentNode, createNode, createConnection);
            const elseNodes = elseResult.nodes || [];
            const elseExitNodes = elseResult.exitNodes || [];
            
            if (elseNodes.length > 0) {
                createConnection(currentNode, elseNodes[0], 'right', 'top', 'false');
                falseExitNode = elseExitNodes.length > 0 ? elseExitNodes[0] : elseNodes[elseNodes.length - 1];
                exitNodes.push(...(elseExitNodes.length > 0 ? elseExitNodes : [falseExitNode]));
            } else {
                exitNodes.push({ node: currentNode, port: 'right', label: 'false' });
            }
        } else {
            // Нет else - false ветка выходит из условия
            exitNodes.push({ node: currentNode, port: 'right', label: 'false' });
        }
        
        if (exitNodes.length === 0) exitNodes = [currentNode];
    } else if (stmtType === 'WhileStmt') {
        // Цикл while
        const conditionText = convertExprToString(stmt.condition);
        const displayText = `while(${conditionText})`;
        const size = calculateNodeSize(displayText, 'decision');
        currentNode = createNode('decision', truncateText(displayText, 50), displayText, size.width, size.height, undefined, {
            astElement: stmt,
            language: 'c',
            nodeType: stmtType
        });
        
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
        if (stmt.body && typeof stmt.body === 'object') {
            const bodyResult = processASTStatementOrBlock(stmt.body, currentNode, createNode, createConnection);
            const bodyNodes = bodyResult.nodes || [];
            const bodyExitNodes = bodyResult.exitNodes || [];
            
            if (bodyNodes.length > 0) {
                createConnection(currentNode, bodyNodes[0], 'left', 'top', 'true');
                // Соединяем последнюю ноду тела обратно к условию
                const lastNodes = bodyExitNodes.length > 0 
                    ? bodyExitNodes 
                    : [bodyNodes[bodyNodes.length - 1]];
                lastNodes.forEach(lastNode => {
                    const normalized = normalizeExitNode(lastNode);
                    const node = normalized.node;
                    if (node && node !== currentNode) {
                        createConnection(node, currentNode, 'bottom', 'top');
                    }
                });
            }
        }
        
        exitNodes = [{ node: currentNode, port: 'right', label: 'false' }];
    } else if (stmtType === 'ForStmt') {
        // Цикл for
        let conditionText = 'for (';
        if (stmt.init) {
            conditionText += convertASTStatementToString(stmt.init);
        }
        conditionText += '; ';
        if (stmt.condition) {
            conditionText += convertExprToString(stmt.condition);
        }
        conditionText += '; ';
        if (stmt.increment) {
            conditionText += convertExprToString(stmt.increment);
        }
        conditionText += ')';
        
        const size = calculateNodeSize(conditionText, 'decision');
        currentNode = createNode('decision', truncateText(conditionText, 50), conditionText, size.width, size.height, undefined, {
            astElement: stmt,
            language: 'c',
            nodeType: stmtType
        });
        
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
        if (stmt.body && typeof stmt.body === 'object') {
            const bodyResult = processASTStatementOrBlock(stmt.body, currentNode, createNode, createConnection);
            const bodyNodes = bodyResult.nodes || [];
            const bodyExitNodes = bodyResult.exitNodes || [];
            
            if (bodyNodes.length > 0) {
                createConnection(currentNode, bodyNodes[0], 'left', 'top', 'true');
                // Соединяем последнюю ноду тела обратно к условию
                const lastNodes = bodyExitNodes.length > 0 
                    ? bodyExitNodes 
                    : [bodyNodes[bodyNodes.length - 1]];
                lastNodes.forEach(lastNode => {
                    const normalized = normalizeExitNode(lastNode);
                    const node = normalized.node;
                    if (node && node !== currentNode) {
                        createConnection(node, currentNode, 'bottom', 'top');
                    }
                });
            }
        }
        
        exitNodes = [{ node: currentNode, port: 'right', label: 'false' }];
    } else if (stmtType === 'DoWhileStmt') {
        // Цикл do-while (аналог repeat-until)
        // Сначала обрабатываем тело
        const bodyResult = stmt.body && typeof stmt.body === 'object'
            ? processASTStatementOrBlock(stmt.body, parentNode, createNode, createConnection)
            : { nodes: [], exitNodes: [] };
        const bodyNodes = bodyResult.nodes || [];
        const bodyExitNodes = bodyResult.exitNodes || [];
        
        // Соединяем первый элемент тела с предыдущими элементами (previousExitNodes)
        if (bodyNodes.length > 0) {
            previousExitNodes.forEach(prevExit => {
                const normalized = normalizeExitNode(prevExit);
                const prevNode = normalized.node;
                if (prevNode && prevNode !== bodyNodes[0] && prevNode !== parentNode) {
                    const fromPort = normalized.port || 'bottom';
                    const label = normalized.label || '';
                    createConnection(prevNode, bodyNodes[0], fromPort, 'top', label);
                }
            });
        }
        
        // Создаем условие после тела
        const conditionText = convertExprToString(stmt.condition);
        const displayText = `do-while(${conditionText})`;
        const size = calculateNodeSize(displayText, 'decision');
        currentNode = createNode('decision', truncateText(displayText, 50), displayText, size.width, size.height, undefined, {
            astElement: stmt,
            language: 'c',
            nodeType: stmtType
        });
        
        if (bodyNodes.length > 0) {
            // Соединяем последнюю ноду тела с условием
            const lastNodes = bodyExitNodes.length > 0 ? bodyExitNodes : [bodyNodes[bodyNodes.length - 1]];
            lastNodes.forEach(lastNode => {
                const normalized = normalizeExitNode(lastNode);
                const node = normalized.node;
                if (node) {
                    createConnection(node, currentNode, 'bottom', 'top');
                }
            });
        } else {
            // Если тело пустое, соединяем с предыдущими
            previousExitNodes.forEach(prevExit => {
                const normalized = normalizeExitNode(prevExit);
                const prevNode = normalized.node;
                if (prevNode && prevNode !== currentNode && prevNode !== parentNode) {
                    const fromPort = normalized.port || 'bottom';
                    const label = normalized.label || '';
                    createConnection(prevNode, currentNode, fromPort, 'top', label);
                }
            });
        }
        
        // True ветка - возврат к первому элементу тела
        if (bodyNodes.length > 0) {
            createConnection(currentNode, bodyNodes[0], 'left', 'top', 'true');
            exitNodes = [{ node: bodyNodes[0], port: 'right', label: 'false' }];
        } else {
            exitNodes = [{ node: currentNode, port: 'right', label: 'false' }];
        }
        
        // Возвращаем ноды в правильном порядке: тело, затем условие
        return {
            nodes: [...bodyNodes, currentNode],
            exitNodes: exitNodes
        };
    } else if (stmtType === 'SwitchStmt') {
        // Switch statement
        const conditionText = convertExprToString(stmt.condition);
        const displayText = `switch(${conditionText})`;
        const size = calculateNodeSize(displayText, 'decision');
        currentNode = createNode('decision', truncateText(displayText, 50), displayText, size.width, size.height, undefined, {
            astElement: stmt,
            language: 'c',
            nodeType: stmtType
        });
        
        previousExitNodes.forEach(prevExit => {
            const normalized = normalizeExitNode(prevExit);
            const prevNode = normalized.node;
            if (prevNode && prevNode !== currentNode && prevNode !== parentNode) {
                const fromPort = normalized.port || 'bottom';
                const label = normalized.label || '';
                createConnection(prevNode, currentNode, fromPort, 'top', label);
            }
        });
        
        const cases = stmt.cases || [];
        const allCaseExitNodes: (FlowchartNode | NodeExitPoint)[] = [];
        
        for (const caseStmt of cases) {
            if (caseStmt && typeof caseStmt === 'object') {
                if (caseStmt.type === 'CaseStmt') {
                    const caseValue = caseStmt.value ? convertExprToString(caseStmt.value) : 'default';
                    const caseBody = caseStmt.body || [];
                    
                    if (Array.isArray(caseBody) && caseBody.length > 0) {
                        // Обрабатываем body как массив statements
                        const caseBlock: ASTBlock = { type: 'Block', statements: caseBody };
                        const caseResult = processASTBlock(caseBlock, currentNode, createNode, createConnection);
                        const caseNodes = caseResult.nodes || [];
                        const caseExitNodes = caseResult.exitNodes || [];
                        
                        if (caseNodes.length > 0) {
                            createConnection(currentNode, caseNodes[0], 'bottom', 'top', caseValue);
                            allCaseExitNodes.push(...(caseExitNodes.length > 0 ? caseExitNodes : [caseNodes[caseNodes.length - 1]]));
                        }
                    }
                } else if (caseStmt.type === 'DefaultStmt') {
                    const defaultBody = caseStmt.body || [];
                    
                    if (Array.isArray(defaultBody) && defaultBody.length > 0) {
                        const defaultBlock: ASTBlock = { type: 'Block', statements: defaultBody };
                        const defaultResult = processASTBlock(defaultBlock, currentNode, createNode, createConnection);
                        const defaultNodes = defaultResult.nodes || [];
                        const defaultExitNodes = defaultResult.exitNodes || [];
                        
                        if (defaultNodes.length > 0) {
                            createConnection(currentNode, defaultNodes[0], 'bottom', 'top', 'default');
                            allCaseExitNodes.push(...(defaultExitNodes.length > 0 ? defaultExitNodes : [defaultNodes[defaultNodes.length - 1]]));
                        }
                    }
                }
            }
        }
        
        exitNodes = allCaseExitNodes.length > 0 ? allCaseExitNodes : [currentNode];
    } else if (stmtType === 'FunctionDecl') {
        // Объявление функции
        const returnType = stmt.returnType || '';
        const funcName = stmt.name || '';
        const params = (stmt.parameters || []).map(p => `${p.type || ''} ${p.name || ''}`).join(', ');
        const text = `${returnType} ${funcName}(${params})`;
        
        // Для main функции используем особенный стиль (можно сделать её start-like или process с особым цветом)
        // Пока просто делаем её обычным process, но с увеличенным размером
        const isMain = funcName === 'main';
        const size = calculateNodeSize(text, 'process');
        
        // Формируем полный код функции для отображения в контекстном меню
        let functionBody = text + ' {\n';
        if (stmt.body && typeof stmt.body === 'object' && stmt.body.type === 'Block') {
            functionBody += convertASTBlockToString(stmt.body);
        }
        functionBody += '\n}';
        
        currentNode = createNode('process', truncateText(text, 60), text, size.width, size.height, functionBody, {
            astElement: stmt,
            language: 'c',
            nodeType: stmtType
        });
        
        // Добавляем метаданные для идентификации функции
        if (currentNode) {
            (currentNode as any).isFunction = true;
            (currentNode as any).functionName = funcName;
            // Прототип - это функция без body (body отсутствует или null)
            const hasBody = stmt.body && typeof stmt.body === 'object' && stmt.body.type === 'Block';
            if (!hasBody) {
                (currentNode as any).isPrototype = true;
            }
        }
        
        // Добавляем специальный класс или метаданные для main (будет использоваться в рендеринге)
        if (isMain && currentNode) {
            // Можно добавить флаг для особой стилизации
            (currentNode as any).isMainFunction = true;
        }
        
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
    } else if (stmtType === 'ExprStmt') {
        // Выражение-statement (может быть вызов функции, присваивание и т.д.)
        const expr = stmt.expression;
        if (!expr || typeof expr !== 'object') {
            return { nodes: [], exitNodes: previousExitNodes };
        }
        
        // Проверяем, является ли это вызовом функции (IO операция)
        if (expr.type === 'CallExpr') {
            const callee = expr.callee || '';
            const isOutput = callee === 'printf' || callee === 'cout' || callee === 'puts';
            const isInput = callee === 'scanf' || callee === 'cin' || callee === 'gets';
            const nodeType = isOutput ? 'output' : (isInput ? 'input' : 'process');
            
            const args = (expr.arguments || []).map(arg => convertExprToString(arg)).join(', ');
            const text = `${callee}(${args});`;
            
            const size = calculateNodeSize(text, nodeType);
            currentNode = createNode(nodeType, truncateText(text, 50), text, size.width, size.height, undefined, {
                astElement: stmt,
                language: 'c',
                nodeType: stmtType
            });
        } else {
            // Другое выражение
            const text = convertExprToString(expr) + ';';
            const size = calculateNodeSize(text, 'process');
            currentNode = createNode('process', truncateText(text, 50), text, size.width, size.height, undefined, {
                astElement: stmt,
                language: 'c',
                nodeType: stmtType
            });
        }
        
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
    } else if (stmtType === 'ReturnStmt') {
        // Return statement
        const valueText = stmt.value ? convertExprToString(stmt.value) : '';
        const text = valueText ? `return ${valueText};` : 'return;';
        const size = calculateNodeSize(text, 'process');
        currentNode = createNode('process', truncateText(text, 50), text, size.width, size.height, undefined, {
            astElement: stmt,
            language: 'c',
            nodeType: stmtType
        });
        
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
    } else if (stmtType === 'BreakStmt') {
        // Break statement
        currentNode = createNode('process', 'break;', 'break;', undefined, undefined, undefined, {
            astElement: stmt,
            language: 'c',
            nodeType: stmtType
        });
        
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
    } else if (stmtType === 'PreprocessorDirective') {
        // Директива препроцессора: #include, #define и т.д.
        const directive = (stmt as any).directive || '';
        const value = (stmt as any).value || '';
        let text = `#${directive}`;
        if (value) {
            text += ` ${value}`;
        }
        
        const size = calculateNodeSize(text, 'process');
        currentNode = createNode('process', truncateText(text, 50), text, size.width, size.height, undefined, {
            astElement: stmt,
            language: 'c',
            nodeType: stmtType
        });
        
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
    } else if (stmtType === 'StructDecl') {
        // Объявление структуры
        const structName = (stmt as any).name || '';
        const fields = (stmt as any).fields || [];
        const fieldsText = fields.map((f: any) => `${f.varType || ''} ${f.name || ''};`).join('\n');
        const text = `struct ${structName} {\n${fieldsText}\n};`;
        
        const size = calculateNodeSize(text, 'process');
        currentNode = createNode('process', truncateText(text, 60), text, size.width, size.height, undefined, {
            astElement: stmt,
            language: 'c',
            nodeType: stmtType
        });
        
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
    } else if (stmtType === 'TypedefDecl') {
        // Typedef объявление
        const typeName = (stmt as any).typeName || '';
        const alias = (stmt as any).alias || '';
        const text = `typedef ${typeName} ${alias};`;
        
        const size = calculateNodeSize(text, 'process');
        currentNode = createNode('process', truncateText(text, 50), text, size.width, size.height, undefined, {
            astElement: stmt,
            language: 'c',
            nodeType: stmtType
        });
        
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
    
    return {
        nodes: currentNode ? [currentNode] : [],
        exitNodes: exitNodes.length > 0 ? exitNodes : previousExitNodes
    };
}

/**
 * Парсит C/C++ программу (AST формат) в блок-схему
 */
export function parseCToFlowchart(
    jsonData: ASTProgram,
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
    
    const startNode = createNode('start', 'Начало', '');
    let lastMainNode: FlowchartNode = startNode;
    
    if (jsonData.body && typeof jsonData.body === 'object' && jsonData.body.type === 'Block') {
        const statements = jsonData.body.statements || [];
        
        if (statements.length === 0) {
            console.warn('[WARNING] Empty statements array! Parser may have failed to parse the code.');
            console.warn('[WARNING] JSON data:', JSON.stringify(jsonData, null, 2));
        }
        
        let previousExitNodes: (FlowchartNode | NodeExitPoint)[] = [startNode];
        let firstNode: FlowchartNode | null = null; // Для соединения с начальной нодой
        
        // Обрабатываем все statements в порядке массива
        for (const stmt of statements) {
            if (!stmt || typeof stmt !== 'object') continue;
            
            if (stmt.type === 'FunctionDecl') {
                // Создаем ноду для объявления функции
                const returnType = stmt.returnType || '';
                const funcName = stmt.name || '';
                const params = (stmt.parameters || []).map(p => `${p.type || ''} ${p.name || ''}`).join(', ');
                const funcText = `${returnType} ${funcName}(${params})`;
                
                const isMain = funcName === 'main';
                const size = calculateNodeSize(funcText, 'process');
                
                // Формируем полный код функции для отображения в контекстном меню
                let functionBody = funcText + ' {\n';
                if (stmt.body && typeof stmt.body === 'object' && stmt.body.type === 'Block') {
                    functionBody += convertASTBlockToString(stmt.body);
                }
                functionBody += '\n}';
                
                const funcNode = createNode('process', truncateText(funcText, 60), funcText, size.width, size.height, functionBody, {
                    astElement: stmt,
                    language: 'c',
                    nodeType: 'FunctionDecl'
                });
                
                // Добавляем метаданные для идентификации функции
                if (funcNode) {
                    (funcNode as any).isFunction = true;
                    (funcNode as any).functionName = funcName;
                    // Прототип - это функция без body (body отсутствует или null)
                    const hasBody = stmt.body && typeof stmt.body === 'object' && stmt.body.type === 'Block';
                    if (!hasBody) {
                        (funcNode as any).isPrototype = true;
                    }
                }
                
                // Для main функции добавляем флаг
                if (isMain) {
                    (funcNode as any).isMainFunction = true;
                }
                
                // Соединяем с предыдущими
                previousExitNodes.forEach(prevExit => {
                    const normalized = normalizeExitNode(prevExit);
                    const prevNode = normalized.node;
                    if (prevNode && prevNode !== funcNode && prevNode !== startNode) {
                        const fromPort = normalized.port || 'bottom';
                        const label = normalized.label || '';
                        createConnection(prevNode, funcNode, fromPort, 'top', label);
                    } else if (prevNode === startNode) {
                        createConnection(startNode, funcNode);
                    }
                });
                
                // Если это main() и у неё есть body, обрабатываем тело функции
                if (funcName === 'main' && stmt.body && typeof stmt.body === 'object' && stmt.body.type === 'Block') {
                    const mainBodyResult = processASTBlock(stmt.body, funcNode, createNode, createConnection);
                    const mainBodyNodes = mainBodyResult.nodes || [];
                    if (mainBodyNodes.length > 0) {
                        createConnection(funcNode, mainBodyNodes[0]);
                        lastMainNode = mainBodyNodes[mainBodyNodes.length - 1];
                        previousExitNodes = mainBodyResult.exitNodes || [lastMainNode];
                    } else {
                        lastMainNode = funcNode;
                        previousExitNodes = [funcNode];
                    }
                } else {
                    lastMainNode = funcNode;
                    previousExitNodes = [funcNode];
                }
            } else {
                // Другие типы statements (VarDeclStmt на верхнем уровне, StructDecl, PreprocessorDirective и т.д.)
                const result = processASTStatement(stmt, previousExitNodes, startNode, createNode, createConnection);
                if (result.nodes && result.nodes.length > 0) {
                    // Если это первая нода после start, соединяем её с start
                    if (firstNode === null && result.nodes.length > 0) {
                        firstNode = result.nodes[0];
                        // Соединяем start с первой нодой (если она еще не соединена)
                        const normalizedFirst = normalizeExitNode(result.nodes[0]);
                        if (normalizedFirst.node && normalizedFirst.node !== startNode) {
                            // Проверяем, нет ли уже соединения
                            const hasConnection = connections.some(c => c.from === startNode.id && c.to === normalizedFirst.node!.id);
                            if (!hasConnection) {
                                createConnection(startNode, normalizedFirst.node);
                            }
                        }
                    }
                    previousExitNodes = result.exitNodes || [result.nodes[result.nodes.length - 1]];
                    lastMainNode = result.nodes[result.nodes.length - 1];
                }
            }
        }
    } else {
        // Если структура не распознана, создаем простую блок-схему
        lastMainNode = startNode;
    }
    
    // Создаем конечную ноду и соединяем с последней нодой программы
    const endNode = createNode('end', 'Конец', '');
    if (lastMainNode.id !== endNode.id) {
        createConnection(lastMainNode, endNode);
    }
    
    return { nodes, connections };
}

