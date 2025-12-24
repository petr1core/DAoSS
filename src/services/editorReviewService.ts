// Сервис для работы с ревью и комментариями в редакторе блок-схем
import { api, type Review, type CreateReviewItemDto } from './api';

export interface EditorComment {
    id: string;
    nodeId: string;
    body: string;
    status: 'open' | 'resolved';
    authorId: string;
    authorName?: string;
    createdAt: Date;
    updatedAt?: Date;
}

interface ReviewContext {
    projectId: string;
    fileId: string;
    fileVersionId: string;
    review: Review | null;
}

let currentContext: ReviewContext | null = null;
let commentsCache: Map<string, EditorComment[]> = new Map();

/**
 * Инициализирует контекст ревью для файла (без создания нового ревью)
 * Ревью создаётся только при сохранении новой версии файла
 */
export async function initReviewContext(
    projectId: string,
    fileId: string
): Promise<Review | null> {
    try {
        // Получаем версии файла
        const versions = await api.getSourceFileVersions(projectId, fileId);
        if (versions.length === 0) {
            console.warn('[EditorReview] Файл не имеет версий');
            return null;
        }

        // Находим последнюю версию
        const latestVersion = versions.reduce((latest, current) =>
            current.versionIndex > latest.versionIndex ? current : latest
        );

        // Ищем существующее ревью для этой версии файла
        // НЕ создаём новое ревью автоматически - оно создаётся только при сохранении новой версии
        const reviews = await api.getReviews(projectId);
        const review = reviews.find(
            r => r.targetType === 'source_file_version' && r.targetId === latestVersion.id
        );

        if (!review) {
            // Ревью нет - это нормально, оно будет создано при сохранении новой версии
            console.log('[EditorReview] Ревью для версии не найдено (будет создано при сохранении новой версии)');
            currentContext = {
                projectId,
                fileId,
                fileVersionId: latestVersion.id,
                review: null
            };
            return null;
        }

        currentContext = {
            projectId,
            fileId,
            fileVersionId: latestVersion.id,
            review
        };

        // Загружаем все комментарии для этого ревью
        await loadAllComments();

        return review;
    } catch (err) {
        console.error('[EditorReview] Ошибка инициализации контекста:', err);
        return null;
    }
}

/**
 * Создаёт ревью для новой версии файла или диаграммы
 * Вызывается после успешного сохранения новой версии
 */
export async function createReviewForVersion(
    projectId: string,
    targetType: 'source_file_version' | 'diagram_version',
    targetId: string
): Promise<Review | null> {
    try {
        // Проверяем, не существует ли уже ревью для этой версии
        const reviews = await api.getReviews(projectId);
        const existingReview = reviews.find(
            r => r.targetType === targetType && r.targetId === targetId
        );

        if (existingReview) {
            console.log('[EditorReview] Ревью для версии уже существует:', existingReview.id);
            return existingReview;
        }

        // Создаём новое ревью для версии
        const review = await api.createReview(projectId, {
            targetType,
            targetId
        });

        console.log('[EditorReview] Создано ревью для новой версии:', review.id);

        // Если это текущий файл, обновляем контекст
        if (currentContext && currentContext.projectId === projectId) {
            if (targetType === 'source_file_version' && targetId === currentContext.fileVersionId) {
                currentContext.review = review;
                currentContext.fileVersionId = targetId;
            }
        }

        return review;
    } catch (err) {
        console.error('[EditorReview] Ошибка создания ревью для версии:', err);
        return null;
    }
}

/**
 * Загружает все комментарии (ReviewItems) для текущего ревью
 */
async function loadAllComments(): Promise<void> {
    if (!currentContext?.review) return;

    try {
        const items = await api.getReviewItems(
            currentContext.projectId,
            currentContext.review.id
        );

        commentsCache.clear();

        for (const item of items) {
            if (item.anchorType === 'diagram') {
                // Парсим anchorRef для получения nodeId
                const anchorData = parseAnchorRef(item.anchorRef);
                if (!anchorData) continue;

                const nodeId = anchorData.nodeId;
                const comments = commentsCache.get(nodeId) || [];
                comments.push({
                    id: item.id,
                    nodeId: nodeId,
                    body: item.body,
                    status: item.status as 'open' | 'resolved',
                    authorId: item.createdBy,
                    createdAt: new Date(item.createdAt),
                    updatedAt: item.updatedAt ? new Date(item.updatedAt) : undefined
                });
                commentsCache.set(nodeId, comments);
            }
        }

        console.log('[EditorReview] Загружено комментариев:', commentsCache.size, 'блоков');
    } catch (err) {
        console.error('[EditorReview] Ошибка загрузки комментариев:', err);
    }
}

/**
 * Получает комментарии для блока
 */
export function getCommentsForNode(nodeId: string): EditorComment[] {
    return commentsCache.get(nodeId) || [];
}

// Интерфейс для структурированного anchorRef
export interface AnchorRefData {
    nodeId: string;
    nodeText: string;
}

/**
 * Парсит anchorRef в структуру
 */
export function parseAnchorRef(anchorRef: string): AnchorRefData | null {
    try {
        // Пробуем распарсить как JSON (новый формат)
        const parsed = JSON.parse(anchorRef);
        if (parsed.nodeId && parsed.nodeText !== undefined) {
            return parsed;
        }
    } catch {
        // Если не JSON, это старый формат (просто nodeId)
    }
    // Возвращаем anchorRef как nodeId для обратной совместимости
    return { nodeId: anchorRef, nodeText: anchorRef };
}

/**
 * Добавляет комментарий к блоку
 * @param nodeId - ID ноды для локального кеша
 * @param body - текст комментария
 * @param nodeText - текст ноды для отображения в списке ревью
 */
export async function addCommentToNode(
    nodeId: string,
    body: string,
    nodeText?: string
): Promise<EditorComment | null> {
    if (!currentContext) {
        console.error('[EditorReview] Контекст не инициализирован');
        return null;
    }

    // Если ревью нет, создаём его для текущей версии файла
    if (!currentContext.review && currentContext.fileVersionId) {
        try {
            const review = await createReviewForVersion(
                currentContext.projectId,
                'source_file_version',
                currentContext.fileVersionId
            );
            if (review) {
                currentContext.review = review;
            } else {
                console.error('[EditorReview] Не удалось создать ревью для версии');
                return null;
            }
        } catch (err) {
            console.error('[EditorReview] Ошибка создания ревью:', err);
            return null;
        }
    }

    if (!currentContext.review) {
        console.error('[EditorReview] Ревью не найдено и не может быть создано');
        return null;
    }

    try {
        // anchorRef содержит JSON с nodeId и текстом ноды
        const anchorRefData: AnchorRefData = {
            nodeId: nodeId,
            nodeText: nodeText || nodeId
        };

        const dto: CreateReviewItemDto = {
            kind: 'comment',
            anchorType: 'diagram',
            anchorRef: JSON.stringify(anchorRefData),
            body: body
        };

        const item = await api.createReviewItem(
            currentContext.projectId,
            currentContext.review.id,
            dto
        );

        const comment: EditorComment = {
            id: item.id,
            nodeId: nodeId,
            body: item.body,
            status: item.status as 'open' | 'resolved',
            authorId: item.createdBy,
            createdAt: new Date(item.createdAt)
        };

        // Обновляем кеш
        const comments = commentsCache.get(nodeId) || [];
        comments.push(comment);
        commentsCache.set(nodeId, comments);

        return comment;
    } catch (err) {
        console.error('[EditorReview] Ошибка добавления комментария:', err);
        return null;
    }
}

/**
 * Обновляет статус комментария (resolved/open)
 */
export async function updateCommentStatus(
    commentId: string,
    status: 'open' | 'resolved'
): Promise<boolean> {
    if (!currentContext?.review) return false;

    try {
        await api.updateReviewItem(
            currentContext.projectId,
            currentContext.review.id,
            commentId,
            { status }
        );

        // Обновляем кеш
        for (const comments of commentsCache.values()) {
            const comment = comments.find(c => c.id === commentId);
            if (comment) {
                comment.status = status;
                comment.updatedAt = new Date();
                break;
            }
        }

        return true;
    } catch (err) {
        console.error('[EditorReview] Ошибка обновления статуса:', err);
        return false;
    }
}

/**
 * Удаляет комментарий
 */
export async function deleteComment(commentId: string): Promise<boolean> {
    if (!currentContext?.review) return false;

    try {
        await api.deleteReviewItem(
            currentContext.projectId,
            currentContext.review.id,
            commentId
        );

        // Обновляем кеш
        for (const [nodeId, comments] of commentsCache) {
            const index = comments.findIndex(c => c.id === commentId);
            if (index !== -1) {
                comments.splice(index, 1);
                if (comments.length === 0) {
                    commentsCache.delete(nodeId);
                }
                break;
            }
        }

        return true;
    } catch (err) {
        console.error('[EditorReview] Ошибка удаления комментария:', err);
        return false;
    }
}

/**
 * Получает текущий контекст ревью
 */
export function getCurrentReviewContext(): ReviewContext | null {
    return currentContext;
}

/**
 * Проверяет, инициализирован ли контекст ревью
 */
export function isReviewContextReady(): boolean {
    return currentContext !== null && currentContext.review !== null;
}

/**
 * Очищает контекст ревью
 */
export function clearReviewContext(): void {
    currentContext = null;
    commentsCache.clear();
}

/**
 * Получает количество комментариев для блока
 */
export function getCommentCount(nodeId: string): number {
    return commentsCache.get(nodeId)?.length || 0;
}

/**
 * Получает общее количество комментариев
 */
export function getTotalCommentCount(): number {
    let total = 0;
    for (const comments of commentsCache.values()) {
        total += comments.length;
    }
    return total;
}

