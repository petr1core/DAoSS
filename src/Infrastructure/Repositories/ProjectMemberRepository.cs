using DAOSS.Application.Repositories;
using DAOSS.Domain.Entities;
using DAOSS.Infrastructure.Persistence;
using Microsoft.EntityFrameworkCore;

namespace DAOSS.Infrastructure.Repositories;

public class ProjectMemberRepository : EfRepository<ProjectMember>, IProjectMemberRepository
{
	private readonly AppDbContext _dbContext;

	public ProjectMemberRepository(AppDbContext dbContext) : base(dbContext)
	{
		_dbContext = dbContext;
	}

	public async System.Threading.Tasks.Task<string?> GetUserRoleAsync(Guid userId, Guid projectId, System.Threading.CancellationToken cancellationToken = default)
	{
		var member = await _dbContext.ProjectMembers
			.AsNoTracking()
			.FirstOrDefaultAsync(pm => pm.UserId == userId && pm.ProjectId == projectId, cancellationToken);
		return member?.Role;
	}

	public async System.Threading.Tasks.Task<bool> IsMemberAsync(Guid userId, Guid projectId, System.Threading.CancellationToken cancellationToken = default)
	{
		return await _dbContext.ProjectMembers
			.AsNoTracking()
			.AnyAsync(pm => pm.UserId == userId && pm.ProjectId == projectId, cancellationToken);
	}
}


