// Утилиты для работы с узлами
import type { NodeType } from '../types/flowchart';

/**
 * Возвращает текст по умолчанию для типа узла
 */
export function getDefaultText(type: NodeType): string {
    const texts: Record<NodeType, string> = {
        start: 'Начало',
        end: 'Конец',
        process: 'Процесс',
        decision: 'Условие',
        input: 'Ввод',
        output: 'Вывод'
    };
    return texts[type] || 'Блок';
}

/**
 * Возвращает метку для типа узла
 */
export function getTypeLabel(type: NodeType): string {
    const labels: Record<NodeType, string> = {
        start: 'Начало',
        end: 'Конец',
        process: 'Процесс',
        decision: 'Условие',
        input: 'Ввод',
        output: 'Вывод'
    };
    return labels[type] || 'Неизвестно';
}

