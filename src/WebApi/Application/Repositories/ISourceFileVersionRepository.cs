namespace DAOSS.Application.Repositories;

using DAOSS.Domain.Entities;

public interface ISourceFileVersionRepository : IRepository<SourceFileVersion>
{
	System.Threading.Tasks.Task<IReadOnlyList<SourceFileVersion>> GetBySourceFileIdAsync(Guid sourceFileId, System.Threading.CancellationToken cancellationToken = default);
	System.Threading.Tasks.Task<SourceFileVersion?> GetLatestBySourceFileIdAsync(Guid sourceFileId, System.Threading.CancellationToken cancellationToken = default);
	System.Threading.Tasks.Task<int> GetNextVersionIndexAsync(Guid sourceFileId, System.Threading.CancellationToken cancellationToken = default);
}

