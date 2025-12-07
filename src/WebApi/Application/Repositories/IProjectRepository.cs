namespace DAOSS.Application.Repositories;

using DAOSS.Domain.Entities;

public interface IProjectRepository : IRepository<Project>
{
	System.Threading.Tasks.Task<IReadOnlyList<Project>> GetByUserIdAsync(Guid userId, System.Threading.CancellationToken cancellationToken = default);
}


