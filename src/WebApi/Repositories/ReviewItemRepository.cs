using DAOSS.Application.Repositories;
using DAOSS.Domain.Entities;
using DAOSS.Infrastructure.Persistence;
using Microsoft.EntityFrameworkCore;

namespace DAOSS.Infrastructure.Repositories;

public class ReviewItemRepository : EfRepository<ReviewItem>, IReviewItemRepository
{
	private readonly AppDbContext _dbContext;

	public ReviewItemRepository(AppDbContext dbContext) : base(dbContext)
	{
		_dbContext = dbContext;
	}

	public async System.Threading.Tasks.Task<IReadOnlyList<ReviewItem>> GetByReviewIdAsync(Guid reviewId, System.Threading.CancellationToken cancellationToken = default)
	{
		return await _dbContext.Set<ReviewItem>()
			.Where(ri => ri.ReviewId == reviewId)
			.ToListAsync(cancellationToken);
	}
}

