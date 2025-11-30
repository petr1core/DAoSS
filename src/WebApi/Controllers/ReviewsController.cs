using System.ComponentModel.DataAnnotations;
using System.Security.Claims;
using DAOSS.Application.Interfaces;
using DAOSS.Application.Repositories;
using DAOSS.Domain.Entities;
using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc;

namespace DAOSS.WebApi.Controllers;

[ApiController]
[Route("api/projects/{projectId:guid}/reviews")]
public class ReviewsController : ControllerBase
{
	private readonly IReviewService _reviewService;
	private readonly IProjectRepository _projectRepository;

	public ReviewsController(IReviewService reviewService, IProjectRepository projectRepository)
	{
		_reviewService = reviewService;
		_projectRepository = projectRepository;
	}

	public sealed class CreateReviewDto
	{
		[Required]
		[RegularExpression("^(diagram_version|source_file_version)$", ErrorMessage = "targetType must be 'diagram_version' or 'source_file_version'")]
		public string TargetType { get; set; } = string.Empty;
		[Required]
		public Guid TargetId { get; set; }
	}

	public sealed class UpdateReviewDto
	{
		[Required]
		[RegularExpression("^(approved|changes_requested)$", ErrorMessage = "status must be 'approved' or 'changes_requested'")]
		public string Status { get; set; } = string.Empty;
	}

	public sealed class ReviewResponseDto
	{
		public Guid Id { get; set; }
		public Guid ProjectId { get; set; }
		public string TargetType { get; set; } = string.Empty;
		public Guid TargetId { get; set; }
		public string Status { get; set; } = string.Empty;
		public Guid CreatedBy { get; set; }
		public DateTime CreatedAt { get; set; }
		public DateTime? UpdatedAt { get; set; }
	}

	[HttpPost]
	[Authorize(Policy = "ProjectWrite")]
	public async Task<ActionResult<ReviewResponseDto>> Create(Guid projectId, [FromBody] CreateReviewDto dto, CancellationToken ct)
	{
		if (!ModelState.IsValid)
		{
			return BadRequest(ModelState);
		}

		var currentUserIdStr = User.FindFirstValue(ClaimTypes.NameIdentifier);
		if (!Guid.TryParse(currentUserIdStr, out var currentUserId))
		{
			return Unauthorized(new { message = "Invalid user identity" });
		}

		// Проверка существования проекта
		var project = await _projectRepository.GetByIdAsync(projectId, ct);
		if (project == null)
		{
			return NotFound(new { message = "Project not found" });
		}

		try
		{
			var review = await _reviewService.CreateReviewAsync(projectId, dto.TargetType, dto.TargetId, currentUserId, ct);
			var response = new ReviewResponseDto
			{
				Id = review.Id,
				ProjectId = review.ProjectId,
				TargetType = review.TargetType,
				TargetId = review.TargetId,
				Status = review.Status,
				CreatedBy = review.CreatedBy,
				CreatedAt = review.CreatedAt,
				UpdatedAt = review.UpdatedAt
			};
			return CreatedAtAction(nameof(Get), new { projectId, reviewId = review.Id }, response);
		}
		catch (InvalidOperationException ex)
		{
			return BadRequest(new { message = ex.Message });
		}
		catch (ArgumentException ex)
		{
			return BadRequest(new { message = ex.Message });
		}
	}

	[HttpGet]
	[Authorize(Policy = "ProjectRead")]
	public async Task<ActionResult<IReadOnlyList<ReviewResponseDto>>> GetAll(Guid projectId, CancellationToken ct)
	{
		var project = await _projectRepository.GetByIdAsync(projectId, ct);
		if (project == null)
		{
			return NotFound(new { message = "Project not found" });
		}

		var reviews = await _reviewService.GetReviewsByProjectAsync(projectId, ct);
		var result = reviews.Select(r => new ReviewResponseDto
		{
			Id = r.Id,
			ProjectId = r.ProjectId,
			TargetType = r.TargetType,
			TargetId = r.TargetId,
			Status = r.Status,
			CreatedBy = r.CreatedBy,
			CreatedAt = r.CreatedAt,
			UpdatedAt = r.UpdatedAt
		}).ToList();

		return Ok(result);
	}

	[HttpGet("{reviewId:guid}")]
	[Authorize(Policy = "ProjectRead")]
	public async Task<ActionResult<ReviewResponseDto>> Get(Guid projectId, Guid reviewId, CancellationToken ct)
	{
		var project = await _projectRepository.GetByIdAsync(projectId, ct);
		if (project == null)
		{
			return NotFound(new { message = "Project not found" });
		}

		var review = await _reviewService.GetReviewAsync(reviewId, ct);
		if (review == null)
		{
			return NotFound(new { message = "Review not found" });
		}

		if (review.ProjectId != projectId)
		{
			return BadRequest(new { message = "Review does not belong to this project" });
		}

		var response = new ReviewResponseDto
		{
			Id = review.Id,
			ProjectId = review.ProjectId,
			TargetType = review.TargetType,
			TargetId = review.TargetId,
			Status = review.Status,
			CreatedBy = review.CreatedBy,
			CreatedAt = review.CreatedAt,
			UpdatedAt = review.UpdatedAt
		};

		return Ok(response);
	}

	[HttpPut("{reviewId:guid}")]
	[Authorize(Policy = "ProjectWrite")]
	public async Task<IActionResult> Update(Guid projectId, Guid reviewId, [FromBody] UpdateReviewDto dto, CancellationToken ct)
	{
		if (!ModelState.IsValid)
		{
			return BadRequest(ModelState);
		}

		var project = await _projectRepository.GetByIdAsync(projectId, ct);
		if (project == null)
		{
			return NotFound(new { message = "Project not found" });
		}

		var review = await _reviewService.GetReviewAsync(reviewId, ct);
		if (review == null)
		{
			return NotFound(new { message = "Review not found" });
		}

		if (review.ProjectId != projectId)
		{
			return BadRequest(new { message = "Review does not belong to this project" });
		}

		try
		{
			await _reviewService.UpdateReviewStatusAsync(reviewId, dto.Status, ct);
			return NoContent();
		}
		catch (InvalidOperationException ex)
		{
			return BadRequest(new { message = ex.Message });
		}
		catch (ArgumentException ex)
		{
			return BadRequest(new { message = ex.Message });
		}
	}

	[HttpDelete("{reviewId:guid}")]
	[Authorize(Policy = "ProjectWrite")]
	public async Task<IActionResult> Delete(Guid projectId, Guid reviewId, CancellationToken ct)
	{
		var project = await _projectRepository.GetByIdAsync(projectId, ct);
		if (project == null)
		{
			return NotFound(new { message = "Project not found" });
		}

		var review = await _reviewService.GetReviewAsync(reviewId, ct);
		if (review == null)
		{
			return NotFound(new { message = "Review not found" });
		}

		if (review.ProjectId != projectId)
		{
			return BadRequest(new { message = "Review does not belong to this project" });
		}

		try
		{
			await _reviewService.DeleteReviewAsync(reviewId, ct);
			return NoContent();
		}
		catch (InvalidOperationException ex)
		{
			return BadRequest(new { message = ex.Message });
		}
	}

	// Review Items endpoints

	public sealed class CreateReviewItemDto
	{
		[Required]
		[RegularExpression("^(comment|issue)$", ErrorMessage = "kind must be 'comment' or 'issue'")]
		public string Kind { get; set; } = string.Empty;
		[Required]
		[RegularExpression("^(code|diagram)$", ErrorMessage = "anchorType must be 'code' or 'diagram'")]
		public string AnchorType { get; set; } = string.Empty;
		[Required]
		public string AnchorRef { get; set; } = string.Empty;
		[Required]
		public string Body { get; set; } = string.Empty;
	}

	public sealed class UpdateReviewItemDto
	{
		public string? Body { get; set; }
		[RegularExpression("^(open|resolved)$", ErrorMessage = "status must be 'open' or 'resolved'")]
		public string? Status { get; set; }
	}

	public sealed class ReviewItemResponseDto
	{
		public Guid Id { get; set; }
		public Guid ReviewId { get; set; }
		public string Kind { get; set; } = string.Empty;
		public string AnchorType { get; set; } = string.Empty;
		public string AnchorRef { get; set; } = string.Empty;
		public string Body { get; set; } = string.Empty;
		public string Status { get; set; } = string.Empty;
		public Guid CreatedBy { get; set; }
		public DateTime CreatedAt { get; set; }
		public DateTime? UpdatedAt { get; set; }
	}

	[HttpPost("{reviewId:guid}/items")]
	[Authorize(Policy = "ProjectWrite")]
	public async Task<ActionResult<ReviewItemResponseDto>> CreateItem(Guid projectId, Guid reviewId, [FromBody] CreateReviewItemDto dto, CancellationToken ct)
	{
		if (!ModelState.IsValid)
		{
			return BadRequest(ModelState);
		}

		var currentUserIdStr = User.FindFirstValue(ClaimTypes.NameIdentifier);
		if (!Guid.TryParse(currentUserIdStr, out var currentUserId))
		{
			return Unauthorized(new { message = "Invalid user identity" });
		}

		var project = await _projectRepository.GetByIdAsync(projectId, ct);
		if (project == null)
		{
			return NotFound(new { message = "Project not found" });
		}

		var review = await _reviewService.GetReviewAsync(reviewId, ct);
		if (review == null)
		{
			return NotFound(new { message = "Review not found" });
		}

		if (review.ProjectId != projectId)
		{
			return BadRequest(new { message = "Review does not belong to this project" });
		}

		try
		{
			var item = await _reviewService.CreateReviewItemAsync(reviewId, dto.Kind, dto.AnchorType, dto.AnchorRef, dto.Body, currentUserId, ct);
			var response = new ReviewItemResponseDto
			{
				Id = item.Id,
				ReviewId = item.ReviewId,
				Kind = item.Kind,
				AnchorType = item.AnchorType,
				AnchorRef = item.AnchorRef,
				Body = item.Body,
				Status = item.Status,
				CreatedBy = item.CreatedBy,
				CreatedAt = item.CreatedAt,
				UpdatedAt = item.UpdatedAt
			};
			return CreatedAtAction(nameof(GetItem), new { projectId, reviewId, itemId = item.Id }, response);
		}
		catch (InvalidOperationException ex)
		{
			return BadRequest(new { message = ex.Message });
		}
		catch (ArgumentException ex)
		{
			return BadRequest(new { message = ex.Message });
		}
	}

	[HttpGet("{reviewId:guid}/items")]
	[Authorize(Policy = "ProjectRead")]
	public async Task<ActionResult<IReadOnlyList<ReviewItemResponseDto>>> GetAllItems(Guid projectId, Guid reviewId, CancellationToken ct)
	{
		var project = await _projectRepository.GetByIdAsync(projectId, ct);
		if (project == null)
		{
			return NotFound(new { message = "Project not found" });
		}

		var review = await _reviewService.GetReviewAsync(reviewId, ct);
		if (review == null)
		{
			return NotFound(new { message = "Review not found" });
		}

		if (review.ProjectId != projectId)
		{
			return BadRequest(new { message = "Review does not belong to this project" });
		}

		var items = await _reviewService.GetReviewItemsAsync(reviewId, ct);
		var result = items.Select(i => new ReviewItemResponseDto
		{
			Id = i.Id,
			ReviewId = i.ReviewId,
			Kind = i.Kind,
			AnchorType = i.AnchorType,
			AnchorRef = i.AnchorRef,
			Body = i.Body,
			Status = i.Status,
			CreatedBy = i.CreatedBy,
			CreatedAt = i.CreatedAt,
			UpdatedAt = i.UpdatedAt
		}).ToList();

		return Ok(result);
	}

	[HttpGet("{reviewId:guid}/items/{itemId:guid}")]
	[Authorize(Policy = "ProjectRead")]
	public async Task<ActionResult<ReviewItemResponseDto>> GetItem(Guid projectId, Guid reviewId, Guid itemId, CancellationToken ct)
	{
		var project = await _projectRepository.GetByIdAsync(projectId, ct);
		if (project == null)
		{
			return NotFound(new { message = "Project not found" });
		}

		var review = await _reviewService.GetReviewAsync(reviewId, ct);
		if (review == null)
		{
			return NotFound(new { message = "Review not found" });
		}

		if (review.ProjectId != projectId)
		{
			return BadRequest(new { message = "Review does not belong to this project" });
		}

		var item = await _reviewService.GetReviewItemAsync(itemId, ct);
		if (item == null)
		{
			return NotFound(new { message = "Review item not found" });
		}

		if (item.ReviewId != reviewId)
		{
			return BadRequest(new { message = "Review item does not belong to this review" });
		}

		var response = new ReviewItemResponseDto
		{
			Id = item.Id,
			ReviewId = item.ReviewId,
			Kind = item.Kind,
			AnchorType = item.AnchorType,
			AnchorRef = item.AnchorRef,
			Body = item.Body,
			Status = item.Status,
			CreatedBy = item.CreatedBy,
			CreatedAt = item.CreatedAt,
			UpdatedAt = item.UpdatedAt
		};

		return Ok(response);
	}

	[HttpPut("{reviewId:guid}/items/{itemId:guid}")]
	[Authorize(Policy = "ProjectWrite")]
	public async Task<IActionResult> UpdateItem(Guid projectId, Guid reviewId, Guid itemId, [FromBody] UpdateReviewItemDto dto, CancellationToken ct)
	{
		if (!ModelState.IsValid)
		{
			return BadRequest(ModelState);
		}

		var project = await _projectRepository.GetByIdAsync(projectId, ct);
		if (project == null)
		{
			return NotFound(new { message = "Project not found" });
		}

		var review = await _reviewService.GetReviewAsync(reviewId, ct);
		if (review == null)
		{
			return NotFound(new { message = "Review not found" });
		}

		if (review.ProjectId != projectId)
		{
			return BadRequest(new { message = "Review does not belong to this project" });
		}

		var item = await _reviewService.GetReviewItemAsync(itemId, ct);
		if (item == null)
		{
			return NotFound(new { message = "Review item not found" });
		}

		if (item.ReviewId != reviewId)
		{
			return BadRequest(new { message = "Review item does not belong to this review" });
		}

		try
		{
			await _reviewService.UpdateReviewItemAsync(itemId, dto.Body, dto.Status, ct);
			return NoContent();
		}
		catch (InvalidOperationException ex)
		{
			return BadRequest(new { message = ex.Message });
		}
		catch (ArgumentException ex)
		{
			return BadRequest(new { message = ex.Message });
		}
	}

	[HttpDelete("{reviewId:guid}/items/{itemId:guid}")]
	[Authorize(Policy = "ProjectWrite")]
	public async Task<IActionResult> DeleteItem(Guid projectId, Guid reviewId, Guid itemId, CancellationToken ct)
	{
		var project = await _projectRepository.GetByIdAsync(projectId, ct);
		if (project == null)
		{
			return NotFound(new { message = "Project not found" });
		}

		var review = await _reviewService.GetReviewAsync(reviewId, ct);
		if (review == null)
		{
			return NotFound(new { message = "Review not found" });
		}

		if (review.ProjectId != projectId)
		{
			return BadRequest(new { message = "Review does not belong to this project" });
		}

		var item = await _reviewService.GetReviewItemAsync(itemId, ct);
		if (item == null)
		{
			return NotFound(new { message = "Review item not found" });
		}

		if (item.ReviewId != reviewId)
		{
			return BadRequest(new { message = "Review item does not belong to this review" });
		}

		try
		{
			await _reviewService.DeleteReviewItemAsync(itemId, ct);
			return NoContent();
		}
		catch (InvalidOperationException ex)
		{
			return BadRequest(new { message = ex.Message });
		}
	}

	// Comments endpoints

	public sealed class CreateCommentDto
	{
		[Required]
		public string Body { get; set; } = string.Empty;
	}

	public sealed class UpdateCommentDto
	{
		[Required]
		public string Body { get; set; } = string.Empty;
	}

	public sealed class CommentResponseDto
	{
		public Guid Id { get; set; }
		public Guid ReviewItemId { get; set; }
		public string Body { get; set; } = string.Empty;
		public Guid AuthorId { get; set; }
		public DateTime CreatedAt { get; set; }
		public DateTime? UpdatedAt { get; set; }
	}

	[HttpPost("{reviewId:guid}/items/{itemId:guid}/comments")]
	[Authorize(Policy = "ProjectWrite")]
	public async Task<ActionResult<CommentResponseDto>> CreateComment(Guid projectId, Guid reviewId, Guid itemId, [FromBody] CreateCommentDto dto, CancellationToken ct)
	{
		if (!ModelState.IsValid)
		{
			return BadRequest(ModelState);
		}

		var currentUserIdStr = User.FindFirstValue(ClaimTypes.NameIdentifier);
		if (!Guid.TryParse(currentUserIdStr, out var currentUserId))
		{
			return Unauthorized(new { message = "Invalid user identity" });
		}

		var project = await _projectRepository.GetByIdAsync(projectId, ct);
		if (project == null)
		{
			return NotFound(new { message = "Project not found" });
		}

		var review = await _reviewService.GetReviewAsync(reviewId, ct);
		if (review == null)
		{
			return NotFound(new { message = "Review not found" });
		}

		if (review.ProjectId != projectId)
		{
			return BadRequest(new { message = "Review does not belong to this project" });
		}

		var item = await _reviewService.GetReviewItemAsync(itemId, ct);
		if (item == null)
		{
			return NotFound(new { message = "Review item not found" });
		}

		if (item.ReviewId != reviewId)
		{
			return BadRequest(new { message = "Review item does not belong to this review" });
		}

		try
		{
			var comment = await _reviewService.AddCommentAsync(itemId, dto.Body, currentUserId, ct);
			var response = new CommentResponseDto
			{
				Id = comment.Id,
				ReviewItemId = comment.ReviewItemId,
				Body = comment.Body,
				AuthorId = comment.AuthorId,
				CreatedAt = comment.CreatedAt,
				UpdatedAt = comment.UpdatedAt
			};
			return CreatedAtAction(nameof(GetComment), new { projectId, reviewId, itemId, commentId = comment.Id }, response);
		}
		catch (InvalidOperationException ex)
		{
			return BadRequest(new { message = ex.Message });
		}
	}

	[HttpGet("{reviewId:guid}/items/{itemId:guid}/comments")]
	[Authorize(Policy = "ProjectRead")]
	public async Task<ActionResult<IReadOnlyList<CommentResponseDto>>> GetAllComments(Guid projectId, Guid reviewId, Guid itemId, CancellationToken ct)
	{
		var project = await _projectRepository.GetByIdAsync(projectId, ct);
		if (project == null)
		{
			return NotFound(new { message = "Project not found" });
		}

		var review = await _reviewService.GetReviewAsync(reviewId, ct);
		if (review == null)
		{
			return NotFound(new { message = "Review not found" });
		}

		if (review.ProjectId != projectId)
		{
			return BadRequest(new { message = "Review does not belong to this project" });
		}

		var item = await _reviewService.GetReviewItemAsync(itemId, ct);
		if (item == null)
		{
			return NotFound(new { message = "Review item not found" });
		}

		if (item.ReviewId != reviewId)
		{
			return BadRequest(new { message = "Review item does not belong to this review" });
		}

		var comments = await _reviewService.GetCommentsAsync(itemId, ct);
		var result = comments.Select(c => new CommentResponseDto
		{
			Id = c.Id,
			ReviewItemId = c.ReviewItemId,
			Body = c.Body,
			AuthorId = c.AuthorId,
			CreatedAt = c.CreatedAt,
			UpdatedAt = c.UpdatedAt
		}).ToList();

		return Ok(result);
	}

	[HttpGet("{reviewId:guid}/items/{itemId:guid}/comments/{commentId:guid}")]
	[Authorize(Policy = "ProjectRead")]
	public async Task<ActionResult<CommentResponseDto>> GetComment(Guid projectId, Guid reviewId, Guid itemId, Guid commentId, CancellationToken ct)
	{
		var project = await _projectRepository.GetByIdAsync(projectId, ct);
		if (project == null)
		{
			return NotFound(new { message = "Project not found" });
		}

		var review = await _reviewService.GetReviewAsync(reviewId, ct);
		if (review == null)
		{
			return NotFound(new { message = "Review not found" });
		}

		if (review.ProjectId != projectId)
		{
			return BadRequest(new { message = "Review does not belong to this project" });
		}

		var item = await _reviewService.GetReviewItemAsync(itemId, ct);
		if (item == null)
		{
			return NotFound(new { message = "Review item not found" });
		}

		if (item.ReviewId != reviewId)
		{
			return BadRequest(new { message = "Review item does not belong to this review" });
		}

		var comment = await _reviewService.GetCommentAsync(commentId, ct);
		if (comment == null)
		{
			return NotFound(new { message = "Comment not found" });
		}

		if (comment.ReviewItemId != itemId)
		{
			return BadRequest(new { message = "Comment does not belong to this review item" });
		}

		var response = new CommentResponseDto
		{
			Id = comment.Id,
			ReviewItemId = comment.ReviewItemId,
			Body = comment.Body,
			AuthorId = comment.AuthorId,
			CreatedAt = comment.CreatedAt,
			UpdatedAt = comment.UpdatedAt
		};

		return Ok(response);
	}

	[HttpPut("{reviewId:guid}/items/{itemId:guid}/comments/{commentId:guid}")]
	[Authorize(Policy = "ProjectWrite")]
	public async Task<IActionResult> UpdateComment(Guid projectId, Guid reviewId, Guid itemId, Guid commentId, [FromBody] UpdateCommentDto dto, CancellationToken ct)
	{
		if (!ModelState.IsValid)
		{
			return BadRequest(ModelState);
		}

		var project = await _projectRepository.GetByIdAsync(projectId, ct);
		if (project == null)
		{
			return NotFound(new { message = "Project not found" });
		}

		var review = await _reviewService.GetReviewAsync(reviewId, ct);
		if (review == null)
		{
			return NotFound(new { message = "Review not found" });
		}

		if (review.ProjectId != projectId)
		{
			return BadRequest(new { message = "Review does not belong to this project" });
		}

		var item = await _reviewService.GetReviewItemAsync(itemId, ct);
		if (item == null)
		{
			return NotFound(new { message = "Review item not found" });
		}

		if (item.ReviewId != reviewId)
		{
			return BadRequest(new { message = "Review item does not belong to this review" });
		}

		var comment = await _reviewService.GetCommentAsync(commentId, ct);
		if (comment == null)
		{
			return NotFound(new { message = "Comment not found" });
		}

		if (comment.ReviewItemId != itemId)
		{
			return BadRequest(new { message = "Comment does not belong to this review item" });
		}

		try
		{
			await _reviewService.UpdateCommentAsync(commentId, dto.Body, ct);
			return NoContent();
		}
		catch (InvalidOperationException ex)
		{
			return BadRequest(new { message = ex.Message });
		}
	}

	[HttpDelete("{reviewId:guid}/items/{itemId:guid}/comments/{commentId:guid}")]
	[Authorize(Policy = "ProjectWrite")]
	public async Task<IActionResult> DeleteComment(Guid projectId, Guid reviewId, Guid itemId, Guid commentId, CancellationToken ct)
	{
		var project = await _projectRepository.GetByIdAsync(projectId, ct);
		if (project == null)
		{
			return NotFound(new { message = "Project not found" });
		}

		var review = await _reviewService.GetReviewAsync(reviewId, ct);
		if (review == null)
		{
			return NotFound(new { message = "Review not found" });
		}

		if (review.ProjectId != projectId)
		{
			return BadRequest(new { message = "Review does not belong to this project" });
		}

		var item = await _reviewService.GetReviewItemAsync(itemId, ct);
		if (item == null)
		{
			return NotFound(new { message = "Review item not found" });
		}

		if (item.ReviewId != reviewId)
		{
			return BadRequest(new { message = "Review item does not belong to this review" });
		}

		var comment = await _reviewService.GetCommentAsync(commentId, ct);
		if (comment == null)
		{
			return NotFound(new { message = "Comment not found" });
		}

		if (comment.ReviewItemId != itemId)
		{
			return BadRequest(new { message = "Comment does not belong to this review item" });
		}

		try
		{
			await _reviewService.DeleteCommentAsync(commentId, ct);
			return NoContent();
		}
		catch (InvalidOperationException ex)
		{
			return BadRequest(new { message = ex.Message });
		}
	}
}

