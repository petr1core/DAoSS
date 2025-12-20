namespace DAOSS.Application.Interfaces;

using DAOSS.Domain.Entities;

public interface ISourceFileService
{
	System.Threading.Tasks.Task<SourceFile> CreateSourceFileAsync(
		Guid projectId,
		string path,
		string content,
		Guid authorId,
		string? message = null,
		System.Threading.CancellationToken cancellationToken = default);

	System.Threading.Tasks.Task<SourceFileVersion> CreateVersionAsync(
		Guid sourceFileId,
		string content,
		Guid authorId,
		string? message = null,
		System.Threading.CancellationToken cancellationToken = default);

	System.Threading.Tasks.Task<IReadOnlyList<SourceFile>> GetByProjectIdAsync(
		Guid projectId,
		System.Threading.CancellationToken cancellationToken = default);

	System.Threading.Tasks.Task<SourceFile?> GetByIdAsync(
		Guid sourceFileId,
		System.Threading.CancellationToken cancellationToken = default);

	System.Threading.Tasks.Task<IReadOnlyList<SourceFileVersion>> GetVersionsAsync(
		Guid sourceFileId,
		System.Threading.CancellationToken cancellationToken = default);

	System.Threading.Tasks.Task<SourceFileVersion?> GetVersionAsync(
		Guid versionId,
		System.Threading.CancellationToken cancellationToken = default);

	System.Threading.Tasks.Task DeleteSourceFileAsync(
		Guid sourceFileId,
		System.Threading.CancellationToken cancellationToken = default);
}

