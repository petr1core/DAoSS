namespace DAOSS.Application.Interfaces;

using DAOSS.Domain.Entities;

public interface IDiagramService
{
	System.Threading.Tasks.Task<Diagram> GenerateFromCodeAsync(
		Guid projectId,
		Guid sourceFileId,
		string code,
		string language,
		int? startLine = null,
		int? endLine = null,
		string? name = null,
		Guid authorId = default,
		string? message = null,
		System.Threading.CancellationToken cancellationToken = default);

	System.Threading.Tasks.Task<DiagramVersion> CreateVersionAsync(
		Guid diagramId,
		string jsonSnapshot,
		Guid authorId,
		string? message = null,
		System.Threading.CancellationToken cancellationToken = default);

	System.Threading.Tasks.Task<IReadOnlyList<Diagram>> GetByProjectIdAsync(
		Guid projectId,
		System.Threading.CancellationToken cancellationToken = default);

	System.Threading.Tasks.Task<Diagram?> GetByIdAsync(
		Guid diagramId,
		System.Threading.CancellationToken cancellationToken = default);

	System.Threading.Tasks.Task<IReadOnlyList<DiagramVersion>> GetVersionsAsync(
		Guid diagramId,
		System.Threading.CancellationToken cancellationToken = default);

	System.Threading.Tasks.Task UpdateDiagramAsync(
		Diagram diagram,
		System.Threading.CancellationToken cancellationToken = default);

	System.Threading.Tasks.Task DeleteDiagramAsync(
		Guid diagramId,
		System.Threading.CancellationToken cancellationToken = default);
}


