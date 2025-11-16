using DAOSS.Application.Repositories;
using DAOSS.Domain.Entities;
using DAOSS.Infrastructure.Persistence;
using Microsoft.EntityFrameworkCore;

namespace DAOSS.Infrastructure.Repositories;

public class ReviewRepository : EfRepository<Review>, IReviewRepository
{
	private readonly AppDbContext _dbContext;

	public ReviewRepository(AppDbContext dbContext) : base(dbContext)
	{
		_dbContext = dbContext;
	}

	public async System.Threading.Tasks.Task<IReadOnlyList<Review>> GetByProjectIdAsync(Guid projectId, System.Threading.CancellationToken cancellationToken = default)
	{
		return await _dbContext.Set<Review>()
			.Where(r => r.ProjectId == projectId)
			.ToListAsync(cancellationToken);
	}
}


