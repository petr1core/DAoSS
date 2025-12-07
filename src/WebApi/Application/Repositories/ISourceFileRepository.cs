namespace DAOSS.Application.Repositories;

using DAOSS.Domain.Entities;

public interface ISourceFileRepository : IRepository<SourceFile>
{
	System.Threading.Tasks.Task<IReadOnlyList<SourceFile>> GetByProjectIdAsync(Guid projectId, System.Threading.CancellationToken cancellationToken = default);
	System.Threading.Tasks.Task<SourceFile?> GetByProjectAndPathAsync(Guid projectId, string path, System.Threading.CancellationToken cancellationToken = default);
}


