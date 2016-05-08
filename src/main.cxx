#include <DFA.hxx>
#include <NFA.hxx>
#include <Parser.hxx>

int main() try
{
	using StandardNFA = NFA<MatrixLayout>;

	while (true)
	{
		std::string tst;
		StandardNFA nfa;

		std::cout << "Please enter a regex : ";
		std::cin >> tst;

		nfa = Parser<StandardNFA>::parse(tst);
		DFA<MatrixLayout> dfa;
		dfa.buildFrom(nfa);

		while (true)
		{
			std::cout << "> ";
			std::cin >> tst;
			//nfa.debugDisplay(); std::cout << std::endl;
			//dfa.debugDisplay();
			std::cout << (dfa.simulate(tst) ? "match" : "do not match") << std::endl;
		}
	}
	
	return 0;
}
catch (const ExtraneousParenthesisException& e)
{
	std::cout << e.what() << std::endl;
}
catch (const BadRangeException& e)
{
	std::cout << e.what() << std::endl;
}
catch (...)
{
	std::cout << "Unhandled exception !" << std::endl;
}
