using System.ComponentModel.DataAnnotations;
using DAOSS.Application.Repositories;
using DAOSS.Domain.Entities;
using Microsoft.AspNetCore.Mvc;
using Microsoft.AspNetCore.Authorization;

namespace DAOSS.WebApi.Controllers;

[ApiController]
[Route("api/[controller]")]
public class ProjectsController : ControllerBase
{
	private readonly IProjectRepository _projects;
	private readonly IUnitOfWork _uow;
	private readonly IProjectMemberRepository _members;

	public ProjectsController(IProjectRepository projects, IUnitOfWork uow, IProjectMemberRepository members)
	{
		_projects = projects;
		_uow = uow;
		_members = members;
	}

	[HttpGet]
	[Authorize] // список по ownerId — разрешаем авторизованным; позже можно сверять ownerId == current user
	public async Task<ActionResult<IReadOnlyList<Project>>> GetByOwner([FromQuery] Guid? ownerId, CancellationToken ct)
	{
		if (ownerId is null)
		{
			return BadRequest("ownerId is required");
		}
		var items = await _projects.GetByUserIdAsync(ownerId.Value, ct);
		return Ok(items);
	}

	[HttpGet("{id:guid}")]
	[Authorize(Policy = "ProjectRead")]
	public async Task<ActionResult<Project>> GetById(Guid id, CancellationToken ct)
	{
		var project = await _projects.GetByIdAsync(id, ct);
		if (project is null) return NotFound();
		return Ok(project);
	}

	public sealed class ProjectCreateUpdateDto
	{
		[Required]
		public string Name { get; set; } = string.Empty;
		[Required]
		public string Description { get; set; } = string.Empty;
		[Required]
		public Guid OwnerId { get; set; }
		public Guid DefaultLanguageId { get; set; }
		public string Visibility { get; set; } = "private";
		public string? RequiredReviewersRules { get; set; } // JSON строка с правилами ревью, например: [{"Role":"Admin","Count":2},{"Role":"Owner"}]
	}

	[HttpPost]
	[Authorize] // создание проекта доступно авторизованным
	public async Task<ActionResult<Project>> Create([FromBody] ProjectCreateUpdateDto dto, CancellationToken ct)
	{
		// Валидация JSON формата правил ревью (если указано)
		if (!string.IsNullOrWhiteSpace(dto.RequiredReviewersRules))
		{
			try
			{
				System.Text.Json.JsonSerializer.Deserialize<List<object>>(dto.RequiredReviewersRules);
			}
			catch (System.Text.Json.JsonException)
			{
				return BadRequest(new { message = "RequiredReviewersRules must be a valid JSON array" });
			}
		}

		var entity = new Project
		{
			Id = Guid.NewGuid(),
			Name = dto.Name,
			Description = dto.Description,
			OwnerId = dto.OwnerId,
			DefaultLanguageId = dto.DefaultLanguageId,
			Visibility = string.IsNullOrWhiteSpace(dto.Visibility) ? "private" : dto.Visibility,
			RequiredReviewersRules = dto.RequiredReviewersRules,
			CreatedAt = DateTime.UtcNow
		};
		await _projects.AddAsync(entity, ct);
		// авто-добавление владельца в участники с ролью owner
		await _members.AddAsync(new ProjectMember
		{
			Id = Guid.NewGuid(),
			ProjectId = entity.Id,
			UserId = entity.OwnerId,
			Role = "owner",
			CreatedAt = DateTime.UtcNow
		}, ct);
		await _uow.SaveChangesAsync(ct);
		return CreatedAtAction(nameof(GetById), new { id = entity.Id }, entity);
	}

	[HttpPut("{id:guid}")]
	[Authorize(Policy = "ProjectWrite")]
	public async Task<IActionResult> Update(Guid id, [FromBody] ProjectCreateUpdateDto dto, CancellationToken ct)
	{
		var existing = await _projects.GetByIdAsync(id, ct);
		if (existing is null) return NotFound();

		// Валидация JSON формата правил ревью (если указано)
		if (!string.IsNullOrWhiteSpace(dto.RequiredReviewersRules))
		{
			try
			{
				System.Text.Json.JsonSerializer.Deserialize<List<object>>(dto.RequiredReviewersRules);
			}
			catch (System.Text.Json.JsonException)
			{
				return BadRequest(new { message = "RequiredReviewersRules must be a valid JSON array" });
			}
		}

		existing.Name = dto.Name;
		existing.Description = dto.Description;
		existing.OwnerId = dto.OwnerId;
		existing.DefaultLanguageId = dto.DefaultLanguageId;
		existing.Visibility = string.IsNullOrWhiteSpace(dto.Visibility) ? existing.Visibility : dto.Visibility;
		existing.RequiredReviewersRules = dto.RequiredReviewersRules;
		existing.UpdatedAt = DateTime.UtcNow;

		await _projects.UpdateAsync(existing, ct);
		await _uow.SaveChangesAsync(ct);
		return NoContent();
	}

	[HttpDelete("{id:guid}")]
	[Authorize(Policy = "ProjectAdmin")]
	public async Task<IActionResult> Delete(Guid id, CancellationToken ct)
	{
		await _projects.DeleteAsync(id, ct);
		await _uow.SaveChangesAsync(ct);
		return NoContent();
	}
}


