// Утилиты для работы с темой

// Время последнего переключения темы
let lastThemeToggle = 0;
const THEME_TOGGLE_DELAY = 3000; // 3 секунды

/**
 * Инициализирует тему из localStorage или системных настроек
 */
export function initTheme(): boolean {
    const savedTheme = localStorage.getItem('theme');
    const prefersDark = window.matchMedia('(prefers-color-scheme: dark)').matches;
    const html = document.documentElement;
    const body = document.body;
    
    const isDark = savedTheme === 'dark' || (!savedTheme && prefersDark);
    
    if (isDark) {
        html.classList.add('dark');
        body.classList.add('dark');
    } else {
        html.classList.remove('dark');
        body.classList.remove('dark');
    }
    
    return isDark;
}

/**
 * Проверяет, можно ли переключить тему (прошло ли 3 секунды)
 */
export function canToggleTheme(): boolean {
    return Date.now() - lastThemeToggle >= THEME_TOGGLE_DELAY;
}

/**
 * Возвращает оставшееся время до возможности переключения (в секундах)
 */
export function getThemeToggleCooldown(): number {
    const remaining = THEME_TOGGLE_DELAY - (Date.now() - lastThemeToggle);
    return Math.max(0, Math.ceil(remaining / 1000));
}

/**
 * Переключает тему с задержкой 3 секунды между переключениями
 * @returns объект с результатом: { success: boolean, isDark: boolean, cooldownRemaining?: number }
 */
export function toggleTheme(): { success: boolean; isDark: boolean; cooldownRemaining?: number } {
    const html = document.documentElement;
    const body = document.body;
    const isDark = html.classList.contains('dark');
    
    // Проверяем задержку
    if (!canToggleTheme()) {
        const cooldownRemaining = getThemeToggleCooldown();
        return { success: false, isDark, cooldownRemaining };
    }
    
    // Обновляем время последнего переключения
    lastThemeToggle = Date.now();
    
    if (isDark) {
        html.classList.remove('dark');
        body.classList.remove('dark');
        localStorage.setItem('theme', 'light');
        return { success: true, isDark: false };
    } else {
        html.classList.add('dark');
        body.classList.add('dark');
        localStorage.setItem('theme', 'dark');
        return { success: true, isDark: true };
    }
}

/**
 * Устанавливает тему (без задержки, для программного использования)
 */
export function setTheme(isDark: boolean): void {
    const html = document.documentElement;
    const body = document.body;
    if (isDark) {
        html.classList.add('dark');
        body.classList.add('dark');
        localStorage.setItem('theme', 'dark');
    } else {
        html.classList.remove('dark');
        body.classList.remove('dark');
        localStorage.setItem('theme', 'light');
    }
}


