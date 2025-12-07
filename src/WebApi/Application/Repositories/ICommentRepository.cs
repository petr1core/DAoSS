namespace DAOSS.Application.Repositories;

using DAOSS.Domain.Entities;

public interface ICommentRepository : IRepository<Comment>
{
	System.Threading.Tasks.Task<IReadOnlyList<Comment>> GetByReviewItemIdAsync(Guid reviewItemId, System.Threading.CancellationToken cancellationToken = default);
}


