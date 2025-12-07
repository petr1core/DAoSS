namespace DAOSS.Application.Repositories;

using DAOSS.Domain.Entities;

public interface IDiagramRepository : IRepository<Diagram>
{
	System.Threading.Tasks.Task<IReadOnlyList<Diagram>> GetByProjectIdAsync(Guid projectId, System.Threading.CancellationToken cancellationToken = default);
}


