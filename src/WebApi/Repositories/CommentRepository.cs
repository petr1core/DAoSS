using DAOSS.Application.Repositories;
using DAOSS.Domain.Entities;
using DAOSS.Infrastructure.Persistence;
using Microsoft.EntityFrameworkCore;

namespace DAOSS.Infrastructure.Repositories;

public class CommentRepository : EfRepository<Comment>, ICommentRepository
{
	private readonly AppDbContext _dbContext;

	public CommentRepository(AppDbContext dbContext) : base(dbContext)
	{
		_dbContext = dbContext;
	}

	public async System.Threading.Tasks.Task<IReadOnlyList<Comment>> GetByReviewItemIdAsync(Guid reviewItemId, System.Threading.CancellationToken cancellationToken = default)
	{
		return await _dbContext.Comments
			.Where(c => c.ReviewItemId == reviewItemId)
			.ToListAsync(cancellationToken);
	}
}


