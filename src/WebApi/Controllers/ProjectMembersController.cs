using System.ComponentModel.DataAnnotations;
using System.Linq;
using DAOSS.Application.Repositories;
using DAOSS.Domain.Entities;
using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc;

namespace DAOSS.WebApi.Controllers;

[ApiController]
[Route("api/projects/{projectId:guid}/members")]
public class ProjectMembersController : ControllerBase
{
	private readonly IProjectMemberRepository _members;
	private readonly IProjectRepository _projects;
	private readonly IUnitOfWork _uow;

	public ProjectMembersController(IProjectMemberRepository members, IProjectRepository projects, IUnitOfWork uow)
	{
		_members = members;
		_projects = projects;
		_uow = uow;
	}

	public sealed class CreateMemberDto
	{
		[Required]
		public Guid UserId { get; set; }
		[Required]
		[RegularExpression("^(owner|admin|reviewer)$", ErrorMessage = "role must be 'owner', 'admin' or 'reviewer'")]
		public string Role { get; set; } = "reviewer";
	}

	public sealed class UpdateRoleDto
	{
		[Required]
		[RegularExpression("^(owner|admin|reviewer)$", ErrorMessage = "role must be 'owner', 'admin' or 'reviewer'")]
		public string Role { get; set; } = string.Empty;
	}

	[HttpPost]
	[Authorize(Policy = "ProjectAdmin")]
	public async Task<IActionResult> Create(Guid projectId, [FromBody] CreateMemberDto dto, CancellationToken ct)
	{
		var already = await _members.IsMemberAsync(dto.UserId, projectId, ct);
		if (already)
		{
			return Conflict(new { message = "User is already a member of the project" });
		}

		var entity = new ProjectMember
		{
			Id = Guid.NewGuid(),
			ProjectId = projectId,
			UserId = dto.UserId,
			Role = dto.Role,
			CreatedAt = DateTime.UtcNow
		};

		await _members.AddAsync(entity, ct);
		await _uow.SaveChangesAsync(ct);
		return CreatedAtAction(nameof(Get), new { projectId, userId = entity.UserId }, new { entity.ProjectId, entity.UserId, entity.Role });
	}

	[HttpGet("{userId:guid}")]
	[Authorize(Policy = "ProjectRead")]
	public async Task<IActionResult> Get(Guid projectId, Guid userId, CancellationToken ct)
	{
		var role = await _members.GetUserRoleAsync(userId, projectId, ct);
		if (role is null) return NotFound();
		return Ok(new { projectId, userId, role });
	}

	[HttpGet]
	[Authorize(Policy = "ProjectRead")]
	public async Task<ActionResult<IReadOnlyList<object>>> GetAll(Guid projectId, CancellationToken ct)
	{
		var members = await _members.GetByProjectIdAsync(projectId, ct);
		var result = members.Select(m => new { m.ProjectId, m.UserId, m.Role, m.CreatedAt }).ToList();
		return Ok(result);
	}

	[HttpPut("{userId:guid}")]
	[Authorize(Policy = "ProjectAdmin")]
	public async Task<IActionResult> UpdateRole(Guid projectId, Guid userId, [FromBody] UpdateRoleDto dto, CancellationToken ct)
	{
		// Получаем текущего пользователя из JWT
		var currentUserIdStr = User.FindFirst(System.Security.Claims.ClaimTypes.NameIdentifier)?.Value;
		if (!Guid.TryParse(currentUserIdStr, out var currentUserId))
		{
			return Unauthorized(new { message = "Invalid user identity" });
		}

		// Проверяем, что текущий пользователь - owner проекта
		var currentUserRole = await _members.GetUserRoleAsync(currentUserId, projectId, ct);
		if (currentUserRole == null)
		{
			var project = await _projects.GetByIdAsync(projectId, ct);
			if (project == null || project.OwnerId != currentUserId)
			{
				return StatusCode(403, new { message = "Only project owner can change roles" });
			}
			currentUserRole = "owner";
		}
		else if (currentUserRole != "owner")
		{
			return StatusCode(403, new { message = "Only project owner can change roles" });
		}

		// Получаем участника проекта
		var member = await _members.GetByProjectAndUserAsync(projectId, userId, ct);
		if (member is null)
		{
			return NotFound(new { message = "User is not a member of this project" });
		}

		var currentRole = member.Role;
		var newRole = dto.Role.ToLowerInvariant();

		// Проверяем, что роль изменилась
		if (currentRole.ToLowerInvariant() == newRole)
		{
			return Conflict(new { message = "Role is already set to this value" });
		}

		// Специальный случай: передача владения
		if (newRole == "owner")
		{
			// Текущий owner передает владение другому участнику
			if (currentRole != "owner")
			{
				// Обновляем проект: новый owner
				var project = await _projects.GetByIdAsync(projectId, ct);
				if (project == null)
				{
					return NotFound(new { message = "Project not found" });
				}

				// Старый owner становится admin
				var oldOwnerMember = await _members.GetByProjectAndUserAsync(projectId, currentUserId, ct);
				if (oldOwnerMember != null)
				{
					oldOwnerMember.Role = "admin";
					oldOwnerMember.AssignedBy = currentUserId;
					oldOwnerMember.UpdatedAt = DateTime.UtcNow;
					await _members.UpdateAsync(oldOwnerMember, ct);
				}
				else
				{
					// Если owner не был в таблице ProjectMembers, создаем запись
					oldOwnerMember = new ProjectMember
					{
						Id = Guid.NewGuid(),
						ProjectId = projectId,
						UserId = currentUserId,
						Role = "admin",
						AssignedBy = currentUserId,
						CreatedAt = DateTime.UtcNow,
						UpdatedAt = DateTime.UtcNow
					};
					await _members.AddAsync(oldOwnerMember, ct);
				}

				// Новый участник становится owner
				member.Role = "owner";
				member.AssignedBy = currentUserId;
				member.UpdatedAt = DateTime.UtcNow;
				await _members.UpdateAsync(member, ct);

				// Обновляем OwnerId в проекте
				project.OwnerId = userId;
				project.UpdatedAt = DateTime.UtcNow;
				await _projects.UpdateAsync(project, ct);

				await _uow.SaveChangesAsync(ct);
				return Ok(new { projectId, userId, role = member.Role, message = "Project ownership transferred" });
			}
			else
			{
				return BadRequest(new { message = "User is already the project owner" });
			}
		}

		// Нельзя изменить роль текущего owner (кроме передачи владения, которое обработано выше)
		if (currentRole == "owner")
		{
			return BadRequest(new { message = "Cannot change role of project owner. Use ownership transfer instead." });
		}

		// Проверка: нельзя понизить последнего admin до reviewer
		if (currentRole == "admin" && newRole == "reviewer")
		{
			var project = await _projects.GetByIdAsync(projectId, ct);
			if (project == null)
			{
				return NotFound(new { message = "Project not found" });
			}

			// Owner проекта всегда имеет права admin, поэтому если owner существует и это не тот пользователь,
			// которого мы понижаем, то это не последний admin
			if (project.OwnerId != userId)
			{
				// Owner существует и это не тот пользователь, которого мы понижаем
				// Owner всегда имеет права admin, поэтому разрешаем понижение
			}
			else
			{
				// Проверяем, есть ли другие admin в проекте
				var allMembers = await _members.GetByProjectIdAsync(projectId, ct);
				var otherAdminMembers = allMembers.Where(m => m.Role.ToLowerInvariant() == "admin" && m.UserId != userId).ToList();
				
				if (otherAdminMembers.Count == 0)
				{
					// Нет других admin, кроме того, которого мы понижаем
					return BadRequest(new { message = "Cannot demote the last admin to reviewer" });
				}
			}
		}

		// Обновляем роль
		member.Role = newRole;
		member.AssignedBy = currentUserId;
		member.UpdatedAt = DateTime.UtcNow;
		await _members.UpdateAsync(member, ct);
		await _uow.SaveChangesAsync(ct);

		return Ok(new { projectId, userId, role = member.Role });
	}

	[HttpDelete("{userId:guid}")]
	[Authorize(Policy = "ProjectAdmin")]
	public async Task<IActionResult> Delete(Guid projectId, Guid userId, CancellationToken ct)
	{
		var member = await _members.GetByProjectAndUserAsync(projectId, userId, ct);
		if (member is null)
		{
			return NotFound(new { message = "User is not a member of this project" });
		}

		// Нельзя исключить владельца проекта (owner)
		if (member.Role == "owner")
		{
			return BadRequest(new { message = "Cannot remove project owner" });
		}

		// Получаем текущего пользователя из JWT
		var currentUserIdStr = User.FindFirst(System.Security.Claims.ClaimTypes.NameIdentifier)?.Value;
		if (!Guid.TryParse(currentUserIdStr, out var currentUserId))
		{
			return Unauthorized(new { message = "Invalid user identity" });
		}

		// Нельзя исключить самого себя, если ты admin
		var currentUserRole = await _members.GetUserRoleAsync(currentUserId, projectId, ct);
		if (currentUserRole == "admin" && userId == currentUserId)
		{
			return BadRequest(new { message = "Admin cannot remove themselves from the project" });
		}

		await _members.DeleteAsync(member.Id, ct);
		await _uow.SaveChangesAsync(ct);
		return NoContent();
	}
}


