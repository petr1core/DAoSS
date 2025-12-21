// Обработчики для canvas (panning, zoom)

interface CanvasPanState {
    isPanning: boolean;
    panStart: { x: number; y: number };
    panScroll: { x: number; y: number };
}

/**
 * Начинает перетаскивание canvas
 */
export function startCanvasPan(
    event: MouseEvent,
    wrapper: HTMLElement
): CanvasPanState | null {
    // Проверяем, что клик был не по узлу
    const target = event.target as HTMLElement;
    if (target.closest('.flowchart-node') || target.closest('.node-controls')) {
        return null;
    }
    
    return {
        isPanning: true,
        panStart: { x: event.clientX, y: event.clientY },
        panScroll: { x: wrapper.scrollLeft, y: wrapper.scrollTop }
    };
}

/**
 * Обновляет позицию canvas при перетаскивании
 */
export function updateCanvasPan(
    event: MouseEvent,
    panState: CanvasPanState,
    wrapper: HTMLElement
): void {
    if (!panState.isPanning) return;
    
    const deltaX = event.clientX - panState.panStart.x;
    const deltaY = event.clientY - panState.panStart.y;
    
    wrapper.scrollLeft = panState.panScroll.x - deltaX;
    wrapper.scrollTop = panState.panScroll.y - deltaY;
}

/**
 * Завершает перетаскивание canvas
 */
export function stopCanvasPan(
    panState: CanvasPanState | null,
    wrapper: HTMLElement | null
): CanvasPanState | null {
    if (panState) {
        panState.isPanning = false;
    }
    
    if (wrapper) {
        wrapper.style.cursor = '';
    }
    
    return null;
}

/**
 * Увеличивает масштаб
 */
export function zoomIn(currentZoom: number, maxZoom: number = 2): number {
    return Math.min(currentZoom + 0.1, maxZoom);
}

/**
 * Уменьшает масштаб
 */
export function zoomOut(currentZoom: number, minZoom: number = 0.1): number {
    return Math.max(currentZoom - 0.1, minZoom);
}

/**
 * Устанавливает масштаб
 */
export function setZoom(zoom: number, minZoom: number = 0.1, maxZoom: number = 2): number {
    return Math.max(minZoom, Math.min(maxZoom, zoom));
}


