using Microsoft.AspNetCore.Authorization;

namespace DAOSS.WebApi.Authorization;

public class ProjectRoleRequirement : IAuthorizationRequirement
{
	public ProjectRoleRequirement(params string[] allowedRoles)
	{
		AllowedRoles = allowedRoles ?? Array.Empty<string>();
	}

	public IReadOnlyCollection<string> AllowedRoles { get; }
}


