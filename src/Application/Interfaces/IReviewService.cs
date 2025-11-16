namespace DAOSS.Application.Interfaces;

using DAOSS.Domain.Entities;

public interface IReviewService
{
	void AddComment(Guid diagramId, Comment comment);
	IReadOnlyList<Comment> GetComments(Guid diagramId);
}


