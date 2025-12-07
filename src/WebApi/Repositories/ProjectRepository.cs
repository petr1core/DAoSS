using DAOSS.Application.Repositories;
using DAOSS.Domain.Entities;
using DAOSS.Infrastructure.Persistence;
using Microsoft.EntityFrameworkCore;

namespace DAOSS.Infrastructure.Repositories;

public class ProjectRepository : EfRepository<Project>, IProjectRepository
{
	private readonly AppDbContext _dbContext;

	public ProjectRepository(AppDbContext dbContext) : base(dbContext)
	{
		_dbContext = dbContext;
	}

	public async System.Threading.Tasks.Task<IReadOnlyList<Project>> GetByUserIdAsync(Guid userId, System.Threading.CancellationToken cancellationToken = default)
	{
		return await _dbContext.Projects
			.Where(p => p.OwnerId == userId)
			.ToListAsync(cancellationToken);
	}
}


