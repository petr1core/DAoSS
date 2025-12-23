using System.Text.Json;
using DAOSS.Application.Interfaces;
using DAOSS.Application.Repositories;
using DAOSS.Domain.Entities;
using DAOSS.Infrastructure.Persistence;
using Microsoft.EntityFrameworkCore;

namespace DAOSS.Infrastructure.Services;

public class DiagramService : IDiagramService
{
	private readonly IDiagramRepository _diagramRepository;
	private readonly IDiagramVersionRepository _versionRepository;
	private readonly ISourceFileRepository _sourceFileRepository;
	private readonly ICodeRegionRepository _codeRegionRepository;
	private readonly IParserService _parserService;
	private readonly IUnitOfWork _unitOfWork;
	private readonly AppDbContext _dbContext;

	public DiagramService(
		IDiagramRepository diagramRepository,
		IDiagramVersionRepository versionRepository,
		ISourceFileRepository sourceFileRepository,
		ICodeRegionRepository codeRegionRepository,
		IParserService parserService,
		IUnitOfWork unitOfWork,
		AppDbContext dbContext)
	{
		_diagramRepository = diagramRepository;
		_versionRepository = versionRepository;
		_sourceFileRepository = sourceFileRepository;
		_codeRegionRepository = codeRegionRepository;
		_parserService = parserService;
		_unitOfWork = unitOfWork;
		_dbContext = dbContext;
	}

	public async System.Threading.Tasks.Task<Diagram> GenerateFromCodeAsync(
		Guid projectId,
		Guid sourceFileId,
		string code,
		string language,
		int? startLine = null,
		int? endLine = null,
		string? name = null,
		Guid authorId = default,
		string? message = null,
		System.Threading.CancellationToken cancellationToken = default)
	{
		// Проверяем существование исходного файла
		var sourceFile = await _sourceFileRepository.GetByIdAsync(sourceFileId, cancellationToken);
		if (sourceFile == null)
		{
			throw new InvalidOperationException($"Исходный файл с ID {sourceFileId} не найден");
		}

		if (sourceFile.ProjectId != projectId)
		{
			throw new InvalidOperationException("Исходный файл не принадлежит указанному проекту");
		}

		// Парсим код через парсер
		var parseResult = await _parserService.ParseToAstAsync(code, language, cancellationToken);
		
		if (!parseResult.Success || parseResult.Representation == null)
		{
			var errors = string.Join(", ", 
				parseResult.LexerErrors.Select(e => e.Message)
					.Concat(parseResult.ParserErrors.Select(e => e.Message)));
			throw new InvalidOperationException($"Ошибка парсинга кода: {errors}");
		}

		// Преобразуем результат парсера в JSON для диаграммы
		var diagramJson = JsonSerializer.Serialize(parseResult.Representation.Value, new JsonSerializerOptions
		{
			WriteIndented = true
		});

		// Создаем CodeRegion, если указаны строки
		CodeRegion? codeRegion = null;
		if (startLine.HasValue && endLine.HasValue)
		{
			codeRegion = new CodeRegion
			{
				Id = Guid.NewGuid(),
				SourceFileId = sourceFileId,
				StartLine = startLine.Value,
				EndLine = endLine.Value,
				TagName = name ?? "flowchart",
				RegionType = "flowchart",
				CreatedAt = DateTime.UtcNow
			};

			await _codeRegionRepository.AddAsync(codeRegion, cancellationToken);
		}

		// Создаем диаграмму
		var diagram = new Diagram
		{
			Id = Guid.NewGuid(),
			ProjectId = projectId,
			RegionId = codeRegion?.Id ?? Guid.Empty,
			Name = name ?? $"Diagram from {sourceFile.Path}",
			Status = "active",
			CreatedAt = DateTime.UtcNow
		};

		await _diagramRepository.AddAsync(diagram, cancellationToken);

		// Создаем первую версию диаграммы
		var version = new DiagramVersion
		{
			Id = Guid.NewGuid(),
			DiagramId = diagram.Id,
			VersionIndex = 1,
			JsonSnapshot = diagramJson,
			AuthorId = authorId,
			Message = message ?? "Initial version",
			CreatedAt = DateTime.UtcNow
		};

		await _versionRepository.AddAsync(version, cancellationToken);

		// Обновляем latest version
		diagram.LatestVersionId = version.Id;
		await _diagramRepository.UpdateAsync(diagram, cancellationToken);

		await _unitOfWork.SaveChangesAsync(cancellationToken);

		return diagram;
	}

	public async System.Threading.Tasks.Task<DiagramVersion> CreateVersionAsync(
		Guid diagramId,
		string jsonSnapshot,
		Guid authorId,
		string? message = null,
		System.Threading.CancellationToken cancellationToken = default)
	{
		var diagram = await _diagramRepository.GetByIdAsync(diagramId, cancellationToken);
		if (diagram == null)
		{
			throw new InvalidOperationException($"Диаграмма с ID {diagramId} не найдена");
		}

		// Получаем следующий индекс версии
		var nextVersionIndex = await _versionRepository.GetNextVersionIndexAsync(diagramId, cancellationToken);

		// Создаем новую версию
		var version = new DiagramVersion
		{
			Id = Guid.NewGuid(),
			DiagramId = diagramId,
			VersionIndex = nextVersionIndex,
			JsonSnapshot = jsonSnapshot,
			AuthorId = authorId,
			Message = message ?? $"Version {nextVersionIndex}",
			CreatedAt = DateTime.UtcNow
		};

		await _versionRepository.AddAsync(version, cancellationToken);

		// Обновляем latest version
		diagram.LatestVersionId = version.Id;
		diagram.UpdatedAt = DateTime.UtcNow;
		await _diagramRepository.UpdateAsync(diagram, cancellationToken);

		await _unitOfWork.SaveChangesAsync(cancellationToken);

		return version;
	}

	public async System.Threading.Tasks.Task<IReadOnlyList<Diagram>> GetByProjectIdAsync(
		Guid projectId,
		System.Threading.CancellationToken cancellationToken = default)
	{
		return await _diagramRepository.GetByProjectIdAsync(projectId, cancellationToken);
	}

	public async System.Threading.Tasks.Task<Diagram?> GetByIdAsync(
		Guid diagramId,
		System.Threading.CancellationToken cancellationToken = default)
	{
		return await _diagramRepository.GetByIdAsync(diagramId, cancellationToken);
	}

	public async System.Threading.Tasks.Task<IReadOnlyList<DiagramVersion>> GetVersionsAsync(
		Guid diagramId,
		System.Threading.CancellationToken cancellationToken = default)
	{
		return await _versionRepository.GetByDiagramIdAsync(diagramId, cancellationToken);
	}

	public async System.Threading.Tasks.Task UpdateDiagramAsync(
		Diagram diagram,
		System.Threading.CancellationToken cancellationToken = default)
	{
		diagram.UpdatedAt = DateTime.UtcNow;
		await _diagramRepository.UpdateAsync(diagram, cancellationToken);
		await _unitOfWork.SaveChangesAsync(cancellationToken);
	}

	public async System.Threading.Tasks.Task DeleteDiagramAsync(
		Guid diagramId,
		System.Threading.CancellationToken cancellationToken = default)
	{
		var diagram = await _diagramRepository.GetByIdAsync(diagramId, cancellationToken);
		if (diagram == null)
		{
			throw new InvalidOperationException($"Диаграмма с ID {diagramId} не найдена");
		}

		await _diagramRepository.DeleteAsync(diagramId, cancellationToken);
		await _unitOfWork.SaveChangesAsync(cancellationToken);
	}
}

