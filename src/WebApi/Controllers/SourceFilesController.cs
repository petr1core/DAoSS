using System.Security.Claims;
using DAOSS.Application.DTOs;
using DAOSS.Application.Interfaces;
using DAOSS.Application.Repositories;
using DAOSS.Domain.Entities;
using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc;

namespace DAOSS.WebApi.Controllers;

[ApiController]
[Route("api/projects/{projectId:guid}/source-files")]
public class SourceFilesController : ControllerBase
{
	private readonly ISourceFileService _sourceFileService;
	private readonly IProjectRepository _projectRepository;
	private readonly IUserRepository _userRepository;

	public SourceFilesController(
		ISourceFileService sourceFileService,
		IProjectRepository projectRepository,
		IUserRepository userRepository)
	{
		_sourceFileService = sourceFileService;
		_projectRepository = projectRepository;
		_userRepository = userRepository;
	}

	[HttpPost]
	[Authorize(Policy = "ProjectWrite")]
	public async Task<ActionResult<SourceFileResponseDto>> Create(
		Guid projectId,
		[FromBody] CreateSourceFileDto dto,
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

		// Проверка существования проекта
		var project = await _projectRepository.GetByIdAsync(projectId, ct);
		if (project == null)
		{
			return NotFound(new { message = "Project not found" });
		}

		try
		{
			var sourceFile = await _sourceFileService.CreateSourceFileAsync(
				projectId,
				dto.Path,
				dto.Content,
				currentUserId,
				dto.Message,
				ct);

			var response = new SourceFileResponseDto
			{
				Id = sourceFile.Id,
				ProjectId = sourceFile.ProjectId,
				Path = sourceFile.Path,
				LanguageId = sourceFile.LanguageId,
				LatestVersionId = sourceFile.LatestVersionId,
				CreatedAt = sourceFile.CreatedAt
			};

			return CreatedAtAction(nameof(Get), new { projectId, fileId = sourceFile.Id }, response);
		}
		catch (InvalidOperationException ex)
		{
			return BadRequest(new { message = ex.Message });
		}
	}

	[HttpGet]
	[Authorize(Policy = "ProjectRead")]
	public async Task<ActionResult<IReadOnlyList<SourceFileResponseDto>>> GetAll(Guid projectId, CancellationToken ct)
	{
		var project = await _projectRepository.GetByIdAsync(projectId, ct);
		if (project == null)
		{
			return NotFound(new { message = "Project not found" });
		}

		var sourceFiles = await _sourceFileService.GetByProjectIdAsync(projectId, ct);
		var result = sourceFiles.Select(sf => new SourceFileResponseDto
		{
			Id = sf.Id,
			ProjectId = sf.ProjectId,
			Path = sf.Path,
			LanguageId = sf.LanguageId,
			LatestVersionId = sf.LatestVersionId,
			CreatedAt = sf.CreatedAt
		}).ToList();

		return Ok(result);
	}

	[HttpGet("{fileId:guid}")]
	[Authorize(Policy = "ProjectRead")]
	public async Task<ActionResult<SourceFileResponseDto>> Get(Guid projectId, Guid fileId, CancellationToken ct)
	{
		var project = await _projectRepository.GetByIdAsync(projectId, ct);
		if (project == null)
		{
			return NotFound(new { message = "Project not found" });
		}

		var sourceFile = await _sourceFileService.GetByIdAsync(fileId, ct);
		if (sourceFile == null)
		{
			return NotFound(new { message = "Source file not found" });
		}

		if (sourceFile.ProjectId != projectId)
		{
			return BadRequest(new { message = "Source file does not belong to this project" });
		}

		var response = new SourceFileResponseDto
		{
			Id = sourceFile.Id,
			ProjectId = sourceFile.ProjectId,
			Path = sourceFile.Path,
			LanguageId = sourceFile.LanguageId,
			LatestVersionId = sourceFile.LatestVersionId,
			CreatedAt = sourceFile.CreatedAt
		};

		return Ok(response);
	}

	[HttpGet("{fileId:guid}/versions")]
	[Authorize(Policy = "ProjectRead")]
	public async Task<ActionResult<IReadOnlyList<SourceFileVersionResponseDto>>> GetVersions(
		Guid projectId,
		Guid fileId,
		CancellationToken ct)
	{
		var project = await _projectRepository.GetByIdAsync(projectId, ct);
		if (project == null)
		{
			return NotFound(new { message = "Project not found" });
		}

		var sourceFile = await _sourceFileService.GetByIdAsync(fileId, ct);
		if (sourceFile == null || sourceFile.ProjectId != projectId)
		{
			return NotFound(new { message = "Source file not found" });
		}

		var versions = await _sourceFileService.GetVersionsAsync(fileId, ct);
		var result = new List<SourceFileVersionResponseDto>();

		foreach (var version in versions)
		{
			var user = await _userRepository.GetByIdAsync(version.AuthorId, ct);
			result.Add(new SourceFileVersionResponseDto
			{
				Id = version.Id,
				SourceFileId = version.SourceFileId,
				VersionIndex = version.VersionIndex,
				Content = version.Content,
				AuthorId = version.AuthorId,
				AuthorName = user?.Name ?? string.Empty,
				Message = version.Message,
				CreatedAt = version.CreatedAt,
				IsVerified = version.IsVerified
			});
		}

		return Ok(result);
	}

	[HttpGet("{fileId:guid}/versions/{versionId:guid}")]
	[Authorize(Policy = "ProjectRead")]
	public async Task<ActionResult<SourceFileVersionResponseDto>> GetVersion(
		Guid projectId,
		Guid fileId,
		Guid versionId,
		CancellationToken ct)
	{
		var project = await _projectRepository.GetByIdAsync(projectId, ct);
		if (project == null)
		{
			return NotFound(new { message = "Project not found" });
		}

		var sourceFile = await _sourceFileService.GetByIdAsync(fileId, ct);
		if (sourceFile == null || sourceFile.ProjectId != projectId)
		{
			return NotFound(new { message = "Source file not found" });
		}

		var version = await _sourceFileService.GetVersionAsync(versionId, ct);
		if (version == null || version.SourceFileId != fileId)
		{
			return NotFound(new { message = "Version not found" });
		}

		var user = await _userRepository.GetByIdAsync(version.AuthorId, ct);
		var response = new SourceFileVersionResponseDto
		{
			Id = version.Id,
			SourceFileId = version.SourceFileId,
			VersionIndex = version.VersionIndex,
			Content = version.Content,
			AuthorId = version.AuthorId,
			AuthorName = user?.Name ?? string.Empty,
			Message = version.Message,
			CreatedAt = version.CreatedAt,
			IsVerified = version.IsVerified
		};

		return Ok(response);
	}

	[HttpPost("{fileId:guid}/versions")]
	[Authorize(Policy = "ProjectWrite")]
	public async Task<ActionResult<SourceFileVersionResponseDto>> CreateVersion(
		Guid projectId,
		Guid fileId,
		[FromBody] CreateSourceFileVersionDto dto,
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

		var sourceFile = await _sourceFileService.GetByIdAsync(fileId, ct);
		if (sourceFile == null || sourceFile.ProjectId != projectId)
		{
			return NotFound(new { message = "Source file not found" });
		}

		try
		{
			var version = await _sourceFileService.CreateVersionAsync(
				fileId,
				dto.Content,
				currentUserId,
				dto.Message,
				ct);

			var user = await _userRepository.GetByIdAsync(version.AuthorId, ct);
			var response = new SourceFileVersionResponseDto
			{
				Id = version.Id,
				SourceFileId = version.SourceFileId,
				VersionIndex = version.VersionIndex,
				Content = version.Content,
				AuthorId = version.AuthorId,
				AuthorName = user?.Name ?? string.Empty,
				Message = version.Message,
				CreatedAt = version.CreatedAt,
				IsVerified = version.IsVerified
			};

			return CreatedAtAction(
				nameof(GetVersion),
				new { projectId, fileId, versionId = version.Id },
				response);
		}
		catch (InvalidOperationException ex)
		{
			return BadRequest(new { message = ex.Message });
		}
	}

	[HttpDelete("{fileId:guid}")]
	[Authorize(Policy = "ProjectWrite")]
	public async Task<ActionResult> Delete(Guid projectId, Guid fileId, CancellationToken ct)
	{
		var project = await _projectRepository.GetByIdAsync(projectId, ct);
		if (project == null)
		{
			return NotFound(new { message = "Project not found" });
		}

		var sourceFile = await _sourceFileService.GetByIdAsync(fileId, ct);
		if (sourceFile == null || sourceFile.ProjectId != projectId)
		{
			return NotFound(new { message = "Source file not found" });
		}

		try
		{
			await _sourceFileService.DeleteSourceFileAsync(fileId, ct);
			return NoContent();
		}
		catch (InvalidOperationException ex)
		{
			return BadRequest(new { message = ex.Message });
		}
	}
}

