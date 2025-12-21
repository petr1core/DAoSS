namespace DAOSS.Application.Repositories;

using DAOSS.Domain.Entities;

public interface IProjectMemberRepository : IRepository<ProjectMember>
{
	System.Threading.Tasks.Task<string?> GetUserRoleAsync(Guid userId, Guid projectId, System.Threading.CancellationToken cancellationToken = default);
	System.Threading.Tasks.Task<bool> IsMemberAsync(Guid userId, Guid projectId, System.Threading.CancellationToken cancellationToken = default);
	System.Threading.Tasks.Task<ProjectMember?> GetByProjectAndUserAsync(Guid projectId, Guid userId, System.Threading.CancellationToken cancellationToken = default);
	System.Threading.Tasks.Task<IReadOnlyList<ProjectMember>> GetByProjectIdAsync(Guid projectId, System.Threading.CancellationToken cancellationToken = default);
}


