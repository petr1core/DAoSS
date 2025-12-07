using DAOSS.Application.Repositories;
using DAOSS.Domain.Entities;
using DAOSS.Infrastructure.Persistence;
using Microsoft.EntityFrameworkCore;

namespace DAOSS.Infrastructure.Repositories;

public class SourceFileRepository : EfRepository<SourceFile>, ISourceFileRepository
{
	private readonly AppDbContext _dbContext;

	public SourceFileRepository(AppDbContext dbContext) : base(dbContext)
	{
		_dbContext = dbContext;
	}

	public async System.Threading.Tasks.Task<IReadOnlyList<SourceFile>> GetByProjectIdAsync(Guid projectId, System.Threading.CancellationToken cancellationToken = default)
	{
		return await _dbContext.SourceFiles
			.Where(sf => sf.ProjectId == projectId)
			.ToListAsync(cancellationToken);
	}

	public async System.Threading.Tasks.Task<SourceFile?> GetByProjectAndPathAsync(Guid projectId, string path, System.Threading.CancellationToken cancellationToken = default)
	{
		return await _dbContext.SourceFiles
			.FirstOrDefaultAsync(sf => sf.ProjectId == projectId && sf.Path == path, cancellationToken);
	}
}


