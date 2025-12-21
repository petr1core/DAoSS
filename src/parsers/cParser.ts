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
        return text;
    } else if (stmt.type === 'AssignStmt') {
        const target = stmt.target || '';
        const operator = stmt.operator || '=';
        const value = convertExprToString(stmt.value);
        return `${target} ${operator} ${value}`;
    } else if (stmt.type === 'ExprStmt' && stmt.expression) {
        return convertExprToString(stmt.expression);
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
            previousExitNodes = result.exitNodes || [result.nodes[result.nodes.length - 1]];
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
    stmt: ASTStatement,
    previousExitNodes: (FlowchartNode | NodeExitPoint)[],
    parentNode: FlowchartNode | null,
    createNode: NodeCreator,
    createConnection: ConnectionCreator
): ProcessBlockResult {
    if (!stmt || typeof stmt !== 'object') {
        return { nodes: [], exitNodes: previousExitNodes };
    }
    
    const stmtType = stmt.type;
    let currentNode: FlowchartNode | null = null;
    let exitNodes: (FlowchartNode | NodeExitPoint)[] = [];
    
    if (stmtType === 'VarDeclStmt') {
        // Объявление переменной: int x = 5;
        const varType = stmt.varType || '';
        const varName = stmt.name || '';
        let text = `${varType} ${varName}`;
        if (stmt.initializer) {
            const initText = convertExprToString(stmt.initializer);
            text += ` = ${initText}`;
        }
        text += ';';
        
        currentNode = createNode('process', truncateText(text, 30), text);
        
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
        // Присваивание: x = 5;
        const target = stmt.target || '';
        const operator = stmt.operator || '=';
        const value = convertExprToString(stmt.value);
        const text = `${target} ${operator} ${value};`;
        
        currentNode = createNode('process', truncateText(text, 30), text);
        
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
        currentNode = createNode('decision', truncateText(conditionText, 30), conditionText);
        
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
        currentNode = createNode('decision', truncateText(conditionText, 30), conditionText);
        
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
        
        currentNode = createNode('decision', truncateText(conditionText, 30), conditionText);
        
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
        
        // Создаем условие после тела
        const conditionText = convertExprToString(stmt.condition);
        currentNode = createNode('decision', truncateText(conditionText, 30), conditionText);
        
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
        currentNode = createNode('decision', truncateText(`Case: ${conditionText}`, 30), conditionText);
        
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
        
        currentNode = createNode('process', truncateText(text, 50), text);
        
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
            
            currentNode = createNode(nodeType, truncateText(text, 30), text);
        } else {
            // Другое выражение
            const text = convertExprToString(expr) + ';';
            currentNode = createNode('process', truncateText(text, 30), text);
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
        
        currentNode = createNode('process', truncateText(text, 30), text);
        
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
        currentNode = createNode('process', 'break;', 'break;');
        
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
    } else if (
        stmtType === 'StructDecl' || 
        stmtType === 'TypedefDecl' || 
        stmtType === 'PreprocessorDirective'
    ) {
        // Эти типы можно пропустить или показать как process nodes
        // Пока пропускаем
        return { nodes: [], exitNodes: previousExitNodes };
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
        
        // Обрабатываем все statements в порядке массива
        for (const stmt of statements) {
            if (!stmt || typeof stmt !== 'object') continue;
            
            if (stmt.type === 'FunctionDecl') {
                // Создаем ноду для объявления функции
                const returnType = stmt.returnType || '';
                const funcName = stmt.name || '';
                const params = (stmt.parameters || []).map(p => `${p.type || ''} ${p.name || ''}`).join(', ');
                const funcText = `${returnType} ${funcName}(${params})`;
                
                const funcNode = createNode('process', truncateText(funcText, 50), funcText);
                
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
                // Другие типы statements (VarDeclStmt на верхнем уровне, StructDecl и т.д.)
                const result = processASTStatement(stmt, previousExitNodes, startNode, createNode, createConnection);
                if (result.nodes && result.nodes.length > 0) {
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

