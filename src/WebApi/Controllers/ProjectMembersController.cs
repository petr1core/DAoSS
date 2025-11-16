using System.ComponentModel.DataAnnotations;
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
	private readonly IUnitOfWork _uow;

	public ProjectMembersController(IProjectMemberRepository members, IUnitOfWork uow)
	{
		_members = members;
		_uow = uow;
	}

	public sealed class CreateMemberDto
	{
		[Required]
		public Guid UserId { get; set; }
		[Required]
		[RegularExpression("^(owner|reviewer)$", ErrorMessage = "role must be 'owner' or 'reviewer'")]
		public string Role { get; set; } = "reviewer";
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
}


