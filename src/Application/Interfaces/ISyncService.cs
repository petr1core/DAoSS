namespace DAOSS.Application.Interfaces;

using DAOSS.Domain.Entities;

public interface ISyncService
{
	void SyncCodeToDiagram(SourceFile sourceFile);
	void SyncDiagramToCode(Diagram diagram);
}


