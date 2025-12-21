// Утилиты для работы с комментариями
import type { Comment } from '../types/flowchart';

/**
 * Создает новый комментарий
 */
export function createComment(text: string, author: string = 'Пользователь'): Comment {
    return {
        id: `comment-${Date.now()}`,
        text: text.trim(),
        timestamp: new Date(),
        nodeId: '' // Будет установлено при добавлении к узлу
    };
}

/**
 * Добавляет комментарий к узлу
 */
export function addCommentToNode(
    nodeId: string,
    commentText: string,
    nodes: Array<{ id: string; comments: Comment[] }>
): boolean {
    const node = nodes.find(n => n.id === nodeId);
    if (!node) return false;
    
    const comment = createComment(commentText);
    comment.nodeId = nodeId;
    node.comments.push(comment);
    
    return true;
}

