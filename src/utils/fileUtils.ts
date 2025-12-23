// Утилиты для работы с файлами

/**
 * Читает содержимое файла как текст
 */
export function readFileAsText(file: File): Promise<string> {
    return new Promise((resolve, reject) => {
        const reader = new FileReader();
        reader.onload = (e) => {
            if (e.target?.result && typeof e.target.result === 'string') {
                resolve(e.target.result);
            } else {
                reject(new Error('Не удалось прочитать файл'));
            }
        };
        reader.onerror = () => reject(new Error('Ошибка чтения файла'));
        reader.readAsText(file);
    });
}

/**
 * Определяет язык программирования по расширению файла
 */
export function getLanguageFromFilename(filename: string): 'pascal' | 'c' | 'cpp' | null {
    const ext = filename.split('.').pop()?.toLowerCase();
    
    switch (ext) {
        case 'pas':
        case 'p':
        case 'pp':
            return 'pascal';
        case 'c':
            return 'c';
        case 'cpp':
        case 'cc':
        case 'cxx':
        case 'hpp':
        case 'h':
            return 'cpp';
        default:
            return null;
    }
}

/**
 * Загружает пример кода из файла
 */
export async function loadExampleCode(filename: string): Promise<{ code: string; language: 'pascal' | 'c' | 'cpp' } | null> {
    try {
        // Пытаемся загрузить из папки examples
        const response = await fetch(`/examples/${filename}`);
        if (!response.ok) {
            console.warn(`Не удалось загрузить пример ${filename}`);
            return null;
        }
        
        const code = await response.text();
        const language = getLanguageFromFilename(filename) || 'pascal';
        
        return { code, language };
    } catch (error) {
        console.error('Ошибка загрузки примера:', error);
        return null;
    }
}


