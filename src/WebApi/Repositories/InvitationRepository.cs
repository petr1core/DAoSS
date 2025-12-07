using DAOSS.Application.Repositories;
using DAOSS.Domain.Entities;
using DAOSS.Infrastructure.Persistence;
using Microsoft.EntityFrameworkCore;

namespace DAOSS.Infrastructure.Repositories;

public class InvitationRepository : EfRepository<Invitation>, IInvitationRepository
{
	private readonly AppDbContext _dbContext;

	public InvitationRepository(AppDbContext dbContext) : base(dbContext)
	{
		_dbContext = dbContext;
	}

	public async System.Threading.Tasks.Task<IReadOnlyList<Invitation>> GetByProjectIdAsync(Guid projectId, System.Threading.CancellationToken cancellationToken = default)
	{
		return await _dbContext.Invitations
			.AsNoTracking()
			.Where(i => i.ProjectId == projectId)
			.OrderByDescending(i => i.CreatedAt)
			.ToListAsync(cancellationToken);
	}

	public async System.Threading.Tasks.Task<IReadOnlyList<Invitation>> GetByUserIdAsync(Guid userId, System.Threading.CancellationToken cancellationToken = default)
	{
		return await _dbContext.Invitations
			.AsNoTracking()
			.Where(i => i.InvitedUserId == userId)
			.OrderByDescending(i => i.CreatedAt)
			.ToListAsync(cancellationToken);
	}

	public async System.Threading.Tasks.Task<Invitation?> GetPendingByProjectAndUserAsync(Guid projectId, Guid userId, System.Threading.CancellationToken cancellationToken = default)
	{
		return await _dbContext.Invitations
			.AsNoTracking()
			.FirstOrDefaultAsync(i => i.ProjectId == projectId 
				&& i.InvitedUserId == userId 
				&& i.Status == "pending"
				&& i.ExpiresAt > DateTime.UtcNow, cancellationToken);
	}

	public async System.Threading.Tasks.Task<IReadOnlyList<Invitation>> GetPendingExpiredAsync(System.Threading.CancellationToken cancellationToken = default)
	{
		return await _dbContext.Invitations
			.AsNoTracking()
			.Where(i => i.Status == "pending" && i.ExpiresAt <= DateTime.UtcNow)
			.ToListAsync(cancellationToken);
	}
}

