// Сервис для работы с парсером API
import { api } from './api';
import type { ParserResponse } from '../types/parser';

const PARSER_TIMEOUT = 60000; // 60 секунд

/**
 * Вызывает API парсера для обработки кода
 */
export async function callParserAPI(
    code: string, 
    language: string
): Promise<ParserResponse> {
    // Проверка размера кода перед отправкой
    const codeSizeKB = (new Blob([code]).size / 1024).toFixed(2);
    if (code.length > 50000) {
        console.warn(`Большой файл: ${codeSizeKB} KB. Парсер может не справиться.`);
    }

    try {
        const response = await api.parseCode(code, language);
        
        // Логируем результат для отладки
        console.log('[DEBUG] Parser response:', {
            success: response.success,
            hasParserErrors: response.parserErrors && response.parserErrors.length > 0,
            parserErrorsCount: response.parserErrors ? response.parserErrors.length : 0,
            firstError: response.parserErrors && response.parserErrors.length > 0 ? response.parserErrors[0] : null
        });
        
        // Проверяем, есть ли ошибки парсера (даже если success = true)
        if (response.parserErrors && response.parserErrors.length > 0) {
            const parserError = response.parserErrors[0];
            const errorMessage = parserError.message || '';
            console.log('[DEBUG] Parser error message:', errorMessage);
            
            // Проверяем различные варианты сообщений об ошибке памяти
            if (errorMessage.toLowerCase().includes('bad allocation') || 
                errorMessage.toLowerCase().includes('bad_alloc') ||
                errorMessage.toLowerCase().includes('memory') ||
                errorMessage.toLowerCase().includes('allocation')) {
                throw new Error('Парсер не смог обработать код из-за нехватки памяти. ' +
                    'Код слишком сложный или содержит слишком много вложенных конструкций.');
            }
        }
        
        return response;
    } catch (error: unknown) {
        // Обработка различных типов ошибок
        if (error instanceof Error) {
            // Проверяем, не связана ли ошибка с падением парсера
            if (error.message.includes('bad allocation') || 
                error.message.includes('недоступен')) {
                throw new Error('Парсер упал при обработке кода. ' +
                    'Возможно, код слишком сложный. Попробуйте упростить код или разбить его на части.');
            }
            
            if (error.message.includes('Таймаут')) {
                throw new Error('Таймаут запроса. Файл слишком большой или парсер перегружен');
            }
        }
        throw error;
    }
}

