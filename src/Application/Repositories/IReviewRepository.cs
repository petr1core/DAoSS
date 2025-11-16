namespace DAOSS.Application.Repositories;

using DAOSS.Domain.Entities;

public interface IReviewRepository : IRepository<Review>
{
	System.Threading.Tasks.Task<IReadOnlyList<Review>> GetByProjectIdAsync(Guid projectId, System.Threading.CancellationToken cancellationToken = default);
}


