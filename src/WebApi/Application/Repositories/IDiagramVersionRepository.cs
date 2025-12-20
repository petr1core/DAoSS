namespace DAOSS.Application.Repositories;

using DAOSS.Domain.Entities;

public interface IDiagramVersionRepository : IRepository<DiagramVersion>
{
	System.Threading.Tasks.Task<IReadOnlyList<DiagramVersion>> GetByDiagramIdAsync(Guid diagramId, System.Threading.CancellationToken cancellationToken = default);
	System.Threading.Tasks.Task<DiagramVersion?> GetLatestByDiagramIdAsync(Guid diagramId, System.Threading.CancellationToken cancellationToken = default);
	System.Threading.Tasks.Task<int> GetNextVersionIndexAsync(Guid diagramId, System.Threading.CancellationToken cancellationToken = default);
}

