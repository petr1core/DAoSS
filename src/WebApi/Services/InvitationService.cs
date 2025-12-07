using DAOSS.Application.Interfaces;
using DAOSS.Application.Repositories;
using DAOSS.Domain.Entities;
using DAOSS.Infrastructure.Persistence;

namespace DAOSS.Infrastructure.Services;

public class InvitationService : IInvitationService
{
	private readonly IInvitationRepository _invitationRepository;
	private readonly IProjectMemberRepository _projectMemberRepository;
	private readonly IProjectRepository _projectRepository;
	private readonly IUserRepository _userRepository;
	private readonly IUnitOfWork _unitOfWork;
	private readonly AppDbContext _dbContext;

	private const int InvitationExpirationDays = 7;

	public InvitationService(
		IInvitationRepository invitationRepository,
		IProjectMemberRepository projectMemberRepository,
		IProjectRepository projectRepository,
		IUserRepository userRepository,
		IUnitOfWork unitOfWork,
		AppDbContext dbContext)
	{
		_invitationRepository = invitationRepository;
		_projectMemberRepository = projectMemberRepository;
		_projectRepository = projectRepository;
		_userRepository = userRepository;
		_unitOfWork = unitOfWork;
		_dbContext = dbContext;
	}

	public async System.Threading.Tasks.Task<Invitation> SendInvitationAsync(Guid projectId, Guid invitedUserId, string role, Guid invitedByUserId, System.Threading.CancellationToken cancellationToken = default)
	{
		// Валидация роли
		if (role != "reviewer" && role != "admin")
		{
			throw new ArgumentException("Role must be 'reviewer' or 'admin'", nameof(role));
		}

		// Проверка существования проекта
		var project = await _projectRepository.GetByIdAsync(projectId, cancellationToken);
		if (project == null)
		{
			throw new KeyNotFoundException($"Project with id {projectId} not found");
		}

		// Проверка существования пользователя
		var user = await _userRepository.GetByIdAsync(invitedUserId, cancellationToken);
		if (user == null)
		{
			throw new KeyNotFoundException($"User with id {invitedUserId} not found");
		}

		// Проверка, что пользователь не является уже участником
		var isMember = await _projectMemberRepository.IsMemberAsync(invitedUserId, projectId, cancellationToken);
		if (isMember)
		{
			throw new InvalidOperationException($"User {invitedUserId} is already a member of project {projectId}");
		}

		// Проверка, что нет активного pending приглашения
		var existingPending = await _invitationRepository.GetPendingByProjectAndUserAsync(projectId, invitedUserId, cancellationToken);
		if (existingPending != null)
		{
			throw new InvalidOperationException($"There is already a pending invitation for user {invitedUserId} in project {projectId}");
		}

		// Создание приглашения
		var invitation = new Invitation
		{
			Id = Guid.NewGuid(),
			ProjectId = projectId,
			InvitedUserId = invitedUserId,
			InvitedByUserId = invitedByUserId,
			Role = role,
			Status = "pending",
			ExpiresAt = DateTime.UtcNow.AddDays(InvitationExpirationDays),
			CreatedAt = DateTime.UtcNow
		};

		await _invitationRepository.AddAsync(invitation, cancellationToken);
		await _unitOfWork.SaveChangesAsync(cancellationToken);

		return invitation;
	}

	public async System.Threading.Tasks.Task<Invitation> AcceptInvitationAsync(Guid invitationId, Guid userId, System.Threading.CancellationToken cancellationToken = default)
	{
		var invitation = await _invitationRepository.GetByIdAsync(invitationId, cancellationToken);
		if (invitation == null)
		{
			throw new KeyNotFoundException($"Invitation with id {invitationId} not found");
		}

		// Проверка, что приглашение предназначено для этого пользователя
		if (invitation.InvitedUserId != userId)
		{
			throw new UnauthorizedAccessException($"Invitation {invitationId} is not for user {userId}");
		}

		// Проверка статуса
		if (invitation.Status != "pending")
		{
			throw new InvalidOperationException($"Invitation {invitationId} is not pending (current status: {invitation.Status})");
		}

		// Проверка срока действия
		if (invitation.ExpiresAt <= DateTime.UtcNow)
		{
			invitation.Status = "expired";
			invitation.UpdatedAt = DateTime.UtcNow;
			await _invitationRepository.UpdateAsync(invitation, cancellationToken);
			await _unitOfWork.SaveChangesAsync(cancellationToken);
			throw new InvalidOperationException($"Invitation {invitationId} has expired");
		}

		// Проверка, что пользователь еще не является участником
		var isMember = await _projectMemberRepository.IsMemberAsync(userId, invitation.ProjectId, cancellationToken);
		if (isMember)
		{
			// Обновляем статус приглашения на accepted, даже если уже участник
			invitation.Status = "accepted";
			invitation.UpdatedAt = DateTime.UtcNow;
			await _invitationRepository.UpdateAsync(invitation, cancellationToken);
			await _unitOfWork.SaveChangesAsync(cancellationToken);
			return invitation;
		}

		// Создание ProjectMember
		var projectMember = new ProjectMember
		{
			Id = Guid.NewGuid(),
			ProjectId = invitation.ProjectId,
			UserId = userId,
			Role = invitation.Role,
			AssignedBy = invitation.InvitedByUserId,
			CreatedAt = DateTime.UtcNow
		};

		await _projectMemberRepository.AddAsync(projectMember, cancellationToken);

		// Обновление статуса приглашения
		invitation.Status = "accepted";
		invitation.UpdatedAt = DateTime.UtcNow;
		await _invitationRepository.UpdateAsync(invitation, cancellationToken);

		await _unitOfWork.SaveChangesAsync(cancellationToken);

		return invitation;
	}

	public async System.Threading.Tasks.Task<Invitation> RejectInvitationAsync(Guid invitationId, Guid userId, System.Threading.CancellationToken cancellationToken = default)
	{
		var invitation = await _invitationRepository.GetByIdAsync(invitationId, cancellationToken);
		if (invitation == null)
		{
			throw new KeyNotFoundException($"Invitation with id {invitationId} not found");
		}

		// Проверка, что приглашение предназначено для этого пользователя
		if (invitation.InvitedUserId != userId)
		{
			throw new UnauthorizedAccessException($"Invitation {invitationId} is not for user {userId}");
		}

		// Проверка статуса
		if (invitation.Status != "pending")
		{
			throw new InvalidOperationException($"Invitation {invitationId} is not pending (current status: {invitation.Status})");
		}

		// Обновление статуса приглашения
		invitation.Status = "rejected";
		invitation.UpdatedAt = DateTime.UtcNow;
		await _invitationRepository.UpdateAsync(invitation, cancellationToken);
		await _unitOfWork.SaveChangesAsync(cancellationToken);

		return invitation;
	}

	public async System.Threading.Tasks.Task<IReadOnlyList<Invitation>> GetProjectInvitationsAsync(Guid projectId, System.Threading.CancellationToken cancellationToken = default)
	{
		return await _invitationRepository.GetByProjectIdAsync(projectId, cancellationToken);
	}

	public async System.Threading.Tasks.Task<IReadOnlyList<Invitation>> GetUserInvitationsAsync(Guid userId, System.Threading.CancellationToken cancellationToken = default)
	{
		return await _invitationRepository.GetByUserIdAsync(userId, cancellationToken);
	}

	public async System.Threading.Tasks.Task<Invitation?> GetByIdAsync(Guid invitationId, System.Threading.CancellationToken cancellationToken = default)
	{
		return await _invitationRepository.GetByIdAsync(invitationId, cancellationToken);
	}

	public async System.Threading.Tasks.Task CancelInvitationAsync(Guid invitationId, Guid projectId, Guid canceledByUserId, System.Threading.CancellationToken cancellationToken = default)
	{
		var invitation = await _invitationRepository.GetByIdAsync(invitationId, cancellationToken);
		if (invitation == null)
		{
			throw new KeyNotFoundException($"Invitation with id {invitationId} not found");
		}

		// Проверка, что приглашение относится к проекту
		if (invitation.ProjectId != projectId)
		{
			throw new InvalidOperationException($"Invitation {invitationId} does not belong to project {projectId}");
		}

		// Проверка статуса - можно отменять только pending приглашения
		if (invitation.Status != "pending")
		{
			throw new InvalidOperationException($"Cannot cancel invitation {invitationId} with status {invitation.Status}");
		}

		// Проверка прав: тот, кто отправил, или другой admin/owner
		// Проверяем, является ли пользователь владельцем проекта
		var project = await _projectRepository.GetByIdAsync(projectId, cancellationToken);
		if (project == null)
		{
			throw new KeyNotFoundException($"Project with id {projectId} not found");
		}

		var isOwner = project.OwnerId == canceledByUserId;
		var isInviter = invitation.InvitedByUserId == canceledByUserId;
		var userRole = await _projectMemberRepository.GetUserRoleAsync(canceledByUserId, projectId, cancellationToken);
		var isAdmin = userRole == "admin" || userRole == "owner";

		if (!isOwner && !isInviter && !isAdmin)
		{
			throw new UnauthorizedAccessException($"User {canceledByUserId} is not authorized to cancel invitation {invitationId}");
		}

		// Удаление приглашения
		await _invitationRepository.DeleteAsync(invitationId, cancellationToken);
		await _unitOfWork.SaveChangesAsync(cancellationToken);
	}

	public async System.Threading.Tasks.Task CleanupExpiredInvitationsAsync(System.Threading.CancellationToken cancellationToken = default)
	{
		var expiredInvitations = await _invitationRepository.GetPendingExpiredAsync(cancellationToken);
		
		foreach (var invitation in expiredInvitations)
		{
			invitation.Status = "expired";
			invitation.UpdatedAt = DateTime.UtcNow;
			await _invitationRepository.UpdateAsync(invitation, cancellationToken);
		}

		if (expiredInvitations.Count > 0)
		{
			await _unitOfWork.SaveChangesAsync(cancellationToken);
		}
	}
}

