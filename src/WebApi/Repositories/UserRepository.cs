using DAOSS.Application.Repositories;
using DAOSS.Domain.Entities;
using DAOSS.Infrastructure.Persistence;
using Microsoft.EntityFrameworkCore;

namespace DAOSS.Infrastructure.Repositories;

public class UserRepository : EfRepository<User>, IUserRepository
{
	private readonly AppDbContext _dbContext;

	public UserRepository(AppDbContext dbContext) : base(dbContext)
	{
		_dbContext = dbContext;
	}

	public async System.Threading.Tasks.Task<User?> GetByEmailAsync(string email, System.Threading.CancellationToken cancellationToken = default)
	{
		return await _dbContext.Users
			.FirstOrDefaultAsync(u => u.Email == email, cancellationToken);
	}
}


