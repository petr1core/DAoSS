namespace DAOSS.Application.Interfaces;

using DAOSS.Domain.Entities;

public interface IReviewService
{
	System.Threading.Tasks.Task<Review> CreateReviewAsync(Guid projectId, string targetType, Guid targetId, Guid createdBy, System.Threading.CancellationToken cancellationToken = default);
	System.Threading.Tasks.Task<Review?> GetReviewAsync(Guid reviewId, System.Threading.CancellationToken cancellationToken = default);
	System.Threading.Tasks.Task<IReadOnlyList<Review>> GetReviewsByProjectAsync(Guid projectId, System.Threading.CancellationToken cancellationToken = default);
	System.Threading.Tasks.Task<Review> UpdateReviewStatusAsync(Guid reviewId, string status, System.Threading.CancellationToken cancellationToken = default);
	System.Threading.Tasks.Task DeleteReviewAsync(Guid reviewId, System.Threading.CancellationToken cancellationToken = default);
	
	System.Threading.Tasks.Task<ReviewItem> CreateReviewItemAsync(Guid reviewId, string kind, string anchorType, string anchorRef, string body, Guid createdBy, System.Threading.CancellationToken cancellationToken = default);
	System.Threading.Tasks.Task<ReviewItem?> GetReviewItemAsync(Guid reviewItemId, System.Threading.CancellationToken cancellationToken = default);
	System.Threading.Tasks.Task<IReadOnlyList<ReviewItem>> GetReviewItemsAsync(Guid reviewId, System.Threading.CancellationToken cancellationToken = default);
	System.Threading.Tasks.Task<ReviewItem> UpdateReviewItemAsync(Guid reviewItemId, string? body = null, string? status = null, System.Threading.CancellationToken cancellationToken = default);
	System.Threading.Tasks.Task DeleteReviewItemAsync(Guid reviewItemId, System.Threading.CancellationToken cancellationToken = default);
	
	System.Threading.Tasks.Task<Comment> AddCommentAsync(Guid reviewItemId, string body, Guid authorId, System.Threading.CancellationToken cancellationToken = default);
	System.Threading.Tasks.Task<IReadOnlyList<Comment>> GetCommentsAsync(Guid reviewItemId, System.Threading.CancellationToken cancellationToken = default);
	System.Threading.Tasks.Task<Comment?> GetCommentAsync(Guid commentId, System.Threading.CancellationToken cancellationToken = default);
	System.Threading.Tasks.Task<Comment> UpdateCommentAsync(Guid commentId, string body, System.Threading.CancellationToken cancellationToken = default);
	System.Threading.Tasks.Task DeleteCommentAsync(Guid commentId, System.Threading.CancellationToken cancellationToken = default);
}


