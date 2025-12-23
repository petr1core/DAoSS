using DAOSS.Application.Repositories;
using DAOSS.Domain.Entities;
using DAOSS.Infrastructure.Persistence;
using Microsoft.EntityFrameworkCore;
using System.Linq;

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
		// Возвращаем проекты, где пользователь является владельцем или участником
		var ownedProjects = await _dbContext.Projects
			.Where(p => p.OwnerId == userId)
			.Select(p => p.Id)
			.ToListAsync(cancellationToken);

		var memberProjectIds = await _dbContext.ProjectMembers
			.Where(pm => pm.UserId == userId)
			.Select(pm => pm.ProjectId)
			.Distinct()
			.ToListAsync(cancellationToken);

		var allProjectIds = ownedProjects.Union(memberProjectIds).Distinct().ToList();

		return await _dbContext.Projects
			.Where(p => allProjectIds.Contains(p.Id))
			.ToListAsync(cancellationToken);
	}
}


