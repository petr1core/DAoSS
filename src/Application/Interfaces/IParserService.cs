namespace DAOSS.Application.Interfaces;

public interface IParserService
{
	object ParseToAst(string code, string language);
}


