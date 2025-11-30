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
			var project = await _projects.GetByIdAsync(projectId);
			if (project == null)
			{
				return;
			}

			// Проверяем видимость проекта
			var isPublic = project.Visibility?.ToLowerInvariant() == "public";
			var isPrivate = !isPublic;

			// Сначала пробуем членство
			var role = await _members.GetUserRoleAsync(userId, projectId);
			// Если нет записи, проверяем владельца проекта
			if (role == null)
			{
				if (project.OwnerId == userId)
				{
					role = "owner";
				}
			}

			// Для публичных проектов: любой авторизованный пользователь имеет доступ на чтение
			// Для приватных проектов: только участники имеют доступ
			if (role == null)
			{
				// Пользователь не является участником
				if (isPrivate)
				{
					// Приватный проект - доступ запрещен
					return;
				}
				// Публичный проект - разрешаем доступ на чтение (reviewer уровень)
				// Но только если requirement включает reviewer или ниже
				if (requirement.AllowedRoles.Any(ar => ar.ToLowerInvariant() == "reviewer" || ar.ToLowerInvariant() == "public"))
				{
					context.Succeed(requirement);
				}
				return;
			}

			// owner имеет все; admin имеет все кроме удаления проекта; reviewer имеет read/write
			var roleLower = role.ToLowerInvariant();
			bool ok = requirement.AllowedRoles.Any(ar =>
			{
				var r = ar.ToLowerInvariant();
				if (r == "owner") return roleLower == "owner";
				if (r == "admin") return roleLower == "owner" || roleLower == "admin";
				if (r == "reviewer") return roleLower == "owner" || roleLower == "admin" || roleLower == "reviewer";
				if (r == "public") return true; // Любой участник имеет доступ уровня public
				return false;
			});

			if (ok)
			{
				context.Succeed(requirement);
			}
		}
	}
}


