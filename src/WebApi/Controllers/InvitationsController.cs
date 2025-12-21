using System.ComponentModel.DataAnnotations;
using System.Security.Claims;
using DAOSS.Application.Interfaces;
using DAOSS.Application.Repositories;
using DAOSS.Domain.Entities;
using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc;

namespace DAOSS.WebApi.Controllers;

[ApiController]
public class InvitationsController : ControllerBase
{
	private readonly IInvitationService _invitationService;
	private readonly IProjectRepository _projectRepository;
	private readonly IProjectMemberRepository _projectMemberRepository;

	public InvitationsController(
		IInvitationService invitationService,
		IProjectRepository projectRepository,
		IProjectMemberRepository projectMemberRepository)
	{
		_invitationService = invitationService;
		_projectRepository = projectRepository;
		_projectMemberRepository = projectMemberRepository;
	}

	public sealed class SendInvitationDto
	{
		[Required]
		public Guid InvitedUserId { get; set; }
		[Required]
		[RegularExpression("^(admin|reviewer)$", ErrorMessage = "role must be 'admin' or 'reviewer'")]
		public string Role { get; set; } = "reviewer";
	}

	public sealed class InvitationResponseDto
	{
		public Guid Id { get; set; }
		public Guid ProjectId { get; set; }
		public Guid InvitedUserId { get; set; }
		public Guid InvitedByUserId { get; set; }
		public string Role { get; set; } = string.Empty;
		public string Status { get; set; } = string.Empty;
		public DateTime ExpiresAt { get; set; }
		public DateTime CreatedAt { get; set; }
	}

	[HttpPost("api/projects/{projectId:guid}/invitations")]
	[Authorize(Policy = "ProjectAdmin")]
	public async Task<ActionResult<InvitationResponseDto>> SendInvitation(
		Guid projectId,
		[FromBody] SendInvitationDto dto,
		CancellationToken ct)
	{
		if (!ModelState.IsValid)
		{
			return BadRequest(ModelState);
		}

		var currentUserIdStr = User.FindFirstValue(ClaimTypes.NameIdentifier);
		if (!Guid.TryParse(currentUserIdStr, out var invitedByUserId))
		{
			return Unauthorized(new { message = "Invalid user identity" });
		}

		try
		{
			var invitation = await _invitationService.SendInvitationAsync(
				projectId,
				dto.InvitedUserId,
				dto.Role,
				invitedByUserId,
				ct);

			var response = new InvitationResponseDto
			{
				Id = invitation.Id,
				ProjectId = invitation.ProjectId,
				InvitedUserId = invitation.InvitedUserId,
				InvitedByUserId = invitation.InvitedByUserId,
				Role = invitation.Role,
				Status = invitation.Status,
				ExpiresAt = invitation.ExpiresAt,
				CreatedAt = invitation.CreatedAt
			};

			return CreatedAtAction(
				nameof(GetInvitation),
				new { invitationId = invitation.Id },
				response);
		}
		catch (KeyNotFoundException ex)
		{
			return NotFound(new { message = ex.Message });
		}
		catch (InvalidOperationException ex)
		{
			return Conflict(new { message = ex.Message });
		}
		catch (ArgumentException ex)
		{
			return BadRequest(new { message = ex.Message });
		}
	}

	[HttpGet("api/projects/{projectId:guid}/invitations")]
	[Authorize(Policy = "ProjectAdmin")]
	public async Task<ActionResult<IReadOnlyList<InvitationResponseDto>>> GetProjectInvitations(
		Guid projectId,
		[FromQuery] string? status = null,
		CancellationToken ct = default)
	{
		var invitations = await _invitationService.GetProjectInvitationsAsync(projectId, ct);

		var filtered = invitations;
		if (!string.IsNullOrWhiteSpace(status))
		{
			filtered = invitations.Where(i => i.Status.Equals(status, StringComparison.OrdinalIgnoreCase)).ToList();
		}

		var response = filtered.Select(i => new InvitationResponseDto
		{
			Id = i.Id,
			ProjectId = i.ProjectId,
			InvitedUserId = i.InvitedUserId,
			InvitedByUserId = i.InvitedByUserId,
			Role = i.Role,
			Status = i.Status,
			ExpiresAt = i.ExpiresAt,
			CreatedAt = i.CreatedAt
		}).ToList();

		return Ok(response);
	}

	[HttpGet("api/invitations")]
	[Authorize]
	public async Task<ActionResult<IReadOnlyList<InvitationResponseDto>>> GetUserInvitations(
		CancellationToken ct = default)
	{
		var currentUserIdStr = User.FindFirstValue(ClaimTypes.NameIdentifier);
		if (!Guid.TryParse(currentUserIdStr, out var userId))
		{
			return Unauthorized(new { message = "Invalid user identity" });
		}

		var invitations = await _invitationService.GetUserInvitationsAsync(userId, ct);

		var response = invitations.Select(i => new InvitationResponseDto
		{
			Id = i.Id,
			ProjectId = i.ProjectId,
			InvitedUserId = i.InvitedUserId,
			InvitedByUserId = i.InvitedByUserId,
			Role = i.Role,
			Status = i.Status,
			ExpiresAt = i.ExpiresAt,
			CreatedAt = i.CreatedAt
		}).ToList();

		return Ok(response);
	}

	[HttpPost("api/invitations/{invitationId:guid}/accept")]
	[Authorize]
	public async Task<ActionResult<InvitationResponseDto>> AcceptInvitation(
		Guid invitationId,
		CancellationToken ct = default)
	{
		var currentUserIdStr = User.FindFirstValue(ClaimTypes.NameIdentifier);
		if (!Guid.TryParse(currentUserIdStr, out var userId))
		{
			return Unauthorized(new { message = "Invalid user identity" });
		}

		try
		{
			var invitation = await _invitationService.AcceptInvitationAsync(invitationId, userId, ct);

			var response = new InvitationResponseDto
			{
				Id = invitation.Id,
				ProjectId = invitation.ProjectId,
				InvitedUserId = invitation.InvitedUserId,
				InvitedByUserId = invitation.InvitedByUserId,
				Role = invitation.Role,
				Status = invitation.Status,
				ExpiresAt = invitation.ExpiresAt,
				CreatedAt = invitation.CreatedAt
			};

			return Ok(response);
		}
		catch (KeyNotFoundException ex)
		{
			return NotFound(new { message = ex.Message });
		}
		catch (UnauthorizedAccessException ex)
		{
			return Forbid(ex.Message);
		}
		catch (InvalidOperationException ex)
		{
			return BadRequest(new { message = ex.Message });
		}
	}

	[HttpPost("api/invitations/{invitationId:guid}/reject")]
	[Authorize]
	public async Task<ActionResult<InvitationResponseDto>> RejectInvitation(
		Guid invitationId,
		CancellationToken ct = default)
	{
		var currentUserIdStr = User.FindFirstValue(ClaimTypes.NameIdentifier);
		if (!Guid.TryParse(currentUserIdStr, out var userId))
		{
			return Unauthorized(new { message = "Invalid user identity" });
		}

		try
		{
			var invitation = await _invitationService.RejectInvitationAsync(invitationId, userId, ct);

			var response = new InvitationResponseDto
			{
				Id = invitation.Id,
				ProjectId = invitation.ProjectId,
				InvitedUserId = invitation.InvitedUserId,
				InvitedByUserId = invitation.InvitedByUserId,
				Role = invitation.Role,
				Status = invitation.Status,
				ExpiresAt = invitation.ExpiresAt,
				CreatedAt = invitation.CreatedAt
			};

			return Ok(response);
		}
		catch (KeyNotFoundException ex)
		{
			return NotFound(new { message = ex.Message });
		}
		catch (UnauthorizedAccessException ex)
		{
			return Forbid(ex.Message);
		}
		catch (InvalidOperationException ex)
		{
			return BadRequest(new { message = ex.Message });
		}
	}

	[HttpDelete("api/projects/{projectId:guid}/invitations/{invitationId:guid}")]
	[Authorize(Policy = "ProjectAdmin")]
	public async Task<IActionResult> CancelInvitation(
		Guid projectId,
		Guid invitationId,
		CancellationToken ct = default)
	{
		var currentUserIdStr = User.FindFirstValue(ClaimTypes.NameIdentifier);
		if (!Guid.TryParse(currentUserIdStr, out var canceledByUserId))
		{
			return Unauthorized(new { message = "Invalid user identity" });
		}

		try
		{
			await _invitationService.CancelInvitationAsync(invitationId, projectId, canceledByUserId, ct);
			return NoContent();
		}
		catch (KeyNotFoundException ex)
		{
			return NotFound(new { message = ex.Message });
		}
		catch (UnauthorizedAccessException ex)
		{
			return Forbid(ex.Message);
		}
		catch (InvalidOperationException ex)
		{
			return BadRequest(new { message = ex.Message });
		}
	}

	[HttpGet("api/invitations/{invitationId:guid}")]
	[Authorize]
	public async Task<ActionResult<InvitationResponseDto>> GetInvitation(
		Guid invitationId,
		CancellationToken ct = default)
	{
		var invitation = await _invitationService.GetByIdAsync(invitationId, ct);
		if (invitation == null)
		{
			return NotFound(new { message = $"Invitation with id {invitationId} not found" });
		}

		var currentUserIdStr = User.FindFirstValue(ClaimTypes.NameIdentifier);
		if (!Guid.TryParse(currentUserIdStr, out var currentUserId))
		{
			return Unauthorized(new { message = "Invalid user identity" });
		}

		// Проверка прав: приглашенный пользователь или админ/владелец проекта
		var isInvitedUser = invitation.InvitedUserId == currentUserId;
		
		if (!isInvitedUser)
		{
			// Проверяем, является ли пользователь админом или владельцем проекта
			var project = await _projectRepository.GetByIdAsync(invitation.ProjectId, ct);
			if (project == null)
			{
				return NotFound(new { message = $"Project with id {invitation.ProjectId} not found" });
			}

			var isOwner = project.OwnerId == currentUserId;
			var userRole = await _projectMemberRepository.GetUserRoleAsync(currentUserId, invitation.ProjectId, ct);
			var isAdmin = userRole == "admin" || userRole == "owner";

			if (!isOwner && !isAdmin)
			{
				return Forbid("You don't have permission to view this invitation");
			}
		}

		var response = new InvitationResponseDto
		{
			Id = invitation.Id,
			ProjectId = invitation.ProjectId,
			InvitedUserId = invitation.InvitedUserId,
			InvitedByUserId = invitation.InvitedByUserId,
			Role = invitation.Role,
			Status = invitation.Status,
			ExpiresAt = invitation.ExpiresAt,
			CreatedAt = invitation.CreatedAt
		};

		return Ok(response);
	}
}

