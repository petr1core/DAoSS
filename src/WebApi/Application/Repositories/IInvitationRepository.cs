namespace DAOSS.Application.Repositories;

using DAOSS.Domain.Entities;

public interface IInvitationRepository : IRepository<Invitation>
{
	System.Threading.Tasks.Task<IReadOnlyList<Invitation>> GetByProjectIdAsync(Guid projectId, System.Threading.CancellationToken cancellationToken = default);
	System.Threading.Tasks.Task<IReadOnlyList<Invitation>> GetByUserIdAsync(Guid userId, System.Threading.CancellationToken cancellationToken = default);
	System.Threading.Tasks.Task<Invitation?> GetPendingByProjectAndUserAsync(Guid projectId, Guid userId, System.Threading.CancellationToken cancellationToken = default);
	System.Threading.Tasks.Task<IReadOnlyList<Invitation>> GetPendingExpiredAsync(System.Threading.CancellationToken cancellationToken = default);
}

