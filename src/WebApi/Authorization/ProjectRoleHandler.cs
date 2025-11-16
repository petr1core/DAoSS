using System.Security.Claims;
using DAOSS.Application.Repositories;
using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Routing;

namespace DAOSS.WebApi.Authorization;

public class ProjectRoleHandler : AuthorizationHandler<ProjectRoleRequirement>
{
	private readonly IProjectMemberRepository _members;
	private readonly IProjectRepository _projects;

	public ProjectRoleHandler(IProjectMemberRepository members, IProjectRepository projects)
	{
		_members = members;
		_projects = projects;
	}

	protected override async Task HandleRequirementAsync(AuthorizationHandlerContext context, ProjectRoleRequirement requirement)
	{
		var userIdStr = context.User.FindFirstValue(ClaimTypes.NameIdentifier);
		if (!Guid.TryParse(userIdStr, out var userId))
		{
			return;
		}

		// Try get HttpContext and route data
		HttpContext? httpContext = null;
		if (context.Resource is HttpContext directCtx)
		{
			httpContext = directCtx;
		}
		else if (context.Resource is Microsoft.AspNetCore.Mvc.Filters.AuthorizationFilterContext mvcCtx)
		{
			httpContext = mvcCtx.HttpContext;
		}
		if (httpContext is null)
		{
			return;
		}

		var routeValues = httpContext.GetRouteData()?.Values;
		if (routeValues == null) return;

		// Expect "projectId" or "id" (for Project routes)
		Guid projectId;
		if (routeValues.TryGetValue("projectId", out var v1) && Guid.TryParse(v1?.ToString(), out projectId)
			|| (routeValues.TryGetValue("id", out var v2) && Guid.TryParse(v2?.ToString(), out projectId)))
		{
			// сначала пробуем членство
			var role = await _members.GetUserRoleAsync(userId, projectId);
			// если нет записи, считаем владельца проекта owner
			if (role == null)
			{
				var project = await _projects.GetByIdAsync(projectId);
				if (project != null && project.OwnerId == userId)
				{
					role = "owner";
				}
			}
			if (role == null) return;

			// owner имеет все; reviewer имеет read/write; admin == owner
			var roleLower = role.ToLowerInvariant();
			bool ok = requirement.AllowedRoles.Any(ar =>
			{
				var r = ar.ToLowerInvariant();
				if (r == "owner") return roleLower == "owner";
				if (r == "reviewer") return roleLower == "owner" || roleLower == "reviewer";
				return false;
			});

			if (ok)
			{
				context.Succeed(requirement);
			}
		}
	}
}


