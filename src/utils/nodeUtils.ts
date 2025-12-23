// Утилиты для работы с узлами
import type { NodeType } from '../types/flowchart';

/**
 * Возвращает текст по умолчанию для типа узла
 */
export function getDefaultText(type: NodeType | 'function' | 'main'): string {
    const texts: Record<string, string> = {
        start: 'Начало',
        end: 'Конец',
        process: 'Процесс',
        decision: 'Условие',
        input: 'Ввод/Вывод',
        output: 'Ввод/Вывод',
        function: 'function name()',
        main: 'main()'
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


