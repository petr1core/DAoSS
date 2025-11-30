namespace DAOSS.Application.Repositories;

using DAOSS.Domain.Entities;

public interface IReviewItemRepository : IRepository<ReviewItem>
{
	System.Threading.Tasks.Task<IReadOnlyList<ReviewItem>> GetByReviewIdAsync(Guid reviewId, System.Threading.CancellationToken cancellationToken = default);
}

