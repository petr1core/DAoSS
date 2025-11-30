using DAOSS.Application.Interfaces;
using DAOSS.Application.Repositories;
using DAOSS.Domain.Entities;
using DAOSS.Infrastructure.Persistence;
using Microsoft.EntityFrameworkCore;
using System.Text.Json;

namespace DAOSS.Infrastructure.Services;

public class ReviewService : IReviewService
{
	private readonly IReviewRepository _reviewRepository;
	private readonly IReviewItemRepository _reviewItemRepository;
	private readonly ICommentRepository _commentRepository;
	private readonly IProjectRepository _projectRepository;
	private readonly IProjectMemberRepository _projectMemberRepository;
	private readonly IUnitOfWork _unitOfWork;
	private readonly AppDbContext _dbContext;

	public ReviewService(
		IReviewRepository reviewRepository,
		IReviewItemRepository reviewItemRepository,
		ICommentRepository commentRepository,
		IProjectRepository projectRepository,
		IProjectMemberRepository projectMemberRepository,
		IUnitOfWork unitOfWork,
		AppDbContext dbContext)
	{
		_reviewRepository = reviewRepository;
		_reviewItemRepository = reviewItemRepository;
		_commentRepository = commentRepository;
		_projectRepository = projectRepository;
		_projectMemberRepository = projectMemberRepository;
		_unitOfWork = unitOfWork;
		_dbContext = dbContext;
	}

	/// <summary>
	/// Определяет приоритет ревьюера на основе его роли в проекте.
	/// Owner: 100, Admin: 50, Reviewer: 25, Participant: 20, Public: 10
	/// </summary>
	private async Task<int> GetReviewerPriorityAsync(Guid userId, Guid projectId, CancellationToken cancellationToken = default)
	{
		var project = await _projectRepository.GetByIdAsync(projectId, cancellationToken);
		if (project == null)
		{
			return 10; // Public user
		}

		// Проверяем, является ли пользователь владельцем проекта
		if (project.OwnerId == userId)
		{
			return 100; // Owner
		}

		// Проверяем роль в ProjectMembers
		var role = await _projectMemberRepository.GetUserRoleAsync(userId, projectId, cancellationToken);
		if (role == null)
		{
			return 10; // Public user (не участник)
		}

		var roleLower = role.ToLowerInvariant();
		return roleLower switch
		{
			"owner" => 100,
			"admin" => 50,
			"reviewer" => 25,
			_ => 20 // Participant (участник без роли reviewer)
		};
	}

	/// <summary>
	/// Парсит правила ревью из JSON строки.
	/// </summary>
	private List<ReviewRule>? ParseReviewRules(string? rulesJson)
	{
		if (string.IsNullOrWhiteSpace(rulesJson))
		{
			return null;
		}

		try
		{
			var rules = JsonSerializer.Deserialize<List<ReviewRule>>(rulesJson, new JsonSerializerOptions
			{
				PropertyNameCaseInsensitive = true
			});
			return rules;
		}
		catch (JsonException)
		{
			return null;
		}
	}

	/// <summary>
	/// Определяет роль ревьюера на основе приоритета.
	/// </summary>
	private string GetReviewerRoleByPriority(int priority)
	{
		return priority switch
		{
			100 => "Owner",
			50 => "Admin",
			25 => "Reviewer",
			20 => "Reviewer", // Participant засчитывается как Reviewer
			_ => "Public"
		};
	}

	/// <summary>
	/// Проверяет выполнение правил ревью для указанного target.
	/// Возвращает true, если все правила выполнены.
	/// </summary>
	private async Task<bool> CheckReviewRulesComplianceAsync(Guid projectId, Guid targetId, string targetType, CancellationToken cancellationToken = default)
	{
		var project = await _projectRepository.GetByIdAsync(projectId, cancellationToken);
		if (project == null)
		{
			return false;
		}

		var rules = ParseReviewRules(project.RequiredReviewersRules);
		if (rules == null || rules.Count == 0)
		{
			// Если правил нет, считаем что требования выполнены
			return true;
		}

		// Получаем все ревью со статусом "approved" для этого target
		var reviews = await _reviewRepository.GetByTargetAsync(targetType, targetId, cancellationToken);
		var approvedReviews = reviews.Where(r => r.Status == "approved").ToList();

		if (approvedReviews.Count == 0)
		{
			return false;
		}

		// Группируем ревью по ролям на основе приоритета
		var reviewsByRole = new Dictionary<string, List<Review>>();
		foreach (var review in approvedReviews)
		{
			var role = GetReviewerRoleByPriority(review.ReviewerPriority);
			if (role != "Public")
			{
				if (!reviewsByRole.ContainsKey(role))
				{
					reviewsByRole[role] = new List<Review>();
				}
				reviewsByRole[role].Add(review);
			}
		}

		// Проверяем каждое правило
		foreach (var rule in rules)
		{
			var requiredCount = rule.Count ?? 1;
			var role = rule.Role;

			// Подсчитываем количество ревью для этой роли
			// Owner может засчитываться за любую роль
			// Admin может засчитываться за Admin и Reviewer
			// Reviewer может засчитываться только за Reviewer
			int count = 0;

			if (role == "Owner")
			{
				count = reviewsByRole.GetValueOrDefault("Owner", new List<Review>()).Count;
			}
			else if (role == "Admin")
			{
				// Admin ревью + Owner ревью (Owner может засчитываться за Admin)
				count = reviewsByRole.GetValueOrDefault("Admin", new List<Review>()).Count
					+ reviewsByRole.GetValueOrDefault("Owner", new List<Review>()).Count;
			}
			else if (role == "Reviewer")
			{
				// Reviewer ревью + Admin ревью + Owner ревью (все могут засчитываться за Reviewer)
				count = reviewsByRole.GetValueOrDefault("Reviewer", new List<Review>()).Count
					+ reviewsByRole.GetValueOrDefault("Admin", new List<Review>()).Count
					+ reviewsByRole.GetValueOrDefault("Owner", new List<Review>()).Count;
			}

			if (count < requiredCount)
			{
				return false;
			}
		}

		return true;
	}

	/// <summary>
	/// Обновляет статус IsVerified для версии файла/диаграммы на основе правил ревью.
	/// </summary>
	private async Task UpdateVerifiedStatusAsync(Guid projectId, Guid targetId, string targetType, CancellationToken cancellationToken = default)
	{
		var isCompliant = await CheckReviewRulesComplianceAsync(projectId, targetId, targetType, cancellationToken);

		if (targetType == "source_file_version")
		{
			var version = await _dbContext.Set<SourceFileVersion>().FindAsync(new object[] { targetId }, cancellationToken);
			if (version != null)
			{
				version.IsVerified = isCompliant;
				version.UpdatedAt = DateTime.UtcNow;
				await _dbContext.SaveChangesAsync(cancellationToken);
			}
		}
		// Для diagram_version пока не реализовано, так как в требованиях упоминается только SourceFileVersion
	}

	public async System.Threading.Tasks.Task<Review> CreateReviewAsync(Guid projectId, string targetType, Guid targetId, Guid createdBy, System.Threading.CancellationToken cancellationToken = default)
	{
		// Валидация проекта
		var project = await _projectRepository.GetByIdAsync(projectId, cancellationToken);
		if (project == null)
		{
			throw new InvalidOperationException("Project not found");
		}

		// Валидация targetType
		if (targetType != "diagram_version" && targetType != "source_file_version")
		{
			throw new ArgumentException("targetType must be 'diagram_version' or 'source_file_version'", nameof(targetType));
		}

		// Валидация существования target
		bool targetExists = false;
		if (targetType == "diagram_version")
		{
			targetExists = await _dbContext.Set<DiagramVersion>().AnyAsync(dv => dv.Id == targetId, cancellationToken);
		}
		else if (targetType == "source_file_version")
		{
			targetExists = await _dbContext.Set<SourceFileVersion>().AnyAsync(sfv => sfv.Id == targetId, cancellationToken);
		}

		if (!targetExists)
		{
			throw new InvalidOperationException($"Target {targetType} with id {targetId} not found");
		}

		// Определяем приоритет ревьюера
		var priority = await GetReviewerPriorityAsync(createdBy, projectId, cancellationToken);

		var review = new Review
		{
			Id = Guid.NewGuid(),
			ProjectId = projectId,
			TargetType = targetType,
			TargetId = targetId,
			Status = "changes_requested", // Новый дефолтный статус
			CreatedBy = createdBy,
			ReviewerPriority = priority,
			CreatedAt = DateTime.UtcNow
		};

		await _reviewRepository.AddAsync(review, cancellationToken);
		await _unitOfWork.SaveChangesAsync(cancellationToken);

		// Проверяем и обновляем verified статус после создания ревью
		await UpdateVerifiedStatusAsync(projectId, targetId, targetType, cancellationToken);

		return review;
	}

	public async System.Threading.Tasks.Task<Review?> GetReviewAsync(Guid reviewId, System.Threading.CancellationToken cancellationToken = default)
	{
		return await _reviewRepository.GetByIdAsync(reviewId, cancellationToken);
	}

	public async System.Threading.Tasks.Task<IReadOnlyList<Review>> GetReviewsByProjectAsync(Guid projectId, System.Threading.CancellationToken cancellationToken = default)
	{
		return await _reviewRepository.GetByProjectIdAsync(projectId, cancellationToken);
	}

	public async System.Threading.Tasks.Task<Review> UpdateReviewStatusAsync(Guid reviewId, string status, System.Threading.CancellationToken cancellationToken = default)
	{
		var review = await _reviewRepository.GetByIdAsync(reviewId, cancellationToken);
		if (review == null)
		{
			throw new InvalidOperationException("Review not found");
		}

		if (status != "approved" && status != "changes_requested")
		{
			throw new ArgumentException("status must be 'approved' or 'changes_requested'", nameof(status));
		}

		review.Status = status;
		review.UpdatedAt = DateTime.UtcNow;
		await _reviewRepository.UpdateAsync(review, cancellationToken);
		await _unitOfWork.SaveChangesAsync(cancellationToken);

		// Проверяем и обновляем verified статус после обновления статуса ревью
		await UpdateVerifiedStatusAsync(review.ProjectId, review.TargetId, review.TargetType, cancellationToken);

		return review;
	}

	public async System.Threading.Tasks.Task DeleteReviewAsync(Guid reviewId, System.Threading.CancellationToken cancellationToken = default)
	{
		var review = await _reviewRepository.GetByIdAsync(reviewId, cancellationToken);
		if (review == null)
		{
			throw new InvalidOperationException("Review not found");
		}

		// Проверка наличия связанных ReviewItem
		var items = await _reviewItemRepository.GetByReviewIdAsync(reviewId, cancellationToken);
		if (items.Count > 0)
		{
			throw new InvalidOperationException("Cannot delete review with existing review items");
		}

		await _reviewRepository.DeleteAsync(reviewId, cancellationToken);
		await _unitOfWork.SaveChangesAsync(cancellationToken);
	}

	public async System.Threading.Tasks.Task<ReviewItem> CreateReviewItemAsync(Guid reviewId, string kind, string anchorType, string anchorRef, string body, Guid createdBy, System.Threading.CancellationToken cancellationToken = default)
	{
		var review = await _reviewRepository.GetByIdAsync(reviewId, cancellationToken);
		if (review == null)
		{
			throw new InvalidOperationException("Review not found");
		}

		if (kind != "comment" && kind != "issue")
		{
			throw new ArgumentException("kind must be 'comment' or 'issue'", nameof(kind));
		}

		if (anchorType != "code" && anchorType != "diagram")
		{
			throw new ArgumentException("anchorType must be 'code' or 'diagram'", nameof(anchorType));
		}

		var reviewItem = new ReviewItem
		{
			Id = Guid.NewGuid(),
			ReviewId = reviewId,
			Kind = kind,
			AnchorType = anchorType,
			AnchorRef = anchorRef,
			Body = body,
			Status = "open",
			CreatedBy = createdBy,
			CreatedAt = DateTime.UtcNow
		};

		await _reviewItemRepository.AddAsync(reviewItem, cancellationToken);
		await _unitOfWork.SaveChangesAsync(cancellationToken);
		return reviewItem;
	}

	public async System.Threading.Tasks.Task<ReviewItem?> GetReviewItemAsync(Guid reviewItemId, System.Threading.CancellationToken cancellationToken = default)
	{
		return await _reviewItemRepository.GetByIdAsync(reviewItemId, cancellationToken);
	}

	public async System.Threading.Tasks.Task<IReadOnlyList<ReviewItem>> GetReviewItemsAsync(Guid reviewId, System.Threading.CancellationToken cancellationToken = default)
	{
		return await _reviewItemRepository.GetByReviewIdAsync(reviewId, cancellationToken);
	}

	public async System.Threading.Tasks.Task<ReviewItem> UpdateReviewItemAsync(Guid reviewItemId, string? body = null, string? status = null, System.Threading.CancellationToken cancellationToken = default)
	{
		var reviewItem = await _reviewItemRepository.GetByIdAsync(reviewItemId, cancellationToken);
		if (reviewItem == null)
		{
			throw new InvalidOperationException("ReviewItem not found");
		}

		if (status != null)
		{
			if (status != "open" && status != "resolved")
			{
				throw new ArgumentException("status must be 'open' or 'resolved'", nameof(status));
			}
			reviewItem.Status = status;
		}

		if (body != null)
		{
			reviewItem.Body = body;
		}

		reviewItem.UpdatedAt = DateTime.UtcNow;
		await _reviewItemRepository.UpdateAsync(reviewItem, cancellationToken);
		await _unitOfWork.SaveChangesAsync(cancellationToken);
		return reviewItem;
	}

	public async System.Threading.Tasks.Task DeleteReviewItemAsync(Guid reviewItemId, System.Threading.CancellationToken cancellationToken = default)
	{
		var reviewItem = await _reviewItemRepository.GetByIdAsync(reviewItemId, cancellationToken);
		if (reviewItem == null)
		{
			throw new InvalidOperationException("ReviewItem not found");
		}

		// Проверка наличия связанных комментариев
		var comments = await _commentRepository.GetByReviewItemIdAsync(reviewItemId, cancellationToken);
		if (comments.Count > 0)
		{
			throw new InvalidOperationException("Cannot delete review item with existing comments");
		}

		await _reviewItemRepository.DeleteAsync(reviewItemId, cancellationToken);
		await _unitOfWork.SaveChangesAsync(cancellationToken);
	}

	public async System.Threading.Tasks.Task<Comment> AddCommentAsync(Guid reviewItemId, string body, Guid authorId, System.Threading.CancellationToken cancellationToken = default)
	{
		var reviewItem = await _reviewItemRepository.GetByIdAsync(reviewItemId, cancellationToken);
		if (reviewItem == null)
		{
			throw new InvalidOperationException("ReviewItem not found");
		}

		var comment = new Comment
		{
			Id = Guid.NewGuid(),
			ReviewItemId = reviewItemId,
			Body = body,
			AuthorId = authorId,
			CreatedAt = DateTime.UtcNow
		};

		await _commentRepository.AddAsync(comment, cancellationToken);
		await _unitOfWork.SaveChangesAsync(cancellationToken);
		return comment;
	}

	public async System.Threading.Tasks.Task<IReadOnlyList<Comment>> GetCommentsAsync(Guid reviewItemId, System.Threading.CancellationToken cancellationToken = default)
	{
		return await _commentRepository.GetByReviewItemIdAsync(reviewItemId, cancellationToken);
	}

	public async System.Threading.Tasks.Task<Comment?> GetCommentAsync(Guid commentId, System.Threading.CancellationToken cancellationToken = default)
	{
		return await _commentRepository.GetByIdAsync(commentId, cancellationToken);
	}

	public async System.Threading.Tasks.Task<Comment> UpdateCommentAsync(Guid commentId, string body, System.Threading.CancellationToken cancellationToken = default)
	{
		var comment = await _commentRepository.GetByIdAsync(commentId, cancellationToken);
		if (comment == null)
		{
			throw new InvalidOperationException("Comment not found");
		}

		comment.Body = body;
		comment.UpdatedAt = DateTime.UtcNow;
		await _commentRepository.UpdateAsync(comment, cancellationToken);
		await _unitOfWork.SaveChangesAsync(cancellationToken);
		return comment;
	}

	public async System.Threading.Tasks.Task DeleteCommentAsync(Guid commentId, System.Threading.CancellationToken cancellationToken = default)
	{
		var comment = await _commentRepository.GetByIdAsync(commentId, cancellationToken);
		if (comment == null)
		{
			throw new InvalidOperationException("Comment not found");
		}

		await _commentRepository.DeleteAsync(commentId, cancellationToken);
		await _unitOfWork.SaveChangesAsync(cancellationToken);
	}
}

