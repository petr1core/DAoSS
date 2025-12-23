// React hook для парсинга кода в блок-схему
import { useState, useCallback } from 'react';
import { parseJsonToFlowchart, parseCodeToFlowchart } from '../parsers';
import { callParserAPI } from '../services/parserService';
import { showToast } from '../utils/toast';
import type { ParserLanguage, ParserResponse } from '../types/parser';
import type { FlowchartNode, Connection } from '../types/flowchart';

interface UseFlowchartParserResult {
    isParsing: boolean;
    error: string | null;
    parseCode: (code: string, language: ParserLanguage) => Promise<{ nodes: FlowchartNode[]; connections: Connection[] } | null>;
}

/**
 * Hook для парсинга кода в блок-схему
 */
export function useFlowchartParser(): UseFlowchartParserResult {
    const [isParsing, setIsParsing] = useState(false);
    const [error, setError] = useState<string | null>(null);

    const parseCode = useCallback(async (
        code: string,
        language: ParserLanguage
    ): Promise<{ nodes: FlowchartNode[]; connections: Connection[] } | null> => {
        if (!code.trim()) {
            showToast('Введите код для генерации', 'error');
            return null;
        }

        setIsParsing(true);
        setError(null);

        try {
            // Вызываем API парсера
            const response: ParserResponse = await callParserAPI(code, language);

            // Проверяем наличие ошибок парсера
            if (response.lexerErrors && response.lexerErrors.length > 0) {
                const errorMsg = `Ошибки лексера: ${response.lexerErrors.map(e => e.message || e.value).join(', ')}`;
                console.warn('Ошибки лексера:', response.lexerErrors);
                setError(errorMsg);
                showToast(errorMsg, 'error');
                return null;
            }

            if (response.parserErrors && response.parserErrors.length > 0) {
                const errorMsg = `Ошибки парсера: ${response.parserErrors.map(e => e.message || '').join(', ')}`;
                console.warn('Ошибки парсера:', response.parserErrors);
                setError(errorMsg);
                showToast(errorMsg, 'error');
                return null;
            }

            if (!response.success || !response.representation) {
                const errorMsg = response.error || 'Не удалось распарсить код';
                setError(errorMsg);
                showToast(errorMsg, 'error');
                return null;
            }

            // Парсим JSON в блок-схему
            const result = parseJsonToFlowchart(response.representation, language);

            showToast('Блок-схема успешно сгенерирована', 'success');
            return result;
        } catch (err) {
            const errorMessage = err instanceof Error ? err.message : 'Произошла ошибка при парсинге';
            console.error('Parse error:', err);
            setError(errorMessage);
            
            // Определяем, нужен ли fallback
            const shouldUseFallback = 
                errorMessage.includes('503') ||
                errorMessage.includes('недоступен') ||
                errorMessage.includes('упал') ||
                errorMessage.includes('bad allocation') ||
                errorMessage.includes('память') ||
                errorMessage.includes('allocation');
            
            if (shouldUseFallback) {
                const detailedError = 'Парсер не смог обработать код. Возможные причины:\n' +
                    '• Код слишком сложный (много вложенных конструкций)\n' +
                    '• Парсер упал из-за нехватки памяти\n' +
                    '• Парсер не запущен на порту 8080\n\n' +
                    'Попробуйте:\n' +
                    '• Упростить код\n' +
                    '• Разбить код на части\n' +
                    '• Использовать упрощенный метод парсинга';
                
                showToast(detailedError, 'error');
                
                // Используем fallback парсер после небольшой задержки
                setTimeout(() => {
                    try {
                        console.log('Используется упрощенный метод парсинга (fallback)');
                        const fallbackNodes = parseCodeToFlowchart(code);
                        showToast('Использован упрощенный метод парсинга (без парсера)', 'error');
                        // Возвращаем результат fallback, но не через обычный return
                        // Пользователь должен вызвать parseCode снова или использовать fallback отдельно
                    } catch (fallbackError) {
                        console.error('Ошибка при fallback парсинге:', fallbackError);
                        showToast('Не удалось сгенерировать блок-схему', 'error');
                    }
                }, 2000);
                
                return null;
            }
            
            showToast(errorMessage, 'error');
            return null;
        } finally {
            setIsParsing(false);
        }
    }, []);

    return {
        isParsing,
        error,
        parseCode
    };
}

