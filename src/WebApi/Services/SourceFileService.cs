using System.Text.RegularExpressions;
using DAOSS.Application.Interfaces;
using DAOSS.Application.Repositories;
using DAOSS.Domain.Entities;
using DAOSS.Infrastructure.Persistence;
using Microsoft.EntityFrameworkCore;

namespace DAOSS.Infrastructure.Services;

public class SourceFileService : ISourceFileService
{
	private readonly ISourceFileRepository _sourceFileRepository;
	private readonly ISourceFileVersionRepository _versionRepository;
	private readonly ILanguageRepository _languageRepository;
	private readonly IUnitOfWork _unitOfWork;
	private readonly AppDbContext _dbContext;

	public SourceFileService(
		ISourceFileRepository sourceFileRepository,
		ISourceFileVersionRepository versionRepository,
		ILanguageRepository languageRepository,
		IUnitOfWork unitOfWork,
		AppDbContext dbContext)
	{
		_sourceFileRepository = sourceFileRepository;
		_versionRepository = versionRepository;
		_languageRepository = languageRepository;
		_unitOfWork = unitOfWork;
		_dbContext = dbContext;
	}

	public async System.Threading.Tasks.Task<SourceFile> CreateSourceFileAsync(
		Guid projectId,
		string path,
		string content,
		Guid authorId,
		string? message = null,
		System.Threading.CancellationToken cancellationToken = default)
	{
		// Проверяем, не существует ли уже файл с таким путем в проекте
		var existing = await _sourceFileRepository.GetByProjectAndPathAsync(projectId, path, cancellationToken);
		if (existing != null)
		{
			throw new InvalidOperationException($"Файл с путем '{path}' уже существует в проекте");
		}

		// Определяем язык по расширению файла
		var language = await DetermineLanguageByPathAsync(path, cancellationToken);
		if (language == null)
		{
			throw new InvalidOperationException($"Не удалось определить язык для файла '{path}'. Поддерживаемые расширения: .pas, .p, .c, .cpp, .h, .hpp");
		}

		// Создаем первую версию (сначала создаем версию, чтобы получить её ID)
		var versionId = Guid.NewGuid();
		var version = new SourceFileVersion
		{
			Id = versionId,
			VersionIndex = 1,
			Content = content,
			AuthorId = authorId,
			Message = message ?? "Initial version",
			CreatedAt = DateTime.UtcNow
		};

		// Создаем исходный файл с уже установленным LatestVersionId
		var sourceFile = new SourceFile
		{
			Id = Guid.NewGuid(),
			ProjectId = projectId,
			Path = path,
			LanguageId = language.Id,
			LatestVersionId = versionId,
			CreatedAt = DateTime.UtcNow
		};

		// Устанавливаем SourceFileId для версии
		version.SourceFileId = sourceFile.Id;

		await _sourceFileRepository.AddAsync(sourceFile, cancellationToken);
		await _versionRepository.AddAsync(version, cancellationToken);

		await _unitOfWork.SaveChangesAsync(cancellationToken);

		return sourceFile;
	}

	public async System.Threading.Tasks.Task<SourceFileVersion> CreateVersionAsync(
		Guid sourceFileId,
		string content,
		Guid authorId,
		string? message = null,
		System.Threading.CancellationToken cancellationToken = default)
	{
		var sourceFile = await _sourceFileRepository.GetByIdAsync(sourceFileId, cancellationToken);
		if (sourceFile == null)
		{
			throw new InvalidOperationException($"Исходный файл с ID {sourceFileId} не найден");
		}

		// Получаем следующий индекс версии
		var nextVersionIndex = await _versionRepository.GetNextVersionIndexAsync(sourceFileId, cancellationToken);

		// Создаем новую версию
		var version = new SourceFileVersion
		{
			Id = Guid.NewGuid(),
			SourceFileId = sourceFileId,
			VersionIndex = nextVersionIndex,
			Content = content,
			AuthorId = authorId,
			Message = message ?? $"Version {nextVersionIndex}",
			CreatedAt = DateTime.UtcNow
		};

		await _versionRepository.AddAsync(version, cancellationToken);

		// Обновляем latest version
		sourceFile.LatestVersionId = version.Id;
		sourceFile.UpdatedAt = DateTime.UtcNow;
		await _sourceFileRepository.UpdateAsync(sourceFile, cancellationToken);

		await _unitOfWork.SaveChangesAsync(cancellationToken);

		return version;
	}

	public async System.Threading.Tasks.Task<IReadOnlyList<SourceFile>> GetByProjectIdAsync(
		Guid projectId,
		System.Threading.CancellationToken cancellationToken = default)
	{
		return await _sourceFileRepository.GetByProjectIdAsync(projectId, cancellationToken);
	}

	public async System.Threading.Tasks.Task<SourceFile?> GetByIdAsync(
		Guid sourceFileId,
		System.Threading.CancellationToken cancellationToken = default)
	{
		return await _sourceFileRepository.GetByIdAsync(sourceFileId, cancellationToken);
	}

	public async System.Threading.Tasks.Task<IReadOnlyList<SourceFileVersion>> GetVersionsAsync(
		Guid sourceFileId,
		System.Threading.CancellationToken cancellationToken = default)
	{
		return await _versionRepository.GetBySourceFileIdAsync(sourceFileId, cancellationToken);
	}

	public async System.Threading.Tasks.Task<SourceFileVersion?> GetVersionAsync(
		Guid versionId,
		System.Threading.CancellationToken cancellationToken = default)
	{
		return await _versionRepository.GetByIdAsync(versionId, cancellationToken);
	}

	public async System.Threading.Tasks.Task DeleteSourceFileAsync(
		Guid sourceFileId,
		System.Threading.CancellationToken cancellationToken = default)
	{
		var sourceFile = await _sourceFileRepository.GetByIdAsync(sourceFileId, cancellationToken);
		if (sourceFile == null)
		{
			throw new InvalidOperationException($"Исходный файл с ID {sourceFileId} не найден");
		}

		await _sourceFileRepository.DeleteAsync(sourceFileId, cancellationToken);
		await _unitOfWork.SaveChangesAsync(cancellationToken);
	}

	private async System.Threading.Tasks.Task<Language?> DetermineLanguageByPathAsync(string path, System.Threading.CancellationToken cancellationToken)
	{
		var extension = Path.GetExtension(path).ToLowerInvariant();

		// Маппинг расширений на коды языков
		var extensionToLanguageCode = new Dictionary<string, string>
		{
			{ ".pas", "pascal" },
			{ ".p", "pascal" },
			{ ".c", "c" },
			{ ".cpp", "cpp" },
			{ ".cc", "cpp" },
			{ ".cxx", "cpp" },
			{ ".h", "c" },
			{ ".hpp", "cpp" },
			{ ".hxx", "cpp" }
		};

		if (!extensionToLanguageCode.TryGetValue(extension, out var languageCode))
		{
			return null;
		}

		return await _languageRepository.GetByCodeAsync(languageCode, cancellationToken);
	}
}

