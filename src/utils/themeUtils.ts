// Утилиты для работы с темой

/**
 * Инициализирует тему из localStorage или системных настроек
 */
export function initTheme(): boolean {
    const savedTheme = localStorage.getItem('theme');
    const prefersDark = window.matchMedia('(prefers-color-scheme: dark)').matches;
    const html = document.documentElement;
    
    const isDark = savedTheme === 'dark' || (!savedTheme && prefersDark);
    
    if (isDark) {
        html.classList.add('dark');
    } else {
        html.classList.remove('dark');
    }
    
    return isDark;
}

/**
 * Переключает тему
 */
export function toggleTheme(): boolean {
    const html = document.documentElement;
    const isDark = html.classList.contains('dark');
    
    if (isDark) {
        html.classList.remove('dark');
        localStorage.setItem('theme', 'light');
        return false;
    } else {
        html.classList.add('dark');
        localStorage.setItem('theme', 'dark');
        return true;
    }
}

/**
 * Устанавливает тему
 */
export function setTheme(isDark: boolean): void {
    const html = document.documentElement;
    if (isDark) {
        html.classList.add('dark');
        localStorage.setItem('theme', 'dark');
    } else {
        html.classList.remove('dark');
        localStorage.setItem('theme', 'light');
    }
}


