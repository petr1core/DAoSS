namespace DAOSS.Application.Repositories;

using DAOSS.Domain.Entities;

public interface IReviewRepository : IRepository<Review>
{
	System.Threading.Tasks.Task<IReadOnlyList<Review>> GetByProjectIdAsync(Guid projectId, System.Threading.CancellationToken cancellationToken = default);
	System.Threading.Tasks.Task<IReadOnlyList<Review>> GetByTargetAsync(string targetType, Guid targetId, System.Threading.CancellationToken cancellationToken = default);
}


