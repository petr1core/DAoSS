namespace DAOSS.Application.Interfaces;

using DAOSS.Domain.Entities;

public interface IInvitationService
{
	System.Threading.Tasks.Task<Invitation> SendInvitationAsync(Guid projectId, Guid invitedUserId, string role, Guid invitedByUserId, System.Threading.CancellationToken cancellationToken = default);
	System.Threading.Tasks.Task<Invitation> AcceptInvitationAsync(Guid invitationId, Guid userId, System.Threading.CancellationToken cancellationToken = default);
	System.Threading.Tasks.Task<Invitation> RejectInvitationAsync(Guid invitationId, Guid userId, System.Threading.CancellationToken cancellationToken = default);
	System.Threading.Tasks.Task<IReadOnlyList<Invitation>> GetProjectInvitationsAsync(Guid projectId, System.Threading.CancellationToken cancellationToken = default);
	System.Threading.Tasks.Task<IReadOnlyList<Invitation>> GetUserInvitationsAsync(Guid userId, System.Threading.CancellationToken cancellationToken = default);
	System.Threading.Tasks.Task<Invitation?> GetByIdAsync(Guid invitationId, System.Threading.CancellationToken cancellationToken = default);
	System.Threading.Tasks.Task CancelInvitationAsync(Guid invitationId, Guid projectId, Guid canceledByUserId, System.Threading.CancellationToken cancellationToken = default);
	System.Threading.Tasks.Task CleanupExpiredInvitationsAsync(System.Threading.CancellationToken cancellationToken = default);
}

