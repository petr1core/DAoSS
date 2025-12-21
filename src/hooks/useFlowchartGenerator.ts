// React hook для генерации блок-схемы с поддержкой fallback
import { useState, useCallback } from 'react';
import { useFlowchartParser } from './useFlowchartParser';
import { parseCodeToFlowchart } from '../parsers';
import { showToast } from '../utils/toast';
import type { FlowchartNode, Connection } from '../types/flowchart';
import type { ParserLanguage } from '../types/parser';

interface UseFlowchartGeneratorResult {
    isGenerating: boolean;
    error: string | null;
    generate: (code: string, language: ParserLanguage) => Promise<{ nodes: FlowchartNode[]; connections: Connection[] } | null>;
    generateWithFallback: (code: string) => FlowchartNode[];
}

/**
 * Hook для генерации блок-схемы с поддержкой fallback парсера
 */
export function useFlowchartGenerator(): UseFlowchartGeneratorResult {
    const parser = useFlowchartParser();
    const [isGenerating, setIsGenerating] = useState(false);
    const [error, setError] = useState<string | null>(null);

    const generate = useCallback(async (
        code: string,
        language: ParserLanguage
    ): Promise<{ nodes: FlowchartNode[]; connections: Connection[] } | null> => {
        if (!code.trim()) {
            showToast('Введите код для генерации', 'error');
            return null;
        }

        setIsGenerating(true);
        setError(null);

        try {
            const result = await parser.parseCode(code, language);
            
            if (result) {
                setIsGenerating(false);
                return result;
            }
            
            // Если парсер вернул null, но ошибка не критическая, пробуем fallback
            if (parser.error && !parser.error.includes('недоступен') && !parser.error.includes('память')) {
                // Пробуем fallback только для некритических ошибок
                const fallbackNodes = parseCodeToFlowchart(code);
                setIsGenerating(false);
                showToast('Использован упрощенный метод парсинга', 'error');
                return {
                    nodes: fallbackNodes,
                    connections: []
                };
            }
            
            setIsGenerating(false);
            return null;
        } catch (err) {
            const errorMessage = err instanceof Error ? err.message : 'Ошибка генерации блок-схемы';
            setError(errorMessage);
            setIsGenerating(false);
            
            // Автоматический fallback для критических ошибок
            if (
                errorMessage.includes('503') ||
                errorMessage.includes('недоступен') ||
                errorMessage.includes('упал') ||
                errorMessage.includes('bad allocation') ||
                errorMessage.includes('память')
            ) {
                const fallbackNodes = parseCodeToFlowchart(code);
                showToast('Использован упрощенный метод парсинга (fallback)', 'error');
                return {
                    nodes: fallbackNodes,
                    connections: []
                };
            }
            
            return null;
        }
    }, [parser]);

    const generateWithFallback = useCallback((code: string): FlowchartNode[] => {
        return parseCodeToFlowchart(code);
    }, []);

    return {
        isGenerating: isGenerating || parser.isParsing,
        error: error || parser.error,
        generate,
        generateWithFallback
    };
}

