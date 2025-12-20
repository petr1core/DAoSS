using DAOSS.Application.Repositories;
using DAOSS.Domain.Entities;
using DAOSS.Infrastructure.Persistence;
using Microsoft.EntityFrameworkCore;

namespace DAOSS.Infrastructure.Repositories;

public class SourceFileVersionRepository : EfRepository<SourceFileVersion>, ISourceFileVersionRepository
{
	private readonly AppDbContext _dbContext;

	public SourceFileVersionRepository(AppDbContext dbContext) : base(dbContext)
	{
		_dbContext = dbContext;
	}

	public async System.Threading.Tasks.Task<IReadOnlyList<SourceFileVersion>> GetBySourceFileIdAsync(Guid sourceFileId, System.Threading.CancellationToken cancellationToken = default)
	{
		return await _dbContext.SourceFileVersions
			.Where(v => v.SourceFileId == sourceFileId)
			.OrderBy(v => v.VersionIndex)
			.ToListAsync(cancellationToken);
	}

	public async System.Threading.Tasks.Task<SourceFileVersion?> GetLatestBySourceFileIdAsync(Guid sourceFileId, System.Threading.CancellationToken cancellationToken = default)
	{
		return await _dbContext.SourceFileVersions
			.Where(v => v.SourceFileId == sourceFileId)
			.OrderByDescending(v => v.VersionIndex)
			.FirstOrDefaultAsync(cancellationToken);
	}

	public async System.Threading.Tasks.Task<int> GetNextVersionIndexAsync(Guid sourceFileId, System.Threading.CancellationToken cancellationToken = default)
	{
		var maxVersion = await _dbContext.SourceFileVersions
			.Where(v => v.SourceFileId == sourceFileId)
			.Select(v => (int?)v.VersionIndex)
			.MaxAsync(cancellationToken);

		return (maxVersion ?? 0) + 1;
	}
}

