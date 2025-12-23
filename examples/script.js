// Главный объект приложения
const FlowchartApp = {
    // Состояние приложения
    state: {
        currentJson: null,
        graph: null,
        selectedNode: null,
        zoomLevel: 1,
        nodeCounter: 0,
        nodePositions: new Map(),
        history: [],
        historyIndex: -1
    },


    // Инициализация при загрузке страницы
    init() {
        console.log('🚀 Flowchart App Initializing...');

        // Устанавливаем обработчики событий
        this.setupEventListeners();

        // Загружаем пример JSON
        this.loadSample();

        // Инициализируем пустую диаграмму
        this.initEmptyFlowchart();

        // Обновляем статус
        this.updateStatus('Ready. Paste your Pascal JSON and click "Load Flowchart"', 'info');
        // Добавляем панель инструментов
        this.addToolbar();
        
        //this.addContextMenu();
    },
    // Настройка обработчиков событий
    setupEventListeners() {
        // Кнопка загрузки блок-схемы
        document.getElementById('loadBtn').addEventListener('click', () => {
            this.loadFlowchart();
        });

        // Кнопка загрузки примера
        document.getElementById('sampleBtn').addEventListener('click', () => {
            this.loadSample();
            this.loadFlowchart();
        });

        // Кнопка экспорта
        document.getElementById('exportBtn').addEventListener('click', () => {
            this.exportAsPNG();
        });

        // Валидация JSON
        document.getElementById('validateBtn').addEventListener('click', () => {
            this.validateJson();
        });

        // Форматирование JSON
        document.getElementById('formatBtn').addEventListener('click', () => {
            this.formatJson();
        });

        // Управление зумом
        document.getElementById('zoomIn').addEventListener('click', () => {
            this.zoomIn();
        });

        document.getElementById('zoomOut').addEventListener('click', () => {
            this.zoomOut();
        });

        document.getElementById('zoomReset').addEventListener('click', () => {
            this.resetZoom();
        });

        // Зум колесиком мыши
        const svg = document.getElementById('flowchart');
        svg.addEventListener('wheel', (e) => {
            e.preventDefault();
            if (e.deltaY < 0) {
                this.zoomIn();
            } else {
                this.zoomOut();
            }
        });

    },
    // Панель инструментов для редактирования
    addToolbar() {
        // Создаем контейнер для тулбара
        const toolbarContainer = document.createElement('div');
        toolbarContainer.className = 'toolbar-container';
        toolbarContainer.style.cssText = `
            position: absolute;
            top: 70px;
            left: 20px;
            z-index: 1000;
            background: white;
            padding: 10px;
            border-radius: 8px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
            display: none;
        `;

        toolbarContainer.innerHTML = `
            <div style="margin-bottom: 10px; font-weight: bold; color: #4A00E0;">📝 Tools</div>
            <div style="display: flex; flex-direction: column; gap: 5px;">
                <button class="tool-btn" id="tbAddNode">+ Add Node</button>
                <button class="tool-btn" id="tbUndo">↶ Undo</button>
                <button class="tool-btn" id="tbRedo">↷ Redo</button>
                <button class="tool-btn" id="tbSaveJson">💾 Save JSON</button>
                <button class="tool-btn" id="tbGenerateCode">⚡ Generate Code</button>
            </div>
        `;

        document.querySelector('.flowchart-container').appendChild(toolbarContainer);

        // Стили для кнопок
        const style = document.createElement('style');
        style.textContent = `
            .tool-btn {
                padding: 6px 12px;
                background: #4A00E0;
                color: white;
                border: none;
                border-radius: 4px;
                cursor: pointer;
                font-size: 12px;
                transition: background 0.3s;
            }
            .tool-btn:hover {
                background: #8E2DE2;
            }
            .node-dialog {
                position: fixed;
                top: 50%;
                left: 50%;
                transform: translate(-50%, -50%);
                background: white;
                padding: 20px;
                border-radius: 10px;
                box-shadow: 0 5px 30px rgba(0,0,0,0.3);
                z-index: 10000;
                min-width: 300px;
            }
            .node-dialog h3 {
                margin: 0 0 15px 0;
                color: #4A00E0;
            }
            .dialog-content {
                display: flex;
                flex-direction: column;
                gap: 10px;
            }
            .dialog-content label {
                font-weight: bold;
                color: #666;
                font-size: 14px;
            }
            .dialog-content select,
            .dialog-content textarea {
                padding: 8px;
                border: 2px solid #ddd;
                border-radius: 4px;
                font-family: inherit;
            }
            .dialog-content textarea {
                min-height: 60px;
                resize: vertical;
            }
            .dialog-buttons {
                display: flex;
                gap: 10px;
                margin-top: 15px;
            }
            .dialog-buttons button {
                flex: 1;
                padding: 10px;
                border: none;
                border-radius: 5px;
                cursor: pointer;
                font-weight: bold;
            }
            .dialog-buttons .primary {
                background: #4A00E0;
                color: white;
            }
            .dialog-buttons button:not(.primary) {
                background: #f5f5f5;
                color: #666;
            }
        `;
        document.head.appendChild(style);

        // Обработчики
        document.getElementById('tbAddNode').addEventListener('click', () => {
            this.showAddNodeDialog();
            toolbarContainer.style.display = 'none';
        });

        document.getElementById('tbUndo').addEventListener('click', () => {
            this.undo();
            toolbarContainer.style.display = 'none';
        });

        document.getElementById('tbRedo').addEventListener('click', () => {
            this.redo();
            toolbarContainer.style.display = 'none';
        });

        document.getElementById('tbSaveJson').addEventListener('click', () => {
            this.saveCurrentJson();
            toolbarContainer.style.display = 'none';
        });

        document.getElementById('tbGenerateCode').addEventListener('click', () => {
            this.generateCode();
            toolbarContainer.style.display = 'none';
        });

        // Показываем/скрываем тулбар по клику на иконку
        const toggleBtn = document.createElement('button');
        toggleBtn.innerHTML = '⚙️';
        toggleBtn.className = 'toolbar-toggle';
        toggleBtn.style.cssText = `
            position: absolute;
            top: 70px;
            left: 20px;
            z-index: 999;
            width: 40px;
            height: 40px;
            background: #4A00E0;
            color: white;
            border: none;
            border-radius: 50%;
            cursor: pointer;
            font-size: 20px;
            display: flex;
            align-items: center;
            justify-content: center;
            box-shadow: 0 2px 10px rgba(0,0,0,0.2);
        `;

        toggleBtn.addEventListener('click', (e) => {
            e.stopPropagation();
            toolbarContainer.style.display =
                toolbarContainer.style.display === 'none' ? 'block' : 'none';
        });

        document.querySelector('.flowchart-container').appendChild(toggleBtn);

        // Скрываем тулбар при клике вне его
        document.addEventListener('click', (e) => {
            if (!toolbarContainer.contains(e.target) && e.target !== toggleBtn) {
                toolbarContainer.style.display = 'none';
            }
        });
    },

    // Диалог добавления узла
    showAddNodeDialog() {
        // Удаляем существующий диалог если есть
        const existingDialog = document.querySelector('.node-dialog');
        if (existingDialog) existingDialog.remove();

        const dialog = document.createElement('div');
        dialog.className = 'node-dialog';
        dialog.innerHTML = `
        <h3>➕ Add New Node</h3>
        <div class="dialog-content">
            <label>Section:</label>
            <select id="nodeSection">
                <option value="functionBlock">Functions/Procedures</option>
                <option value="mainBlock">Main Block</option>
                <option value="constantBlock">Constants</option>
                <option value="variableBlock">Variables</option>
            </select>
            
             <div id="nodeTypeContainer" style="display: none;">
                <label>Node Type:</label>
                <select id="nodeType"></select>
            </div>
            
            <label>Content:</label>
            <textarea id="nodeContent" placeholder="Enter node content..." rows="4"></textarea>
            <div id="contentHint" style="color: #666; font-size: 12px; margin-top: 5px;"></div>

            <div class="dialog-buttons">
                <button id="cancelAddBtn">Cancel</button>
                <button id="confirmAddBtn" class="primary">Add Node</button>
            </div>
        </div>
    `;

        document.body.appendChild(dialog);

        const sectionSelect = document.getElementById('nodeSection');
        const typeContainer = document.getElementById('nodeTypeContainer');
        const typeSelect = document.getElementById('nodeType');
        const contentHint = document.getElementById('contentHint');
        const contentLabel = document.getElementById('contentLabel');
        const contentTextarea = document.getElementById('nodeContent');

        console.log('Dialog elements:', {
            sectionSelect: !!sectionSelect,
            typeContainer: !!typeContainer,
            typeSelect: !!typeSelect,
            contentHint: !!contentHint,
            contentLabel: !!contentLabel,
            contentTextarea: !!contentTextarea
        });

        // Объект с типами для каждой секции
        const sectionConfigs = {
            functionBlock: {
                types: [
                    { value: 'function', label: 'Function' },
                    { value: 'procedure', label: 'Procedure' }
                ],
                hint: 'Examples:\n• function Add(a,b: integer): integer;\n• procedure PrintMessage(msg: string);',
                placeholder: 'Enter function/procedure declaration...'
            },
            constantBlock: {
                types: null, // Без типа
                hint: 'Examples: • PI = 3.14159; • MAX_SIZE = 100;',
                placeholder: 'Enter constant declaration...'
            },
            variableBlock: {
                types: null, // Без типа
                hint: 'Examples: • x, y: integer; • name: string; • arr: array[1..10] of real;',
                placeholder: 'Enter variable declaration...'
            },
            mainBlock: {
                types: [
                    { value: 'assign', label: 'Assignment' },
                    { value: 'io', label: 'IO Operation' },
                    { value: 'if', label: 'If Condition' },
                    { value: 'while', label: 'While Loop' },
                    { value: 'for', label: 'For Loop' },
                    { value: 'until', label: 'Repeat-Until' },
                    { value: 'caseOf', label: 'Case Of' }
                ],
                hint: 'Examples:• Assignment: "x := 5 + 3" • IO: "Writeln(\'Hello\')" • If: "if x > 0 then" • While: "while i < 10 do"',
                placeholder: 'Enter statement...'
            }
        };

        // Функция обновления UI в зависимости от выбранной секции
        const updateUIForSection = (section) => {
            const config = sectionConfigs[section];

            if (!config) {
                console.error('No config for section:', section);
                return;
            }

            // Обновляем типы
            if (config.types && typeSelect) {
                typeContainer.style.display = 'block';
                typeSelect.innerHTML = '';
                config.types.forEach(type => {
                    const option = document.createElement('option');
                    option.value = type.value;
                    option.textContent = type.label;
                    typeSelect.appendChild(option);
                });
            } else if (typeContainer) {
                typeContainer.style.display = 'none';
            }

            // Обновляем подсказку и placeholder
            if (contentHint) {
                contentHint.innerHTML = config.hint; // Используем innerHTML для HTML
            }

            if (contentTextarea) {
                contentTextarea.placeholder = config.placeholder;
            }

            // Обновляем label
            if (contentLabel) {
                contentLabel.textContent = section === 'functionBlock' ?
                    'Declaration:' : 'Content:';
            }

            console.log('UI updated for section:', section);
        };

        // Инициализируем для первой секции
        updateUIForSection(sectionSelect.value);

        // Обработчик изменения секции
        sectionSelect.addEventListener('change', (e) => {
            updateUIForSection(e.target.value);
        });

        document.getElementById('cancelAddBtn').addEventListener('click', () => dialog.remove());
        
        document.getElementById('confirmAddBtn').addEventListener('click', () => {
            const section = document.getElementById('nodeSection').value;
            const type = typeContainer.style.display !== 'none' ? typeSelect.value : null;
            const content = document.getElementById('nodeContent').value;

            if (!content) {
                alert('Please enter content for the node');
                return;
            }

            // Валидация в зависимости от секции
            if (!this.validateNodeInput(section, type, content)) {
                return;
            }

            this.addNodeToJson(section, type, content);
            dialog.remove();
            //this.loadFlowchart();

        });

        dialog.addEventListener('click', (e) => {
            if (e.target === dialog) {
                dialog.remove();
            }
        });
        // Закрытие по ESC
        const escHandler = (e) => {
            if (e.key === 'Escape') {
                dialog.remove();
                document.removeEventListener('keydown', escHandler);
            }
        };
        document.addEventListener('keydown', escHandler);

        // Удаляем обработчик при закрытии диалога
        dialog.addEventListener('remove', () => {
            document.removeEventListener('keydown', escHandler);
        });

    },

    validateNodeInput(section, type, content) {
        // Базовые проверки
        if (!content.trim()) {
            alert('Content cannot be empty');
            return false;
        }

        switch (section) {
            case 'functionBlock':
                // Проверяем, что это объявление функции/процедуры
                if (!content.toLowerCase().includes('function') &&
                    !content.toLowerCase().includes('procedure')) {
                    alert('Function/Procedure must contain "function" or "procedure" keyword');
                    return false;
                }
                if (!content.endsWith(';')) {
                    alert('Function/Procedure declaration must end with semicolon (;)');
                    return false;
                }
                break;

            case 'constantBlock':
                // Проверяем формат константы
                if (!content.includes('=') && !content.includes(':')) {
                    alert('Constant should contain "=" or ":" (e.g., PI = 3.14 or PI: real = 3.14)');
                    return false;
                }
                if (!content.endsWith(';')) {
                    alert('Constant declaration must end with semicolon (;)');
                    return false;
                }
                break;

            case 'variableBlock':
                // Проверяем формат переменной
                if (!content.includes(':')) {
                    alert('Variable declaration must contain type (e.g., x: integer)');
                    return false;
                }
                if (!content.endsWith(';')) {
                    alert('Variable declaration must end with semicolon (;)');
                    return false;
                }
                break;

            case 'mainBlock':
                // Для mainBlock тип обязателен
                if (!type) {
                    alert('Please select node type for main block');
                    return false;
                }

                // Дополнительные проверки в зависимости от типа
                switch (type) {
                    case 'if':
                    case 'while':
                    case 'for':
                    case 'until':
                        if (!content.toLowerCase().includes('then') &&
                            !content.toLowerCase().includes('do') &&
                            !content.toLowerCase().includes('until')) {
                            alert(`${type} statement should contain "then", "do" or "until" keyword`);
                            return false;
                        }
                        break;

                    case 'assign':
                        if (!content.includes(':=')) {
                            alert('Assignment must contain ":=" operator');
                            return false;
                        }
                        break;

                    case 'io':
                        if (!content.toLowerCase().includes('read') &&
                            !content.toLowerCase().includes('write')) {
                            alert('IO operation should contain "Read" or "Write" keyword');
                            return false;
                        }
                        break;

                    case 'caseOf':
                        if (!content.toLowerCase().includes('case')) {
                            alert('Case statement should contain "case" keyword');
                            return false;
                        }
                        break;
                }
                break;
        }

        return true;
    },

    addNodeToJson(section, type, content) {
        if (!this.state.currentJson) {
            alert('Please load a JSON first');
            return;
        }

        // Сохраняем в историю
        this.saveToHistory('before add node');

        const sections = this.state.currentJson.program.sections;

        // Убедимся, что секция существует
        if (!sections[section]) {
            sections[section] = {};
        }

        // Генерируем новый ключ
        let maxKey = -1;
        Object.keys(sections[section]).forEach(key => {
            if (key.startsWith('expr')) {
                const num = parseInt(key.replace('expr', ''));
                if (!isNaN(num) && num > maxKey) maxKey = num;
            }
        });

        const newKey = `expr${maxKey + 1}`;

        // Создаем узел в зависимости от секции
        switch (section) {
            case 'functionBlock':
                // Для functionBlock создаем объект с declaration и телом
                const isFunction = content.toLowerCase().includes('function');
                sections[section][newKey] = {
                    type: isFunction ? 'function' : 'procedure',
                    declaration: content.endsWith(';') ? content.slice(0, -1) : content,
                    body: {
                        expr0: {
                            type: 'io',
                            value: '// TODO: Add body statements'
                        }
                    }
                };
                break;

            case 'constantBlock':
            case 'variableBlock':
                // Для constantBlock и variableBlock - просто строка
                sections[section][newKey] = content.endsWith(';') ? content : content + ';';
                break;

            case 'mainBlock':
                // Для mainBlock создаем объект с типом
                const newNode = {
                    type: type,
                    value: content
                };

                // Добавляем дополнительные поля в зависимости от типа
                if (type === 'if' || type === 'while' || type === 'for') {
                    newNode.condition = content;
                    newNode.body = {};
                } else if (type === 'until') {
                    newNode.condition = content;
                    newNode.body = {
                        expr0: {
                            type: 'io',
                            value: '// TODO: Add repeat body'
                        }
                    };
                } else if (type === 'caseOf') {
                    newNode.compareValue = content.split(' ')[1] || 'value';
                    newNode.body = {
                        "branch 0": {
                            conditionValues: "1 2 3",
                            todo: {
                                expr0: {
                                    type: 'io',
                                    value: '// TODO: Add case body'
                                }
                            }
                        }
                    };
                }

                sections[section][newKey] = newNode;
                break;
        }

        //Обновляем JSON в textarea
        document.getElementById('jsonInput').value = JSON.stringify(
            this.state.currentJson, null, 2
        );

        // Отладочная информация
        console.log('=== ADD NODE SUCCESS ===');
        console.log('Section:', section);
        console.log('Type:', type || 'N/A');
        console.log('Key:', newKey);
        console.log('Content:', content);
        console.log('Node structure:', sections[section][newKey]);


        this.saveToHistory(`added ${type} node to ${section}`);
        this.updateStatus(`✓ Added ${type} node to ${section}`, 'success');

        // Загружаем обновленный граф
        this.loadFlowchart();

        // Отладочный вывод
        console.log(`Added node to ${section}:`, sections[section][newKey]);
        console.log('Updated sections:', sections);
    },

    // Сохранение в историю
    saveToHistory(action) {
        if (!this.state.currentJson) return;

        // Удаляем все после текущего индекса (если сделали новое изменение после undo)
        this.state.history = this.state.history.slice(0, this.state.historyIndex + 1);

        // Добавляем новый снимок
        this.state.history.push({
            action: action,
            timestamp: new Date().toISOString(),
            data: JSON.parse(JSON.stringify(this.state.currentJson)) // deep copy
        });

        this.state.historyIndex = this.state.history.length - 1;

        console.log(`History saved: ${action}, index: ${this.state.historyIndex}, total: ${this.state.history.length}`);
    },

    // Отмена действия
    undo() {
        if (this.state.historyIndex > 0) {
            this.state.historyIndex--;
            this.state.currentJson = JSON.parse(JSON.stringify(
                this.state.history[this.state.historyIndex].data
            ));

            // Обновляем JSON в textarea
            document.getElementById('jsonInput').value = JSON.stringify(
                this.state.currentJson, null, 2
            );

            this.loadFlowchart();
            this.updateStatus(`↶ Undo: ${this.state.history[this.state.historyIndex + 1]?.action}`, 'info');
        } else {
            this.updateStatus('Nothing to undo', 'warning');
        }
    },

    // Повтор действия
    redo() {
        if (this.state.historyIndex < this.state.history.length - 1) {
            this.state.historyIndex++;
            this.state.currentJson = JSON.parse(JSON.stringify(
                this.state.history[this.state.historyIndex].data
            ));

            // Обновляем JSON в textarea
            document.getElementById('jsonInput').value = JSON.stringify(
                this.state.currentJson, null, 2
            );

            this.loadFlowchart();
            this.updateStatus(`↷ Redo: ${this.state.history[this.state.historyIndex]?.action}`, 'info');
        } else {
            this.updateStatus('Nothing to redo', 'warning');
        }
    },

    // Сохранение текущего JSON
    saveCurrentJson() {
        if (!this.state.currentJson) {
            alert('No JSON to save');
            return;
        }

        const jsonStr = JSON.stringify(this.state.currentJson, null, 2);
        const blob = new Blob([jsonStr], { type: 'application/json' });
        const url = URL.createObjectURL(blob);

        const a = document.createElement('a');
        a.href = url;
        a.download = 'pascal-flowchart.json';
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);

        this.updateStatus('✓ JSON saved to file', 'success');
    },

    // Генерация кода
    generateCode() {
        if (!this.state.currentJson) {
            alert('No JSON to generate code from');
            return;
        }

        // Здесь должен быть вызов твоего генератора кода
        // Пока просто покажем сообщение
        this.updateStatus('⚡ Code generation would call your PascalCodeGenerator', 'info');

        // В реальности:
        // const code = pascalCodeGenerator.generatePascal(this.state.currentJson);
        // console.log('Generated code:', code);
        // Можно показать в новом окне или сохранить в файл
    },

    // Загрузка примера JSON Pascal
    loadSample() {
        const sampleJson = {            
            "program": {
                "name": "program qq",
                    "sections": {
                    "constantBlock": {
                        "expr2": "PI : real = 3.1415926;"
                    },
                    "functionBlock": {
                        "expr0": {
                            "body": {
                                "expr0": {
                                    "type": "assign",
                                        "value": "AddNumbers := a + b"
                                }
                            },
                            "declaration": "function AddNumbers ( a , b : integer ) : integer",
                                "type": "function"
                        },
                        "expr1": {
                            "body": {
                                "expr0": {
                                    "type": "io",
                                        "value": "Writeln ( 'Hello,' , name , '!' )"
                                }
                            },
                            "declaration": "procedure GreetUser ( name : string )",
                                "type": "procedure"
                        }
                    },
                    "mainBlock": {
                        "expr10": {
                            "body": {
                                "expr0": {
                                    "type": "io",
                                        "value": "Writeln ( 'Yes,if 1 ' )"
                                },
                                "expr1": {
                                    "type": "io",
                                        "value": "Writeln ( 'Yes,if 2 ' )"
                                }
                            },
                            "condition": "if 5 mod 3 > 0 then",
                                "type": "if"
                        },
                        "expr11": {
                            "body": {
                                "branch 0": {
                                    "conditionValues": "1 2 3",
                                        "todo": {
                                        "expr0": {
                                            "type": "io",
                                                "value": "Writeln ( 'Switch works 1' )"
                                        }
                                    }
                                },
                                "branch 1": {
                                    "conditionValues": "4 5",
                                        "todo": {
                                        "expr0": {
                                            "type": "io",
                                                "value": "Writeln ( 'Switch works 2' )"
                                        }
                                    }
                                },
                                "branch 2": {
                                    "conditionValues": "else",
                                        "todo": {
                                        "expr0": {
                                            "type": "io",
                                                "value": "Writeln ( 'Switch no works' )"
                                        }
                                    }
                                }
                            },
                            "compareValue": "num1",
                                "type": "caseOf"
                        },
                        "expr12": {
                            "body": {
                                "expr0": {
                                    "type": "io",
                                        "value": "Writeln ( 'Yes,if 1 ' )"
                                },
                                "expr1": {
                                    "type": "io",
                                        "value": "Writeln ( 'Yes,if 2 ' )"
                                }
                            },
                            "condition": "if 5 mod 3 > 0 then",
                                "type": "if"
                        },
                        "expr13": {
                            "body": {
                                "expr0": {
                                    "type": "io",
                                        "value": "Writeln ( 'No,else 1 ' )"
                                },
                                "expr1": {
                                    "type": "io",
                                        "value": "Writeln ( 'No,else 2 ' )"
                                }
                            },
                            "condition": "else",
                                "type": "else"
                        },
                        "expr14": {
                            "body": {
                                "expr0": {
                                    "type": "io",
                                        "value": "Writeln ( 'Yes,if 1 ' )"
                                },
                                "expr1": {
                                    "type": "io",
                                        "value": "Writeln ( 'Yes,if 2 ' )"
                                }
                            },
                            "condition": "if 5 mod 3 > 0 then",
                                "type": "if"
                        },
                        "expr15": {
                            "body": {
                                "expr0": {
                                    "type": "io",
                                        "value": "Writeln ( 'No,else 1 ' )"
                                },
                                "expr1": {
                                    "type": "io",
                                        "value": "Writeln ( 'No,else 2 ' )"
                                }
                            },
                            "condition": "else",
                                "type": "else"
                        },
                        "expr16": {
                            "type": "assign",
                                "value": "res2 := 'Hello world'"
                        },
                        "expr17": {
                            "type": "assign",
                                "value": "num1 := 2"
                        },
                        "expr18": {
                            "type": "io",
                                "value": "Writeln ( 'From table ' , num1 )"
                        },
                        "expr19": {
                            "body": {
                                "expr0": {
                                    "type": "io",
                                        "value": "Writeln ( 'Pim' )"
                                },
                                "expr1": {
                                    "body": {
                                        "expr0": {
                                            "type": "io",
                                                "value": "Writeln ( 'Pam' )"
                                        }
                                    },
                                    "condition": "if PI <> num1 then",
                                        "type": "if"
                                }
                            },
                            "condition": "if PI <> num1 then",
                                "type": "if"
                        },
                        "expr20": {
                            "body": {
                                "expr0": {
                                    "type": "io",
                                        "value": "Writeln ( 'Pum' )"
                                }
                            },
                            "condition": "else",
                                "type": "else"
                        },
                        "expr21": {
                            "body": {
                                "expr0": {
                                    "type": "io",
                                        "value": "Writeln ( 'Pim' )"
                                },
                                "expr1": {
                                    "body": {
                                        "expr0": {
                                            "type": "io",
                                                "value": "Writeln ( 'Yes,if 1 ' )"
                                        },
                                        "expr1": {
                                            "type": "io",
                                                "value": "Writeln ( 'Yes,if 2 ' )"
                                        }
                                    },
                                    "condition": "if 5 mod 3 > 0 then",
                                        "type": "if"
                                },
                                "expr2": {
                                    "body": {
                                        "expr0": {
                                            "type": "io",
                                                "value": "Writeln ( 'No,else 1 ' )"
                                        },
                                        "expr1": {
                                            "type": "io",
                                                "value": "Writeln ( 'No,else 2 ' )"
                                        }
                                    },
                                    "condition": "else",
                                        "type": "else"
                                }
                            },
                            "condition": "if PI <> num1 then",
                                "type": "if"
                        },
                        "expr22": {
                            "body": {
                                "expr0": {
                                    "type": "io",
                                        "value": "Writeln ( 'Pum' )"
                                }
                            },
                            "condition": "else",
                                "type": "else"
                        },
                        "expr23": {
                            "body": {
                                "expr0": {
                                    "type": "io",
                                        "value": "Write ( '3' )"
                                }
                            },
                            "condition": "for i := 1 to 8 do",
                                "type": "for"
                        },
                        "expr24": {
                            "body": {
                                "expr0": {
                                    "type": "io",
                                        "value": "Write ( 'Yes' )"
                                },
                                "expr1": {
                                    "type": "assign",
                                        "value": "num1 := num1 + 1"
                                }
                            },
                            "condition": "while num1 < 6 do",
                                "type": "while"
                        },
                        "expr25": {
                            "body": {
                                "expr0": {
                                    "type": "io",
                                        "value": "Writeln ( 'Yes,if 1 ' )"
                                },
                                "expr1": {
                                    "type": "io",
                                        "value": "Writeln ( 'Yes,if 2 ' )"
                                }
                            },
                            "condition": "if 5 mod 3 > 0 then",
                                "type": "if"
                        },
                        "expr26": {
                            "body": {
                                "expr0": {
                                    "type": "io",
                                        "value": "Writeln ( 'No,else 1 ' )"
                                },
                                "expr1": {
                                    "type": "io",
                                        "value": "Writeln ( 'No,else 2 ' )"
                                }
                            },
                            "condition": "else",
                                "type": "else"
                        },
                        "expr27": {
                            "body": {
                                "expr0": {
                                    "type": "io",
                                        "value": "Write ( '3' )"
                                },
                                "expr1": {
                                    "type": "assign",
                                        "value": "num1 := num1 + 1"
                                }
                            },
                            "condition": "until num1 < 7",
                                "type": "until"
                        },
                        "expr28": {
                            "type": "assign",
                                "value": "num1 := num1 + 3"
                        },
                        "expr6": {
                            "type": "io",
                                "value": "Read ( Res )"
                        },
                        "expr7": {
                            "type": "io",
                                "value": "Writeln ( 'From Read ' , Res )"
                        },
                        "expr8": {
                            "type": "assign",
                            "value": "num1 := 12 div 2"
                        },
                        "expr9": {
                            "type": "assign",
                            "value": "num1 := AddNumbers ( PI , PI )"
                        }
                    },
                    "variableBlock": {
                        "expr3": "num1 , num2 , i : integer;",
                        "expr4": "Res , d : real;",
                        "expr5": "res2 : string;"
                    }
                }
            
        }
        };

        document.getElementById('jsonInput').value = JSON.stringify(sampleJson, null, 2);
        this.updateStatus('Sample JSON loaded', 'success');
    },

    // Валидация JSON
    validateJson() {
        try {
            const jsonText = document.getElementById('jsonInput').value;
            JSON.parse(jsonText);
            this.updateStatus('✓ JSON is valid', 'success');
            return true;
        } catch (error) {
            this.updateStatus(`✗ JSON Error: ${error.message}`, 'error');
            return false;
        }
    },

    // Форматирование JSON
    formatJson() {
        try {
            const jsonText = document.getElementById('jsonInput').value;
            const parsed = JSON.parse(jsonText);
            document.getElementById('jsonInput').value = JSON.stringify(parsed, null, 2);
            this.updateStatus('JSON formatted', 'success');
        } catch (error) {
            this.updateStatus(`Cannot format: ${error.message}`, 'error');
        }
    },

    // Основная функция загрузки блок-схемы
    loadFlowchart() {
        try {
            // Получаем JSON из текстового поля
            const jsonText = document.getElementById('jsonInput').value;
            if (!jsonText.trim()) {
                this.updateStatus('Please enter some JSON', 'warning');
                return;
            }

            // Парсим JSON
            this.state.currentJson = JSON.parse(jsonText);

            // Проверяем структуру
            if (!this.state.currentJson.program || !this.state.currentJson.program.sections) {
                throw new Error('Invalid Pascal JSON structure');
            }

            // Инициализируем историю если пустая
            if (this.state.history.length === 0) {
                this.saveToHistory('initial load');
            }

            // Обновляем статус
            this.updateStatus('Processing Pascal JSON...', 'info');

            // Создаем граф из JSON
            this.createGraphFromJson();

            // Отладочная информация
            this.debugGraph();   

            // Рендерим граф
            const success = this.renderFlowchart();

            if (success) {
                // Обновляем статистику
                this.updateStats();
                this.updateStatus('✓ Flowchart loaded successfully', 'success');
            } else {
                this.updateStatus('⚠ Flowchart rendered with errors', 'warning');
            }

        } catch (error) {
            this.updateStatus(`Error: ${error.message}`, 'error');
            console.error('Flowchart Error:', error);
        }
    },

    // Создание графа из JSON Pascal
    createGraphFromJson() {
        const jsonData = this.state.currentJson;
        this.state.nodeCounter = 0;

        // Создаем новый граф dagre
        const g = new dagreD3.graphlib.Graph()
            .setGraph({
                rankdir: 'TB',          // Направление сверху вниз
                marginx: 30,
                marginy: 30,
                nodesep: 60,
                edgesep: 20,
                ranksep: 80
            })
            .setDefaultEdgeLabel(() => ({}));

        // 1. Добавляем стартовый узел
        g.setNode('start', {
            label: '🚀 START\n' + (jsonData.program.name || 'Program'),
            shape: 'circle',
            style: 'fill: #4CAF50; stroke: #2E7D32; stroke-width: 3px;',
            class: 'start-node',
            padding: 15
        });

        const sections = jsonData.program.sections;
        let lastNodeId = 'start';

        // 2. Обрабатываем блоки в порядке Pascal
        const sectionOrder = [
            { key: 'functionBlock', title: '📦 FUNCTIONS', color: '#2196F3' },
            { key: 'constantBlock', title: '🔢 CONSTANTS', color: '#FF9800' },
            { key: 'variableBlock', title: '📊 VARIABLES', color: '#9C27B0' },
            { key: 'mainBlock', title: '⚡ MAIN CODE', color: '#F44336' }
        ];

        // Обработка всех секций кроме mainBlock (они простые)
        sectionOrder.forEach((sectionInfo) => {
            if (sectionInfo.key === 'mainBlock') return; // mainBlock обработаем отдельно

            const section = sections[sectionInfo.key];
            if (!section || Object.keys(section).length === 0) return;

            // Обрабатываем элементы внутри секции
            Object.entries(section).forEach(([exprKey, exprValue]) => {
                this.state.nodeCounter++;
                const nodeId = `node_${this.state.nodeCounter}`;

                // Формируем метку для узла
                const nodeLabel = this.formatNodeLabel(exprKey, exprValue, sectionInfo.key);

                // Определяем форму узла
                let nodeShape = 'rect';
                if (sectionInfo.key === 'functionBlock') {
                    nodeShape = 'ellipse';
                }

                // Определяем стиль
                const nodeStyle = this.getNodeStyle(exprValue, sectionInfo.key);

                // Создаем конфигурацию узла
                const nodeConfig = {
                    label: nodeLabel || 'Unknown',
                    style: nodeStyle,
                    shape: nodeShape,
                    class: `node ${sectionInfo.key}`,
                    padding: 12,
                    data: {
                        id: exprKey,
                        value: exprValue,
                        section: sectionInfo.key
                    }
                };

                // Добавляем узел
                g.setNode(nodeId, nodeConfig);

                // Добавляем связь от предыдущего узла
                if (lastNodeId && lastNodeId !== nodeId) {
                    g.setEdge(lastNodeId, nodeId, {
                        arrowhead: 'vee',
                        style: 'stroke: #666; stroke-width: 2px; fill: none;',
                        arrowheadStyle: 'fill: #666; stroke: #666;'
                    });
                }

                lastNodeId = nodeId;
            });
        });

        // 3. ОСОБАЯ ОБРАБОТКА mainBlock с вложенными структурами
        if (sections.mainBlock) {
            lastNodeId = this.processMainBlock(g, sections.mainBlock, lastNodeId);
        }

        // 4. Добавляем конечный узел
        g.setNode('end', {
            label: '🏁 END',
            shape: 'circle',
            style: 'fill: #F44336; stroke: #D32F2F; stroke-width: 3px;',
            class: 'end-node',
            padding: 15
        });

        // Связываем последний узел с конечным
        if (lastNodeId && lastNodeId !== 'end') {
            g.setEdge(lastNodeId, 'end', {
                arrowhead: 'vee',
                style: 'stroke: #666; stroke-width: 2px; fill: none;',
                arrowheadStyle: 'fill: #666; stroke: #666;'
            });
        }

        this.state.graph = g;
        console.log('Graph created with nodes:', g.nodes());
    },

    // Обработка mainBlock с вложенными структурами
    processMainBlock(g, mainBlock, startNodeId) {
        let lastNodeId = startNodeId;

        // Сортируем выражения по ключам (expr0, expr1, ...)
        const sortedExpressions = Object.entries(mainBlock)
            .sort(([keyA], [keyB]) => {
                const numA = parseInt(keyA.replace('expr', ''));
                const numB = parseInt(keyB.replace('expr', ''));
                return numA - numB;
            });

        // Обрабатываем пары if-else
        for (let i = 0; i < sortedExpressions.length; i++) {
            const [exprKey, exprValue] = sortedExpressions[i];
            if (!exprValue) continue;

            this.state.nodeCounter++;
            const nodeId = `node_${this.state.nodeCounter}`;

            // Проверяем пару if-else
            const nextExpr = sortedExpressions[i + 1];
            const isIfWithElse = exprValue.type === 'if' &&
                nextExpr &&
                nextExpr[1].type === 'else';

            if (isIfWithElse) {
                // Обрабатываем if-else как одну структуру
                lastNodeId = this.processIfElseBlock(g, exprKey, exprValue,
                    nextExpr[0], nextExpr[1],
                    lastNodeId, nodeId);
                i++; // Пропускаем else блок
            } else if (exprValue.type === 'until') {
                // Обработка repeat..until (аналог do-while)
                lastNodeId = this.processUntilBlock(g, exprKey, exprValue, lastNodeId, nodeId);
            } else if (exprValue.type === 'if' || exprValue.type === 'while' || exprValue.type === 'for') {
                lastNodeId = this.processConditionalBlock(g, exprKey, exprValue, lastNodeId, nodeId);
            } else if (exprValue.type === 'caseOf') {
                lastNodeId = this.processCaseBlock(g, exprKey, exprValue, lastNodeId, nodeId);
            } else {
                // Простой узел (assign, io)
                lastNodeId = this.processSimpleNode(g, exprKey, exprValue, lastNodeId, nodeId);
            }
        }

        return lastNodeId;
    },

    // Обработка if-else блока
    processIfElseBlock(g, ifKey, ifValue, elseKey, elseValue, prevNodeId, ifNodeId) {
        // Создаем узел IF
        g.setNode(ifNodeId, {
            label: `🔀 IF\n${ifValue.condition || 'No condition'}`,
            shape: 'diamond',
            style: this.getNodeStyle(ifValue, 'mainBlock'),
            class: 'node mainBlock if',
            padding: 12,
            data: {
                id: ifKey,
                value: ifValue,
                section: 'mainBlock'
            }
        });

        // Связь с предыдущим узлом
        if (prevNodeId) {
            g.setEdge(prevNodeId, ifNodeId, {
                arrowhead: 'vee',
                style: 'stroke: #666; stroke-width: 2px; fill: none;',
                arrowheadStyle: 'fill: #666; stroke: #666;'
            });
        }

        // ========== THEN BRANCH ==========
        let lastThenNodeId = ifNodeId;

        if (ifValue.body && Object.keys(ifValue.body).length > 0) {
            // Узел начала THEN блока
            this.state.nodeCounter++;
            const beginThenId = `node_${this.state.nodeCounter}`;
            g.setNode(beginThenId, {
                label: '▶ THEN',
                shape: 'rect',
                style: 'fill: #C8E6C9; stroke: #4CAF50; stroke-width: 2px;',
                class: 'node then-start',
                padding: 8
            });

            g.setEdge(ifNodeId, beginThenId, {
                label: 'true',
                style: 'stroke: #4CAF50; stroke-width: 2px; fill: none;',
                arrowhead: 'vee',
                arrowheadStyle: 'fill: #4CAF50; stroke: #4CAF50;',
                labelStyle: 'fill: #4CAF50; font-weight: bold;'
            });

            lastThenNodeId = beginThenId;

            // Обрабатываем тело THEN
            const thenEntries = Object.entries(ifValue.body);
            for (const [bodyKey, bodyValue] of thenEntries) {
                this.state.nodeCounter++;
                const bodyNodeId = `node_${this.state.nodeCounter}`;

                if (bodyValue.type === 'if' || bodyValue.type === 'while' || bodyValue.type === 'for') {
                    lastThenNodeId = this.processConditionalBlock(g, bodyKey, bodyValue, lastThenNodeId, bodyNodeId);
                } else {
                    lastThenNodeId = this.processSimpleNode(g, bodyKey, bodyValue, lastThenNodeId, bodyNodeId);
                }
            }

            // Конец THEN блока
            this.state.nodeCounter++;
            const endThenId = `node_${this.state.nodeCounter}`;
            g.setNode(endThenId, {
                label: '◀ END THEN',
                shape: 'rect',
                style: 'fill: #C8E6C9; stroke: #4CAF50; stroke-width: 2px;',
                class: 'node then-end',
                padding: 8
            });

            g.setEdge(lastThenNodeId, endThenId, {
                style: 'stroke: #4CAF50; stroke-width: 2px; fill: none;',
                arrowhead: 'vee',
                arrowheadStyle: 'fill: #4CAF50; stroke: #4CAF50;'
            });

            lastThenNodeId = endThenId;
        }

        // ========== ELSE BRANCH ==========
        this.state.nodeCounter++;
        const elseNodeId = `node_${this.state.nodeCounter}`;

        // Узел ELSE
        g.setNode(elseNodeId, {
            label: '🚫 ELSE',
            shape: 'rect',
            style: 'fill: #FFCDD2; stroke: #F44336; stroke-width: 2px;',
            class: 'node else-start',
            padding: 8,
            data: {
                id: elseKey,
                value: elseValue,
                section: 'mainBlock'
            }
        });

        // Связь IF → ELSE (false ветка)
        g.setEdge(ifNodeId, elseNodeId, {
            label: 'false',
            style: 'stroke: #F44336; stroke-width: 2px; fill: none;',
            arrowhead: 'vee',
            arrowheadStyle: 'fill: #F44336; stroke: #F44336;',
            labelStyle: 'fill: #F44336; font-weight: bold;'
        });

        let lastElseNodeId = elseNodeId;

        // Обрабатываем тело ELSE
        if (elseValue.body && Object.keys(elseValue.body).length > 0) {
            const elseEntries = Object.entries(elseValue.body);
            for (const [bodyKey, bodyValue] of elseEntries) {
                this.state.nodeCounter++;
                const bodyNodeId = `node_${this.state.nodeCounter}`;

                if (bodyValue.type === 'if' || bodyValue.type === 'while' || bodyValue.type === 'for') {
                    lastElseNodeId = this.processConditionalBlock(g, bodyKey, bodyValue, lastElseNodeId, bodyNodeId);
                } else {
                    lastElseNodeId = this.processSimpleNode(g, bodyKey, bodyValue, lastElseNodeId, bodyNodeId);
                }
            }

            // Конец ELSE блока
            this.state.nodeCounter++;
            const endElseId = `node_${this.state.nodeCounter}`;
            g.setNode(endElseId, {
                label: '◀ END ELSE',
                shape: 'rect',
                style: 'fill: #FFCDD2; stroke: #F44336; stroke-width: 2px;',
                class: 'node else-end',
                padding: 8
            });

            g.setEdge(lastElseNodeId, endElseId, {
                style: 'stroke: #F44336; stroke-width: 2px; fill: none;',
                arrowhead: 'vee',
                arrowheadStyle: 'fill: #F44336; stroke: #F44336;'
            });

            lastElseNodeId = endElseId;
        }

        // ========== MERGE POINT ==========
        // Объединяем THEN и ELSE ветки
        this.state.nodeCounter++;
        const mergeNodeId = `node_${this.state.nodeCounter}`;

        g.setNode(mergeNodeId, {
            label: '🔀 MERGE',
            shape: 'circle',
            style: 'fill: #9C27B0; stroke: #7B1FA2; stroke-width: 2px;',
            class: 'node merge-point',
            padding: 8
        });

        // Связи от концов веток к merge
        g.setEdge(lastThenNodeId, mergeNodeId, {
            style: 'stroke: #4CAF50; stroke-width: 2px; fill: none;',
            arrowhead: 'vee',
            arrowheadStyle: 'fill: #4CAF50; stroke: #4CAF50;'
        });

        g.setEdge(lastElseNodeId, mergeNodeId, {
            style: 'stroke: #F44336; stroke-width: 2px; fill: none;',
            arrowhead: 'vee',
            arrowheadStyle: 'fill: #F44336; stroke: #F44336;'
        });

        return mergeNodeId;
    },

    // Обработка repeat..until блока
    processUntilBlock(g, exprKey, exprValue, prevNodeId, nodeId) {
        // Узел начала REPEAT
        g.setNode(nodeId, {
            label: '🔁 REPEAT',
            shape: 'rect',
            style: 'fill: #FFF3E0; stroke: #FF9800; stroke-width: 3px;',
            class: 'node mainBlock repeat',
            padding: 12,
            data: {
                id: exprKey,
                value: exprValue,
                section: 'mainBlock'
            }
        });

        if (prevNodeId) {
            g.setEdge(prevNodeId, nodeId, {
                arrowhead: 'vee',
                style: 'stroke: #666; stroke-width: 2px; fill: none;',
                arrowheadStyle: 'fill: #666; stroke: #666;'
            });
        }

        let lastBodyNodeId = nodeId;

        // Обрабатываем тело REPEAT
        if (exprValue.body && Object.keys(exprValue.body).length > 0) {
            const bodyEntries = Object.entries(exprValue.body);

            for (const [bodyKey, bodyValue] of bodyEntries) {
                this.state.nodeCounter++;
                const bodyNodeId = `node_${this.state.nodeCounter}`;

                if (bodyValue.type === 'if' || bodyValue.type === 'while' || bodyValue.type === 'for') {
                    lastBodyNodeId = this.processConditionalBlock(g, bodyKey, bodyValue, lastBodyNodeId, bodyNodeId);
                } else {
                    lastBodyNodeId = this.processSimpleNode(g, bodyKey, bodyValue, lastBodyNodeId, bodyNodeId);
                }
            }
        }

        // Узел условия UNTIL
        this.state.nodeCounter++;
        const untilNodeId = `node_${this.state.nodeCounter}`;

        g.setNode(untilNodeId, {
            label: `⏹ UNTIL\n${exprValue.condition || 'No condition'}`,
            shape: 'diamond',
            style: 'fill: #FFECB3; stroke: #FF9800; stroke-width: 2px;',
            class: 'node until-condition',
            padding: 12
        });

        g.setEdge(lastBodyNodeId, untilNodeId, {
            style: 'stroke: #FF9800; stroke-width: 2px; fill: none;',
            arrowhead: 'vee',
            arrowheadStyle: 'fill: #FF9800; stroke: #FF9800;'
        });

        // Обратная связь (пока условие не выполнено)
        g.setEdge(untilNodeId, nodeId, {
            label: 'false',
            style: 'stroke: #FF9800; stroke-width: 2px; fill: none; stroke-dasharray: 5,5;',
            arrowhead: 'vee',
            arrowheadStyle: 'fill: #FF9800; stroke: #FF9800;',
            labelStyle: 'fill: #FF9800; font-weight: bold;'
        });

        // Выход из цикла (когда условие выполнено)
        this.state.nodeCounter++;
        const exitNodeId = `node_${this.state.nodeCounter}`;

        g.setNode(exitNodeId, {
            label: '⏭ EXIT REPEAT',
            shape: 'rect',
            style: 'fill: #FFF3E0; stroke: #FF9800; stroke-width: 2px;',
            class: 'node repeat-exit',
            padding: 8
        });

        g.setEdge(untilNodeId, exitNodeId, {
            label: 'true',
            style: 'stroke: #FF9800; stroke-width: 2px; fill: none;',
            arrowhead: 'vee',
            arrowheadStyle: 'fill: #FF9800; stroke: #FF9800;',
            labelStyle: 'fill: #FF9800; font-weight: bold;'
        });

        return exitNodeId;
    },
    // Обработка условных блоков (if, while, for)
    processConditionalBlock(g, exprKey, exprValue, prevNodeId, nodeId) {
        // Создаем узел условия
        const conditionLabel = exprValue.type === 'if' ?
            `🔀 IF\n${exprValue.condition || 'No condition'}` :
            exprValue.type === 'while' ? `🔄 WHILE\n${exprValue.condition || 'No condition'}` :
                `➰ FOR\n${exprValue.condition || 'No condition'}`;

        g.setNode(nodeId, {
            label: conditionLabel,
            shape: exprValue.type === 'if' ? 'diamond' : 'rect',
            style: this.getNodeStyle(exprValue, 'mainBlock'),
            class: `node mainBlock conditional ${exprValue.type}`,
            padding: 12,
            data: {
                id: exprKey,
                value: exprValue,
                section: 'mainBlock'
            }
        });

        // Связь с предыдущим узлом
        if (prevNodeId) {
            g.setEdge(prevNodeId, nodeId, {
                arrowhead: 'vee',
                style: 'stroke: #666; stroke-width: 2px; fill: none;',
                arrowheadStyle: 'fill: #666; stroke: #666;'
            });
        }

        // Обрабатываем тело условия
        if (exprValue.body && Object.keys(exprValue.body).length > 0) {
            let lastBodyNodeId = nodeId;

            // Добавляем узел "Begin body"
            this.state.nodeCounter++;
            const beginBodyId = `node_${this.state.nodeCounter}`;
            g.setNode(beginBodyId, {
                label: '▶ BEGIN BODY',
                shape: 'rect',
                style: 'fill: #E8EAF6; stroke: #3F51B5; stroke-width: 2px;',
                class: 'node body-start',
                padding: 8,
                data: { type: 'body-start' }
            });

            g.setEdge(nodeId, beginBodyId, {
                label: exprValue.type === 'if' ? 'then' : 'do',
                style: 'stroke: #4CAF50; stroke-width: 2px; fill: none;',
                arrowhead: 'vee',
                arrowheadStyle: 'fill: #4CAF50; stroke: #4CAF50;',
                labelStyle: 'fill: #4CAF50; font-weight: bold;'
            });

            lastBodyNodeId = beginBodyId;

            // Рекурсивно обрабатываем вложенные выражения
            if (exprValue.body.expr0 || exprValue.body.expr1) {
                const bodyEntries = Object.entries(exprValue.body);

                for (const [bodyKey, bodyValue] of bodyEntries) {
                    this.state.nodeCounter++;
                    const bodyNodeId = `node_${this.state.nodeCounter}`;

                    if (bodyValue.type === 'if' || bodyValue.type === 'while' || bodyValue.type === 'for') {
                        lastBodyNodeId = this.processConditionalBlock(g, bodyKey, bodyValue, lastBodyNodeId, bodyNodeId);
                    } else {
                        lastBodyNodeId = this.processSimpleNode(g, bodyKey, bodyValue, lastBodyNodeId, bodyNodeId);
                    }
                }
            }

            // Добавляем узел "End body"
            this.state.nodeCounter++;
            const endBodyId = `node_${this.state.nodeCounter}`;
            g.setNode(endBodyId, {
                label: '◀ END BODY',
                shape: 'rect',
                style: 'fill: #E8EAF6; stroke: #3F51B5; stroke-width: 2px;',
                class: 'node body-end',
                padding: 8,
                data: { type: 'body-end' }
            });

            g.setEdge(lastBodyNodeId, endBodyId, {
                style: 'stroke: #666; stroke-width: 2px; fill: none;',
                arrowhead: 'vee',
                arrowheadStyle: 'fill: #666; stroke: #666;'
            });

            // Для циклов добавляем обратную связь
            if (exprValue.type === 'while' || exprValue.type === 'for') {
                g.setEdge(endBodyId, nodeId, {
                    label: 'loop',
                    style: 'stroke: #FF9800; stroke-width: 2px; fill: none; stroke-dasharray: 5,5;',
                    arrowhead: 'vee',
                    arrowheadStyle: 'fill: #FF9800; stroke: #FF9800;',
                    labelStyle: 'fill: #FF9800; font-weight: bold;'
                });

                // Добавляем узел "Continue after loop"
                this.state.nodeCounter++;
                const afterLoopId = `node_${this.state.nodeCounter}`;
                g.setNode(afterLoopId, {
                    label: '⏭ AFTER LOOP',
                    shape: 'rect',
                    style: 'fill: #FFF3E0; stroke: #FF9800; stroke-width: 2px;',
                    class: 'node after-loop',
                    padding: 8,
                    data: { type: 'after-loop' }
                });

                g.setEdge(nodeId, afterLoopId, {
                    label: 'exit',
                    style: 'stroke: #FF9800; stroke-width: 2px; fill: none;',
                    arrowhead: 'vee',
                    arrowheadStyle: 'fill: #FF9800; stroke: #FF9800;',
                    labelStyle: 'fill: #FF9800; font-weight: bold;'
                });

                return afterLoopId;
            }

            return endBodyId;
        }

        return nodeId;
    },

    // Обработка case блока
    processCaseBlock(g, exprKey, exprValue, prevNodeId, nodeId) {
        g.setNode(nodeId, {
            label: `🔘 CASE OF\n${exprValue.compareValue || 'No value'}`,
            shape: 'ellipse',
            style: this.getNodeStyle(exprValue, 'mainBlock'),
            class: 'node mainBlock case',
            padding: 12,
            data: {
                id: exprKey,
                value: exprValue,
                section: 'mainBlock'
            }
        });

        if (prevNodeId) {
            g.setEdge(prevNodeId, nodeId, {
                arrowhead: 'vee',
                style: 'stroke: #666; stroke-width: 2px; fill: none;',
                arrowheadStyle: 'fill: #666; stroke: #666;'
            });
        }

        // Обрабатываем ветки case
        if (exprValue.body) {
            const branches = Object.entries(exprValue.body);
            branches.forEach(([branchKey, branchValue], index) => {
                this.state.nodeCounter++;
                const branchNodeId = `node_${this.state.nodeCounter}`;

                g.setNode(branchNodeId, {
                    label: `📌 ${branchValue.conditionValues || 'default'}`,
                    shape: 'rect',
                    style: 'fill: #FCE4EC; stroke: #E91E63; stroke-width: 2px;',
                    class: 'node case-branch',
                    padding: 10,
                    data: {
                        branch: branchKey,
                        values: branchValue
                    }
                });

                // Связь от case к ветке
                g.setEdge(nodeId, branchNodeId, {
                    style: 'stroke: #E91E63; stroke-width: 2px; fill: none;',
                    arrowhead: 'vee',
                    arrowheadStyle: 'fill: #E91E63; stroke: #E91E63;'
                });

                // Обрабатываем тело ветки
                if (branchValue.todo) {
                    let lastTodoNodeId = branchNodeId;
                    const todoEntries = Object.entries(branchValue.todo);

                    todoEntries.forEach(([todoKey, todoValue]) => {
                        this.state.nodeCounter++;
                        const todoNodeId = `node_${this.state.nodeCounter}`;

                        lastTodoNodeId = this.processSimpleNode(g, todoKey, todoValue, lastTodoNodeId, todoNodeId);
                    });

                    // После ветки - объединяем обратно
                    this.state.nodeCounter++;
                    const mergeNodeId = `node_${this.state.nodeCounter}`;
                    g.setNode(mergeNodeId, {
                        label: '🔗 MERGE',
                        shape: 'circle',
                        style: 'fill: #9C27B0; stroke: #7B1FA2; stroke-width: 2px;',
                        class: 'node merge',
                        padding: 8,
                        data: { type: 'merge' }
                    });

                    g.setEdge(lastTodoNodeId, mergeNodeId, {
                        style: 'stroke: #9C27B0; stroke-width: 2px; fill: none;',
                        arrowhead: 'vee',
                        arrowheadStyle: 'fill: #9C27B0; stroke: #9C27B0;'
                    });

                    // От merge обратно к основному потоку
                    this.state.nodeCounter++;
                    const continueNodeId = `node_${this.state.nodeCounter}`;
                    g.setNode(continueNodeId, {
                        label: '➡ CONTINUE',
                        shape: 'rect',
                        style: 'fill: #E1BEE7; stroke: #9C27B0; stroke-width: 2px;',
                        class: 'node continue',
                        padding: 8,
                        data: { type: 'continue' }
                    });

                    g.setEdge(mergeNodeId, continueNodeId, {
                        style: 'stroke: #9C27B0; stroke-width: 2px; fill: none;',
                        arrowhead: 'vee',
                        arrowheadStyle: 'fill: #9C27B0; stroke: #9C27B0;'
                    });

                    return continueNodeId;
                }
            });
        }

        return nodeId;
    },

    // Обработка простых узлов (assign, io)
    processSimpleNode(g, exprKey, exprValue, prevNodeId, nodeId) {
        const nodeLabel = this.formatNodeLabel(exprKey, exprValue, 'mainBlock');

        g.setNode(nodeId, {
            label: nodeLabel,
            shape: 'rect',
            style: this.getNodeStyle(exprValue, 'mainBlock'),
            class: `node mainBlock ${exprValue.type || 'simple'}`,
            padding: 12,
            data: {
                id: exprKey,
                value: exprValue,
                section: 'mainBlock'
            }
        });

        if (prevNodeId) {
            g.setEdge(prevNodeId, nodeId, {
                arrowhead: 'vee',
                style: 'stroke: #666; stroke-width: 2px; fill: none;',
                arrowheadStyle: 'fill: #666; stroke: #666;'
            });
        }

        return nodeId;
    },

    // Форматирование метки узла
    formatNodeLabel(key, value, sectionType) {
        // Защита от undefined
        if (!value) return `❓ Unknown\n${key}`;

        if (sectionType === 'constantBlock' || sectionType === 'variableBlock') {
            return `📝 ${key}\n${value}`;
        }

        if (sectionType === 'functionBlock') {
            if (typeof value === 'object') {
                return `📦 ${value.type || 'function'}\n${value.declaration || key}`;
            }
            return `📦 ${value}`;
        }

        if (sectionType === 'mainBlock') {
            if (value.type === 'io') {
                const ioType = value.value && value.value.includes('Write') ? '📤' : '📥';
                return `${ioType} ${(value.type || 'io').toUpperCase()}\n${value.value || 'No content'}`;
            } else if (value.type === 'assign') {
                return `🔄 ${(value.type || 'assign').toUpperCase()}\n${value.value || 'No content'}`;
            }
            // Для condition/loop тел
            return `📋 ${key}\n${JSON.stringify(value).substring(0, 50)}...`;
        }

        return `📋 ${key}\n${typeof value === 'string' ? value : 'Object'}`;
    },

    // Определение стиля узла
    getNodeStyle(value, sectionType) {
        const baseStyle = 'stroke-width: 2px;';

        if (!value) return baseStyle + 'fill: #FFFFFF; stroke: #333;';

        switch (sectionType) {
            case 'functionBlock':
                return baseStyle + 'fill: #E3F2FD; stroke: #2196F3;';

            case 'constantBlock':
                return baseStyle + 'fill: #FFF3E0; stroke: #FF9800;';

            case 'variableBlock':
                return baseStyle + 'fill: #F3E5F5; stroke: #9C27B0;';

            case 'mainBlock':
                if (value.type === 'io') {
                    return baseStyle + 'fill: #E8F5E8; stroke: #4CAF50;';
                } else if (value.type === 'assign') {
                    return baseStyle + 'fill: #FFF8E1; stroke: #FFC107;';
                } else if (value.type === 'if' || value.type === 'while' || value.type === 'for') {
                    return baseStyle + 'fill: #E8EAF6; stroke: #3F51B5;';
                } else if (value.type === 'caseOf') {
                    return baseStyle + 'fill: #FCE4EC; stroke: #E91E63;';
                }
                return baseStyle + 'fill: #F5F5F5; stroke: #9E9E9E;';

            default:
                return baseStyle + 'fill: #FFFFFF; stroke: #333;';
        }
    },

    // Рендеринг блок-схемы
    renderFlowchart() {
        try {
            const svg = d3.select('#flowchart');

            // Очищаем предыдущий граф
            svg.selectAll('*').remove();

            // Проверяем, есть ли граф
            if (!this.state.graph) {
                throw new Error('Graph not created');
            }

            // Создаем внутреннюю группу для трансформаций
            const innerGroup = svg.append('g');

            // Создаем рендерер dagre-d3
            const render = new dagreD3.render();

            // Рендерим граф
            render(innerGroup, this.state.graph);

            // Настраиваем масштабирование
            const zoom = d3.zoom()
                .scaleExtent([0.1, 3])
                .on('zoom', (event) => {
                    innerGroup.attr('transform', event.transform);
                    this.state.zoomLevel = event.transform.k;
                });

            svg.call(zoom);
            //TODO
            
            // Добавляем интерактивность узлам
            innerGroup.selectAll('g.node')
                .on('click', (event, nodeId) => {
                    console.log('Click on node:', nodeId);
                    event.stopPropagation();
                    this.showNodeDetails(nodeId);
                })
                .on('mouseover', (event, nodeId) => {
                    d3.select(event.currentTarget)
                        .select('rect, polygon, ellipse, path')
                        .style('stroke-width', '3px')
                        .style('filter', 'drop-shadow(0 0 5px rgba(0,0,0,0.2))');
                })
                .on('mouseout', (event, nodeId) => {
                    if (nodeId !== this.state.selectedNode) {
                        d3.select(event.currentTarget)
                            .select('rect, polygon, ellipse, path')
                            .style('stroke-width', '2px')
                            .style('filter', 'none');
                    }
                });
            // Добавляем drag & drop
            // В renderFlowchart() ЗАМЕНИТЕ весь блок drag & drop на этот:

            // Разделяем клик и drag более явно
            let isDragging = false;
            let dragStartTime = 0;
            let dragStartX = 0;
            let dragStartY = 0;

            // ЗАМЕНИТЕ весь блок drag & drop (от ~строка 880 до ~строка 980) на:

            innerGroup.selectAll('g.node')
                .call(d3.drag()
                    .filter(event => {
                        // Разрешаем drag только для левой кнопки мыши
                        return event.button === 0;
                    })
                    .on('start', function (event, nodeId) {
                        // event.subject = {x: event.x, y: event.y, nodeId: nodeId};

                        // Визуальная обратная связь при начале drag
                        d3.select(this)
                            .classed('dragging', true)
                            .select('rect, polygon, ellipse, path')
                            .style('stroke', '#FF5722')
                            .style('stroke-width', '3px')
                            .style('opacity', '0.9');

                        console.log('Drag start for node:', nodeId);
                    })
                    .on('drag', function (event, nodeId) {
                        // this - это группа <g> узла
                        d3.select(this)
                            .attr('transform', `translate(${event.x}, ${event.y})`);
                    })
                    .on('end', function (event, nodeId) {
                        // Возвращаем нормальный стиль
                        d3.select(this)
                            .classed('dragging', false)
                            .select('rect, polygon, ellipse, path')
                            .style('stroke', () => {
                                // Получаем оригинальный цвет из данных узла
                                const node = FlowchartApp.state.graph.node(nodeId);
                                if (node && node.style) {
                                    const match = node.style.match(/stroke:\s*([^;]+)/);
                                    return match ? match[1] : '#333';
                                }
                                return '#333';
                            })
                            .style('stroke-width', '2px')
                            .style('opacity', '1');

                        console.log('Drag end for node:', nodeId, 'at', event.x, event.y);

                        // Сохраняем позицию
                        FlowchartApp.saveNodePosition(nodeId, event.x, event.y);
                    })
                );

            // Центрируем граф
            setTimeout(() => {
                this.centerGraph(svg);
            }, 100); // Даем время на рендеринг

            return true;

        } catch (error) {
            console.error('Render error:', error);

            // Показываем сообщение об ошибке
            const svg = d3.select('#flowchart');
            svg.selectAll('*').remove();

            svg.append('text')
                .attr('x', '50%')
                .attr('y', '50%')
                .attr('text-anchor', 'middle')
                .attr('font-size', '16px')
                .attr('fill', '#F44336')
                .text(`Render error: ${error.message}`);

            svg.append('text')
                .attr('x', '50%')
                .attr('y', '55%')
                .attr('text-anchor', 'middle')
                .attr('font-size', '14px')
                .attr('fill', '#666')
                .text('Check console for details');

            return false;
        }
    },
    setupNodeInteractions() {
        const svg = d3.select('#flowchart');
        const nodes = svg.selectAll('g.node');

        // Явное разделение клика и drag
        nodes.each(function (nodeId) {
            const node = d3.select(this);
            let isDrag = false;
            let startX, startY;

            // Mouse down
            node.on('mousedown', function (event) {
                if (event.button !== 0) return;

                startX = event.clientX;
                startY = event.clientY;
                isDrag = false;

                // Запускаем таймер для определения drag
                setTimeout(() => {
                    if (!isDrag && this.contains(event.target)) {
                        // Если прошло время и не начался drag - это клик
                        FlowchartApp.showNodeDetails(nodeId);
                    }
                }, 200);
            });

            // Drag поведение
            node.call(d3.drag()
                .on('start', function (event) {
                    event.sourceEvent.stopPropagation();
                    isDrag = true;

                    // Визуальная обратная связь
                    node.select('rect, polygon, ellipse, path')
                        .style('stroke', '#FF5722')
                        .style('stroke-width', '3px');
                })
                .on('drag', function (event) {
                    const transform = d3.zoomTransform(svg.node());
                    node.attr('transform',
                        `translate(${event.x / transform.k}, ${event.y / transform.k})`);
                })
                .on('end', function (event) {
                    // Возвращаем нормальный стиль
                    node.select('rect, polygon, ellipse, path')
                        .style('stroke', () => {
                            const nodeData = FlowchartApp.state.graph.node(nodeId);
                            const style = nodeData.style || '';
                            const match = style.match(/stroke:\s*([^;]+)/);
                            return match ? match[1] : '#333';
                        })
                        .style('stroke-width', '2px');

                    // Сохраняем позицию
                    const transform = d3.zoomTransform(svg.node());
                    FlowchartApp.saveNodePosition(nodeId,
                        event.x / transform.k,
                        event.y / transform.k);
                })
            );
        });
    },
    saveNodePosition(nodeId, x, y) {
        // Сохраняем позицию для истории
        //this.saveToHistory('before move node');

        // Обновляем позицию в состоянии
        if (!this.state.nodePositions) {
            this.state.nodePositions = new Map();
        }
        this.state.nodePositions.set(nodeId, { x, y });

        // Обновляем JSON если нужно
        //this.updateNodePositionInJson(nodeId, x, y);

        this.updateStatus(`✓ Node position saved`, 'success');
    },

    // Новый метод для обновления стрелок при перемещении узла

    /*updateEdgesForNode(nodeId, x, y) {
        if (!this.state.graph) return;

        const svg = d3.select('#flowchart');
        const edges = this.state.graph.edges();

        // Находим все связи, связанные с этим узлом
        edges.forEach(edge => {
            if (edge.v === nodeId || edge.w === nodeId) {
                // Находим путь стрелки
                const edgePath = svg.select(`.edgePath[data-edge="${edge.v}-${edge.w}"]`);

                if (!edgePath.empty()) {
                    // Получаем координаты начального и конечного узлов
                    const sourceNode = d3.select(`g.node [id="${edge.v}"]`).parent();
                    const targetNode = d3.select(`g.node [id="${edge.w}"]`).parent();

                    if (!sourceNode.empty() && !targetNode.empty()) {
                        // Получаем текущие трансформации узлов
                        const sourceTransform = sourceNode.attr('transform');
                        const targetTransform = targetNode.attr('transform');

                        // Извлекаем координаты из transform
                        const sourceX = this.extractTranslateX(sourceTransform);
                        const sourceY = this.extractTranslateY(sourceTransform);
                        const targetX = this.extractTranslateX(targetTransform);
                        const targetY = this.extractTranslateY(targetTransform);

                        // Получаем размеры узлов
                        const sourceRect = sourceNode.select('rect, polygon, ellipse, path').node();
                        const targetRect = targetNode.select('rect, polygon, ellipse, path').node();

                        if (sourceRect && targetRect) {
                            const sourceBounds = sourceRect.getBBox();
                            const targetBounds = targetRect.getBBox();

                            // Вычисляем точки соединения
                            const startPoint = this.calculateConnectionPoint(
                                sourceX, sourceY, sourceBounds,
                                targetX, targetY, targetBounds
                            );

                            const endPoint = this.calculateConnectionPoint(
                                targetX, targetY, targetBounds,
                                sourceX, sourceY, sourceBounds
                            );

                            // Обновляем путь
                            edgePath.select('path')
                                .attr('d', `M${startPoint.x},${startPoint.y} L${endPoint.x},${endPoint.y}`);
                        }
                    }
                }
            }
        });
    },*/

    // Дополнительные вспомогательные методы
    extractTranslateX(transform) {
        if (!transform || transform === 'none') return 0;
        const match = transform.match(/translate\(([^,]+)/);
        return match ? parseFloat(match[1]) : 0;
    },

    extractTranslateY(transform) {
        if (!transform || transform === 'none') return 0;
        const match = transform.match(/translate\([^,]+,([^)]+)/);
        return match ? parseFloat(match[1]) : 0;
    },

    calculateConnectionPoint(sourceX, sourceY, sourceBounds, targetX, targetY, targetBounds) {
        // Простой алгоритм для точек соединения
        const sourceCenterX = sourceX + sourceBounds.width / 2;
        const sourceCenterY = sourceY + sourceBounds.height / 2;
        const targetCenterX = targetX + targetBounds.width / 2;
        const targetCenterY = targetY + targetBounds.height / 2;

        // Вектор от источника к цели
        const dx = targetCenterX - sourceCenterX;
        const dy = targetCenterY - sourceCenterY;

        // Определяем сторону соединения
        if (Math.abs(dx) > Math.abs(dy)) {
            // Горизонтальное соединение
            return {
                x: sourceX + (dx > 0 ? sourceBounds.width : 0),
                y: sourceY + sourceBounds.height / 2
            };
        } else {
            // Вертикальное соединение
            return {
                x: sourceX + sourceBounds.width / 2,
                y: sourceY + (dy > 0 ? sourceBounds.height : 0)
            };
        }
    },

   /* onDragStart(event, nodeId) {
        console.log('Drag start:', nodeId);
        d3.select(event.sourceEvent.target).classed('dragging', true);
    },

    onDrag(event, nodeId) {
        // Перемещаем только группу узла, не перерисовывая весь граф
        const nodeGroup = d3.select(`#flowchart g.node [id="${nodeId}"]`).parent();

        // Временно меняем стиль перетаскиваемого узла
        nodeGroup.classed('dragging', true)
            .select('rect, polygon, ellipse, path')
            .style('stroke', '#FF5722')
            .style('stroke-width', '3px')
            .style('filter', 'drop-shadow(0 0 8px rgba(255,87,34,0.5))');

        // Применяем трансформацию
        nodeGroup.attr('transform', `translate(${event.x}, ${event.y})`);

        // Можно показать координаты при перетаскивании
        event.sourceEvent.preventDefault();
    },

    onDragEnd(event, nodeId) {
        const nodeGroup = d3.select(`#flowchart g.node [id="${nodeId}"]`).parent();
        const node = this.state.graph.node(nodeId);

        // Возвращаем нормальный стиль
        nodeGroup.classed('dragging', false)
            .select('rect, polygon, ellipse, path')
            .style('stroke', () => {
                // Возвращаем оригинальный цвет из стиля
                const style = node.style || '';
                const match = style.match(/stroke:\s*([^;]+)/);
                return match ? match[1] : '#333';
            })
            .style('stroke-width', '2px')
            .style('filter', 'none');

        console.log(`Node ${nodeId} moved to: (${event.x}, ${event.y})`);

        // Здесь можно:
        // 1. Сохранить новые координаты в JSON
        // 2. Пересчитать позиции других узлов
        // 3. Обновить связи

        // Временное решение - просто сбросить позицию
        // nodeGroup.attr('transform', null);

        // ИЛИ: пересчитать весь граф с новыми данными
        // this.updateNodePositionInJson(nodeId, event.x, event.y);
    },
    */

    // Опционально: метод для обновления позиции в JSON
    updateNodePositionInJson(nodeId, x, y) {
        if (!this.state.currentJson) return;

        const node = this.state.graph.node(nodeId);
        if (!node || !node.data) return;

        this.saveToHistory('before move node');

        // Добавляем координаты в данные узла
        const sections = this.state.currentJson.program.sections;
        const section = node.data.section;
        const nodeKey = node.data.id;

        if (sections[section] && sections[section][nodeKey]) {
            // Создаем или обновляем поле position
            if (!sections[section][nodeKey].position) {
                sections[section][nodeKey].position = {};
            }
            sections[section][nodeKey].position = { x, y };

            // Обновляем JSON
            document.getElementById('jsonInput').value = JSON.stringify(
                this.state.currentJson, null, 2
            );

            this.saveToHistory(`moved node ${nodeKey}`);
            this.updateStatus(`✓ Node position updated`, 'success');
        }
    },

    // Центрирование графа
    centerGraph(svg) {
        try {
            const svgWidth = svg.node().getBoundingClientRect().width;
            const svgHeight = svg.node().getBoundingClientRect().height;
            const graphWidth = this.state.graph.graph().width;
            const graphHeight = this.state.graph.graph().height;

            console.log(`SVG: ${svgWidth}x${svgHeight}, Graph: ${graphWidth}x${graphHeight}`);

            // Проверяем размеры графа
            if (graphWidth > 0 && graphHeight > 0 && svgWidth > 0 && svgHeight > 0) {
                const scale = Math.min(
                    (svgWidth) / graphWidth,
                    (svgHeight - 200) / graphHeight
                ) * 0.8;

                const translate = [
                    (svgWidth - graphWidth * scale) / 2,
                    30
                ];

                console.log(`Centering: translate(${translate[0]}, ${translate[1]}), scale(${scale})`);

                const transform = d3.zoomIdentity
                    .translate(translate[0], translate[1])
                    .scale(scale);

                const zoom = d3.zoom().scaleExtent([0.1, 3]);
                svg.call(d3.zoom().transform, transform);

                this.state.zoomLevel = scale;
            } else {
                console.warn('Invalid dimensions for centering');
            }
        } catch (error) {
            console.warn('Could not center graph:', error);
        }
    },

    // Показ сообщения при пустом графе
    showEmptyMessage(message) {
        const svg = d3.select('#flowchart');
        svg.selectAll('*').remove();

        svg.append('text')
            .attr('x', '50%')
            .attr('y', '50%')
            .attr('text-anchor', 'middle')
            .attr('font-size', '16px')
            .attr('fill', '#666')
            .text(message);
    },

    // Показ деталей узла
    showNodeDetails(nodeId) {
        try {
            const node = this.state.graph.node(nodeId);
            if (!node) return;

            // Если уже открыто это же меню - закрываем
            if (this.state.selectedNode === nodeId &&
                document.getElementById('nodeDetails').classList.contains('active')) {
                this.hideNodeDetails();
                return;
            }

            // ИСПРАВЛЕНО: Не присваивай node.data.section, а используй существующую переменную
            this.state.selectedNode = nodeId;

            const details = {
                'Node ID': nodeId,
                'Label': node.label || 'No label',
                'Type': node.class || 'Unknown',
                'Shape': node.shape || 'rect'
            };

            // Добавляем пользовательские данные если есть
            if (node.data) {
                details['Section'] = node.data.section || 'Unknown';
                details['Original ID'] = node.data.id || 'Unknown';

                if (node.data.value) {
                    if (typeof node.data.value === 'string') {
                        // Для простых строк (constantBlock, variableBlock)
                        details['Value'] = node.data.value;
                    } else if (typeof node.data.value === 'object') {
                        // Для объектов (mainBlock)
                        Object.entries(node.data.value).forEach(([key, value]) => {
                            if (key === 'body' && typeof value === 'object') {
                                details['Body'] = 'Complex structure (see JSON)';
                            } else if (typeof value === 'string') {
                                details[key] = value;
                            } else {
                                details[key] = JSON.stringify(value, null, 2);
                            }
                        });
                    }
                }
            }

            let detailsHtml = Object.entries(details)
                .map(([key, value]) => `<div class="detail-row"><strong>${key}:</strong> ${value}</div>`)
                .join('');

            // Добавляем крестик закрытия и кнопки действий
            detailsHtml += `
            <div class="details-footer">
                <button id="closeDetailsBtn" class="close-btn">✕ Close</button>
                <button id="editNodeBtn" class="edit-btn">✏ Edit</button>
                <button id="deleteNodeBtn" class="delete-btn">🗑 Delete</button>
            </div>
        `;

            document.getElementById('nodeInfo').innerHTML = detailsHtml;
            document.getElementById('nodeDetails').classList.add('active');

            // Обработчики для кнопок
            document.getElementById('closeDetailsBtn')?.addEventListener('click', () => {
                this.hideNodeDetails();
            });

            document.getElementById('editNodeBtn')?.addEventListener('click', () => {
                this.showEditNodeDialog(nodeId, node);
            });

            document.getElementById('deleteNodeBtn')?.addEventListener('click', () => {
                this.deleteNode(nodeId, node);
            });

            // Подсвечиваем выбранный узел
            d3.select('#flowchart')
                .selectAll('g.node')
                .select('rect, polygon, ellipse, path')
                .style('stroke-width', '2px');

            const selectedNode = d3.select(`#flowchart g.node [id="${nodeId}"]`);
            if (!selectedNode.empty()) {
                selectedNode
                    .parent()
                    .select('rect, polygon, ellipse, path')
                    .style('stroke-width', '4px')
                    .style('stroke', '#FF5722');
            }

            // Закрытие по клику вне меню
            setTimeout(() => {
                const clickHandler = (event) => {
                    const detailsPanel = document.getElementById('nodeDetails');
                    if (detailsPanel &&
                        !detailsPanel.contains(event.target) &&
                        !event.target.closest('.node-dialog')) {
                        this.hideNodeDetails();
                        document.removeEventListener('click', clickHandler);
                    }
                };
                document.addEventListener('click', clickHandler);
            }, 100);

        } catch (error) {
            console.error('Error showing node details:', error);
        }
    },

    // Новый метод для скрытия деталей
    hideNodeDetails() {
        this.state.selectedNode = null;
        document.getElementById('nodeDetails').classList.remove('active');

        // Снимаем подсветку
        const svg = d3.select('#flowchart');
        svg.selectAll('g.node')
            .select('rect, polygon, ellipse, path')
            .style('stroke-width', '2px')
            .style('stroke', function () {
                // Возвращаем оригинальный цвет
                const parent = d3.select(this).node().parentElement;
                const nodeId = parent.querySelector('[id]')?.id;
                if (nodeId) {
                    const node = FlowchartApp.state.graph?.node(nodeId);
                    if (node && node.style) {
                        const match = node.style.match(/stroke:\s*([^;]+)/);
                        return match ? match[1] : '#333';
                    }
                }
                return '#333';
            });

        // Убираем обработчик клика вне меню
        document.removeEventListener('click', this.handleOutsideClick);

        // ВАЖНО: Перезагружаем обработчики событий для узлов
        this.rebindNodeEvents();
    },

    // Новый метод для перезагрузки обработчиков
    rebindNodeEvents() {
        const svg = d3.select('#flowchart');
        const nodes = svg.selectAll('g.node');

        // Удаляем старые обработчики
        nodes.on('click', null);

        // Добавляем новые
        nodes.on('click', (event, nodeId) => {
            event.stopPropagation();
            this.showNodeDetails(nodeId);
        });
    },

    // Диалог редактирования узла
    showEditNodeDialog(nodeId, node) {
        const dialog = document.createElement('div');
        dialog.className = 'node-dialog edit-dialog';

        let content = '';
        if (node.data.section === 'mainBlock') {
            content = node.data.value.value || '';
        } else {
            content = node.data.value || '';
        }

        dialog.innerHTML = `
        <h3>✏ Edit Node</h3>
        <div class="dialog-content">
            <label>Node Type: <strong>${node.data.section} - ${node.data.value?.type || 'statement'}</strong></label>
            
            <label>Content:</label>
            <textarea id="editNodeContent" rows="4">${content}</textarea>
            
            <div class="dialog-buttons">
                <button id="cancelEditBtn">Cancel</button>
                <button id="saveEditBtn" class="primary">Save Changes</button>
            </div>
        </div>
    `;

        document.body.appendChild(dialog);

        document.getElementById('cancelEditBtn').addEventListener('click', () => {
            dialog.remove();
        });

        document.getElementById('saveEditBtn').addEventListener('click', () => {
            const newContent = document.getElementById('editNodeContent').value.trim();
            this.updateNodeContent(nodeId, node, newContent);
            dialog.remove();
            this.hideNodeDetails();
        });

        // Закрытие по клику вне диалога
        dialog.addEventListener('click', (e) => {
            if (e.target === dialog) {
                dialog.remove();
            }
        });
    },

    // Обновление содержимого узла
    updateNodeContent(nodeId, node, newContent) {
        if (!newContent || !this.state.currentJson) return;

        this.saveToHistory('before edit node');

        const sections = this.state.currentJson.program.sections;
        const section = node.data.section;
        const nodeKey = node.data.id;

        if (sections[section] && sections[section][nodeKey]) {
            if (section === 'mainBlock') {
                sections[section][nodeKey].value = newContent;
            } else {
                sections[section][nodeKey] = newContent +
                    (newContent.endsWith(';') ? '' : ';');
            }

            // Обновляем JSON в textarea
            document.getElementById('jsonInput').value = JSON.stringify(
                this.state.currentJson, null, 2
            );

            this.saveToHistory(`edited node ${nodeKey} in ${section}`);
            this.loadFlowchart();
            this.updateStatus(`✓ Node "${nodeKey}" updated`, 'success');
        }
    },

    handleOutsideClick(event) {
        const detailsPanel = document.getElementById('nodeDetails');
        if (detailsPanel &&
            !detailsPanel.contains(event.target) &&
            !event.target.closest('.node-dialog')) {
            this.hideNodeDetails();
        }
    },
    // Удаление узла
    deleteNode(nodeId, node) {
        if (!confirm('Delete this node?')) return;

        if (!this.state.currentJson) return;

        this.saveToHistory('before delete node');

        const sections = this.state.currentJson.program.sections;
        const section = node.data.section;
        const nodeKey = node.data.id;

        if (sections[section] && sections[section][nodeKey]) {
            delete sections[section][nodeKey];

            // Обновляем JSON в textarea
            document.getElementById('jsonInput').value = JSON.stringify(
                this.state.currentJson, null, 2
            );

            this.saveToHistory(`deleted node ${nodeKey} from ${section}`);
            this.loadFlowchart();
            this.hideNodeDetails();
            this.updateStatus(`✓ Node "${nodeKey}" deleted`, 'success');
        }
    },

    // Управление зумом
    zoomIn(factor = 1.2) {
        try {
            const svg = d3.select('#flowchart');
            if (!svg.node()) {
                console.error('SVG element not found');
                return;
            }

            // Получаем текущий zoom
            const zoom = d3.zoom().scaleExtent([0.1, 3]);
            const currentTransform = d3.zoomTransform(svg.node());

            // Вычисляем новую трансформацию
            const newScale = Math.min(3, currentTransform.k * factor);
            const newTransform = d3.zoomIdentity
                .translate(currentTransform.x, currentTransform.y)
                .scale(newScale);

            // Применяем трансформацию и обновляем zoom
            svg.transition()
                .duration(200)
                .call(zoom.transform, newTransform);

            this.state.zoomLevel = newScale;
            //console.log('Zoom in to:', newScale);
        } catch (error) {
            console.error('Zoom in error:', error);
        }
    },

    zoomOut(factor = 0.8) {
        try {
            const svg = d3.select('#flowchart');
            if (!svg.node()) {
                console.error('SVG element not found');
                return;
            }

            // Получаем текущий zoom
            const zoom = d3.zoom().scaleExtent([0.1, 3]);
            const currentTransform = d3.zoomTransform(svg.node());

            // Вычисляем новую трансформацию
            const newScale = Math.max(0.1, currentTransform.k * factor);
            const newTransform = d3.zoomIdentity
                .translate(currentTransform.x, currentTransform.y)
                .scale(newScale);

            // Применяем трансформацию и обновляем zoom
            svg.transition()
                .duration(200)
                .call(zoom.transform, newTransform);

            this.state.zoomLevel = newScale;
            //console.log('Zoom out to:', newScale);
        } catch (error) {
            console.error('Zoom out error:', error);
        }
    },

    resetZoom() {
        try {
            const svg = d3.select('#flowchart');
            if (!svg.node()) {
                console.error('SVG element not found');
                return;
            }

            // Получаем текущий zoom
            const zoom = d3.zoom().scaleExtent([0.1, 3]);

            // Создаем трансформацию для центрирования
            this.centerGraph(svg);

            this.state.zoomLevel = 1;
            console.log('Reset zoom to 1');
        } catch (error) {
            console.error('Reset zoom error:', error);
        }
    },

   
    //TODO


    // Экспорт в PNG
    exportAsPNG() {
        try {
            const svg = document.getElementById('flowchart');

            // Клонируем SVG чтобы не повлиять на оригинал
            const clonedSvg = svg.cloneNode(true);

            // Убираем интерактивные элементы
            clonedSvg.querySelectorAll('.node-details, .legend, .zoom-controls, .toolbar-toggle')
                .forEach(el => el.remove());

            // Конвертируем Unicode в ASCII для btoa
            const svgData = new XMLSerializer().serializeToString(clonedSvg);
            const cleanedSvgData = svgData
                .replace(/[\u007F-\uFFFF]/g, function (chr) {
                    return "&#" + chr.charCodeAt(0) + ";";
                });

            const canvas = document.createElement('canvas');
            const ctx = canvas.getContext('2d');

            const svgSize = svg.getBoundingClientRect();
            canvas.width = svgSize.width * 2; // Для лучшего качества
            canvas.height = svgSize.height * 2;

            const img = new Image();
            img.onload = function () {
                // Белый фон
                ctx.fillStyle = 'white';
                ctx.fillRect(0, 0, canvas.width, canvas.height);

                // Рисуем SVG
                ctx.drawImage(img, 0, 0, canvas.width, canvas.height);

                // Создаем ссылку для скачивания
                const link = document.createElement('a');
                link.download = 'pascal-flowchart.png';
                link.href = canvas.toDataURL('image/png');
                link.click();
            };

            img.src = 'data:image/svg+xml;base64,' + btoa(cleanedSvgData);

            this.updateStatus('✓ Flowchart exported as PNG', 'success');

        } catch (error) {
            this.updateStatus(`Export error: ${error.message}`, 'error');
            console.error('Export error:', error);
        }
    },

    // Инициализация пустой блок-схемы
    initEmptyFlowchart() {
        const svg = d3.select('#flowchart');
        svg.selectAll('*').remove();

        // Добавляем текст инструкции
        svg.append('text')
            .attr('x', '50%')
            .attr('y', '50%')
            .attr('text-anchor', 'middle')
            .attr('font-size', '18px')
            .attr('fill', '#666')
            .text('← Enter Pascal JSON and click "Load Flowchart"');
    },

    // Обновление статистики
    updateStats() {
        try {
            const nodeCount = this.state.graph ? this.state.graph.nodes().length : 0;
            const edgeCount = this.state.graph ? this.state.graph.edges().length : 0;

            document.getElementById('nodeCount').textContent = nodeCount;
            document.getElementById('edgeCount').textContent = edgeCount;
        } catch (error) {
            console.error('Error updating stats:', error);
        }
    },
    // Функция для дебага графа
    debugGraph() {
        if (!this.state.graph) {
            console.log('No graph to debug');
            return;
        }

        console.log('=== GRAPH DEBUG INFO ===');
        console.log('Nodes:', this.state.graph.nodes());
        console.log('Edges:', this.state.graph.edges());
        console.log('Graph config:', this.state.graph.graph());

        // Проверяем каждый узел
        this.state.graph.nodes().forEach(nodeId => {
            const node = this.state.graph.node(nodeId);
            console.log(`Node ${nodeId}:`, {
                label: node.label,
                shape: node.shape,
                style: node.style,
                class: node.class,
                hasLabel: !!node.label,
                hasShape: !!node.shape,
                shapeType: typeof node.shape
            });
        });
    },

    // Простой тестовый рендер
    testRender() {
        const svg = d3.select('#flowchart');
        svg.selectAll('*').remove();

        // Просто тестируем базовый SVG
        svg.append('circle')
            .attr('cx', 100)
            .attr('cy', 100)
            .attr('r', 50)
            .attr('fill', '#4CAF50')
            .attr('stroke', '#2E7D32')
            .attr('stroke-width', 3);

        svg.append('text')
            .attr('x', 100)
            .attr('y', 100)
            .attr('text-anchor', 'middle')
            .attr('dominant-baseline', 'middle')
            .attr('fill', 'white')
            .text('Test SVG');

        console.log('Test render completed');
    },

    // Обновление статус-бара
    updateStatus(message, type = 'info') {
        const statusEl = document.getElementById('status');
        statusEl.textContent = message;

        // Очищаем предыдущие классы
        statusEl.className = '';

        // Добавляем класс в зависимости от типа
        if (type === 'success') {
            statusEl.classList.add('success');
            statusEl.style.color = '#4CAF50';
        } else if (type === 'error') {
            statusEl.classList.add('error');
            statusEl.style.color = '#F44336';
        } else if (type === 'warning') {
            statusEl.classList.add('warning');
            statusEl.style.color = '#FF9800';
        } else {
            statusEl.style.color = '#666';
        }

        // Автоматическое скрытие успешных сообщений
        if (type === 'success') {
            setTimeout(() => {
                if (statusEl.textContent === message) {
                    statusEl.textContent = 'Ready';
                    statusEl.style.color = '#666';
                }
            }, 3000);
        }
    }
};

// Инициализация приложения при загрузке страницы
document.addEventListener('DOMContentLoaded', () => {
    FlowchartApp.init();
});

// Глобальный объект для отладки
window.FlowchartApp = FlowchartApp;
