namespace DAOSS.Application.Interfaces;

using DAOSS.Domain.Entities;

public interface IDiagramService
{
	Diagram GenerateFromCode(CodeRegion region);
	void UpdateDiagram(Diagram diagram);
}


