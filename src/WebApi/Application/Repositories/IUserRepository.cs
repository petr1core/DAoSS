namespace DAOSS.Application.Repositories;

using DAOSS.Domain.Entities;

public interface IUserRepository : IRepository<User>
{
	System.Threading.Tasks.Task<User?> GetByEmailAsync(string email, System.Threading.CancellationToken cancellationToken = default);
}


