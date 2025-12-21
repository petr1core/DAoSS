namespace DAOSS.Application.Repositories;

using DAOSS.Domain.Entities;

public interface ICodeRegionRepository : IRepository<CodeRegion>
{
	System.Threading.Tasks.Task<IReadOnlyList<CodeRegion>> GetBySourceFileIdAsync(Guid sourceFileId, System.Threading.CancellationToken cancellationToken = default);
	System.Threading.Tasks.Task<CodeRegion?> GetByDiagramIdAsync(Guid diagramId, System.Threading.CancellationToken cancellationToken = default);
}

