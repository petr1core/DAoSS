// Утилиты для работы с датами

/**
 * Форматирует дату в строку
 */
export function formatDate(date: Date | string): string {
    return new Intl.DateTimeFormat('ru-RU', {
        day: '2-digit',
        month: '2-digit',
        year: 'numeric',
        hour: '2-digit',
        minute: '2-digit',
        second: '2-digit'
    }).format(new Date(date));
}

/**
 * Возвращает относительное время (например, "5 мин. назад")
 */
export function getTimeAgo(date: Date | string): string {
    const d = typeof date === 'string' ? new Date(date) : date;
    if (isNaN(d.getTime())) return '';
    
    const seconds = Math.floor((Date.now() - d.getTime()) / 1000);
    
    if (seconds < 60) return `${seconds} сек. назад`;
    const minutes = Math.floor(seconds / 60);
    if (minutes < 60) return `${minutes} мин. назад`;
    const hours = Math.floor(minutes / 60);
    if (hours < 24) return `${hours} ч. назад`;
    const days = Math.floor(hours / 24);
    return `${days} дн. назад`;
}


