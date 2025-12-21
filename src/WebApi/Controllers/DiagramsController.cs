using System.Security.Claims;
using DAOSS.Application.DTOs;
using DAOSS.Application.Interfaces;
using DAOSS.Application.Repositories;
using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc;

namespace DAOSS.WebApi.Controllers;

[ApiController]
[Route("api/projects/{projectId:guid}/diagrams")]
public class DiagramsController : ControllerBase
{
	private readonly IDiagramService _diagramService;
	private readonly IProjectRepository _projectRepository;
	private readonly IUserRepository _userRepository;

	public DiagramsController(
		IDiagramService diagramService,
		IProjectRepository projectRepository,
		IUserRepository userRepository)
	{
		_diagramService = diagramService;
		_projectRepository = projectRepository;
		_userRepository = userRepository;
	}

	[HttpPost("generate")]
	[Authorize(Policy = "ProjectWrite")]
	public async Task<ActionResult<DiagramResponseDto>> Generate(
		Guid projectId,
		[FromBody] GenerateDiagramDto dto,
		CancellationToken ct)
	{
		if (!ModelState.IsValid)
		{
			return BadRequest(ModelState);
		}

		var currentUserIdStr = User.FindFirstValue(ClaimTypes.NameIdentifier);
		if (!Guid.TryParse(currentUserIdStr, out var currentUserId))
		{
			return Unauthorized(new { message = "Invalid user identity" });
		}

		var project = await _projectRepository.GetByIdAsync(projectId, ct);
		if (project == null)
		{
			return NotFound(new { message = "Project not found" });
		}

		try
		{
			var diagram = await _diagramService.GenerateFromCodeAsync(
				projectId,
				dto.SourceFileId,
				dto.Code,
				dto.Language,
				dto.StartLine,
				dto.EndLine,
				dto.Name,
				currentUserId,
				dto.Message,
				ct);

			var response = new DiagramResponseDto
			{
				Id = diagram.Id,
				ProjectId = diagram.ProjectId,
				RegionId = diagram.RegionId,
				Name = diagram.Name,
				LatestVersionId = diagram.LatestVersionId,
				Status = diagram.Status,
				CreatedAt = diagram.CreatedAt,
				UpdatedAt = diagram.UpdatedAt
			};

			return CreatedAtAction(nameof(Get), new { projectId, diagramId = diagram.Id }, response);
		}
		catch (InvalidOperationException ex)
		{
			return BadRequest(new { message = ex.Message });
		}
	}

	[HttpGet]
	[Authorize(Policy = "ProjectRead")]
	public async Task<ActionResult<IReadOnlyList<DiagramResponseDto>>> GetAll(Guid projectId, CancellationToken ct)
	{
		var project = await _projectRepository.GetByIdAsync(projectId, ct);
		if (project == null)
		{
			return NotFound(new { message = "Project not found" });
		}

		var diagrams = await _diagramService.GetByProjectIdAsync(projectId, ct);
		var result = diagrams.Select(d => new DiagramResponseDto
		{
			Id = d.Id,
			ProjectId = d.ProjectId,
			RegionId = d.RegionId,
			Name = d.Name,
			LatestVersionId = d.LatestVersionId,
			Status = d.Status,
			CreatedAt = d.CreatedAt,
			UpdatedAt = d.UpdatedAt
		}).ToList();

		return Ok(result);
	}

	[HttpGet("{diagramId:guid}")]
	[Authorize(Policy = "ProjectRead")]
	public async Task<ActionResult<DiagramResponseDto>> Get(Guid projectId, Guid diagramId, CancellationToken ct)
	{
		var project = await _projectRepository.GetByIdAsync(projectId, ct);
		if (project == null)
		{
			return NotFound(new { message = "Project not found" });
		}

		var diagram = await _diagramService.GetByIdAsync(diagramId, ct);
		if (diagram == null)
		{
			return NotFound(new { message = "Diagram not found" });
		}

		if (diagram.ProjectId != projectId)
		{
			return BadRequest(new { message = "Diagram does not belong to this project" });
		}

		var response = new DiagramResponseDto
		{
			Id = diagram.Id,
			ProjectId = diagram.ProjectId,
			RegionId = diagram.RegionId,
			Name = diagram.Name,
			LatestVersionId = diagram.LatestVersionId,
			Status = diagram.Status,
			CreatedAt = diagram.CreatedAt,
			UpdatedAt = diagram.UpdatedAt
		};

		return Ok(response);
	}

	[HttpGet("{diagramId:guid}/versions")]
	[Authorize(Policy = "ProjectRead")]
	public async Task<ActionResult<IReadOnlyList<DiagramVersionResponseDto>>> GetVersions(
		Guid projectId,
		Guid diagramId,
		CancellationToken ct)
	{
		var project = await _projectRepository.GetByIdAsync(projectId, ct);
		if (project == null)
		{
			return NotFound(new { message = "Project not found" });
		}

		var diagram = await _diagramService.GetByIdAsync(diagramId, ct);
		if (diagram == null || diagram.ProjectId != projectId)
		{
			return NotFound(new { message = "Diagram not found" });
		}

		var versions = await _diagramService.GetVersionsAsync(diagramId, ct);
		var result = new List<DiagramVersionResponseDto>();

		foreach (var version in versions)
		{
			var user = await _userRepository.GetByIdAsync(version.AuthorId, ct);
			result.Add(new DiagramVersionResponseDto
			{
				Id = version.Id,
				DiagramId = version.DiagramId,
				VersionIndex = version.VersionIndex,
				JsonSnapshot = version.JsonSnapshot,
				AuthorId = version.AuthorId,
				AuthorName = user?.Name ?? string.Empty,
				Message = version.Message,
				CreatedAt = version.CreatedAt
			});
		}

		return Ok(result);
	}

	[HttpPost("{diagramId:guid}/versions")]
	[Authorize(Policy = "ProjectWrite")]
	public async Task<ActionResult<DiagramVersionResponseDto>> CreateVersion(
		Guid projectId,
		Guid diagramId,
		[FromBody] CreateDiagramVersionDto dto,
		CancellationToken ct)
	{
		if (!ModelState.IsValid)
		{
			return BadRequest(ModelState);
		}

		var currentUserIdStr = User.FindFirstValue(ClaimTypes.NameIdentifier);
		if (!Guid.TryParse(currentUserIdStr, out var currentUserId))
		{
			return Unauthorized(new { message = "Invalid user identity" });
		}

		var project = await _projectRepository.GetByIdAsync(projectId, ct);
		if (project == null)
		{
			return NotFound(new { message = "Project not found" });
		}

		var diagram = await _diagramService.GetByIdAsync(diagramId, ct);
		if (diagram == null || diagram.ProjectId != projectId)
		{
			return NotFound(new { message = "Diagram not found" });
		}

		try
		{
			var version = await _diagramService.CreateVersionAsync(
				diagramId,
				dto.JsonSnapshot,
				currentUserId,
				dto.Message,
				ct);

			var user = await _userRepository.GetByIdAsync(version.AuthorId, ct);
			var response = new DiagramVersionResponseDto
			{
				Id = version.Id,
				DiagramId = version.DiagramId,
				VersionIndex = version.VersionIndex,
				JsonSnapshot = version.JsonSnapshot,
				AuthorId = version.AuthorId,
				AuthorName = user?.Name ?? string.Empty,
				Message = version.Message,
				CreatedAt = version.CreatedAt
			};

			return CreatedAtAction(
				nameof(GetVersions),
				new { projectId, diagramId },
				response);
		}
		catch (InvalidOperationException ex)
		{
			return BadRequest(new { message = ex.Message });
		}
	}

	[HttpPut("{diagramId:guid}")]
	[Authorize(Policy = "ProjectWrite")]
	public async Task<ActionResult<DiagramResponseDto>> Update(
		Guid projectId,
		Guid diagramId,
		[FromBody] UpdateDiagramDto dto,
		CancellationToken ct)
	{
		if (!ModelState.IsValid)
		{
			return BadRequest(ModelState);
		}

		var project = await _projectRepository.GetByIdAsync(projectId, ct);
		if (project == null)
		{
			return NotFound(new { message = "Project not found" });
		}

		var diagram = await _diagramService.GetByIdAsync(diagramId, ct);
		if (diagram == null || diagram.ProjectId != projectId)
		{
			return NotFound(new { message = "Diagram not found" });
		}

		diagram.Name = dto.Name ?? diagram.Name;
		diagram.Status = dto.Status ?? diagram.Status;

		await _diagramService.UpdateDiagramAsync(diagram, ct);

		var response = new DiagramResponseDto
		{
			Id = diagram.Id,
			ProjectId = diagram.ProjectId,
			RegionId = diagram.RegionId,
			Name = diagram.Name,
			LatestVersionId = diagram.LatestVersionId,
			Status = diagram.Status,
			CreatedAt = diagram.CreatedAt,
			UpdatedAt = diagram.UpdatedAt
		};

		return Ok(response);
	}

	[HttpDelete("{diagramId:guid}")]
	[Authorize(Policy = "ProjectWrite")]
	public async Task<ActionResult> Delete(Guid projectId, Guid diagramId, CancellationToken ct)
	{
		var project = await _projectRepository.GetByIdAsync(projectId, ct);
		if (project == null)
		{
			return NotFound(new { message = "Project not found" });
		}

		var diagram = await _diagramService.GetByIdAsync(diagramId, ct);
		if (diagram == null || diagram.ProjectId != projectId)
		{
			return NotFound(new { message = "Diagram not found" });
		}

		try
		{
			await _diagramService.DeleteDiagramAsync(diagramId, ct);
			return NoContent();
		}
		catch (InvalidOperationException ex)
		{
			return BadRequest(new { message = ex.Message });
		}
	}

	public sealed class UpdateDiagramDto
	{
		public string? Name { get; set; }
		public string? Status { get; set; }
	}
}

