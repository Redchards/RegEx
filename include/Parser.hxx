#ifndef PARSER_HXX
#define PARSER_HXX

#include <exception>
#include <string>
#include <utility>

#include <NFA.hxx>
#include <Range.hxx>


class ExtraneousParenthesisException : public std::exception
{
public:
	ExtraneousParenthesisException() = default;
	const char* what() const noexcept override
	{
		return "Unexpected ')'";
	}
};

class BadRangeException : public std::exception
{
public:
	BadRangeException(std::string msg) : msg_{msg}
	{}
	
	const char* what() const noexcept override
	{
		return msg_.c_str();
	}
	
private:
	std::string msg_;
};

template<class TNFA>
class Parser
{
public:
	static TNFA parse(const std::string& str)
	{
		return parseImpl({ str.begin(), str.end() }, false, 0).first;
	}
	
private:
	static std::pair<TNFA, size_t> parseImpl(range<const std::string::const_iterator> strSpan, const bool inParenthesis, size_t recCount)
	{
		TNFA resultNFA{};
		TNFA intermediate{};

		std::pair<TNFA, size_t> intermediateResult{};

		bool needEscaping = false;
		
		std::vector<TNFA> partialResultVector{};

		for (auto it = strSpan.begin(); it < strSpan.end(); ++it)
		{
			auto c = *it;
			
			if(needEscaping)
			{
				needEscaping = false;
				resultNFA.concatenate({ c });
				continue;
			}

			switch (c)
			{
				case '.' :
#ifdef ANY_CHARACTER_SUPPORT
				partialResultVector.emplace_back(any);
#endif
				break;

				case '*' :
					partialResultVector.back().star();
					break;

				case '+' :
					partialResultVector.back().plus();
					break;

				case '(' :
					intermediateResult = std::move(parseImpl({ it + 1, strSpan.end() }, true, recCount));
					partialResultVector.emplace_back(std::move(intermediateResult.first));
					it += intermediateResult.second + 1;
					break;

				case ')' :
					if (inParenthesis)
					{
						for(const auto& part : partialResultVector){ resultNFA.concatenate(part); }
						return { resultNFA, std::distance(strSpan.first, it) };
					}
					else
					{
						throw ExtraneousParenthesisException();
					}
					break;

				case '|' :
					intermediateResult = std::move(parseImpl({ it + 1, strSpan.end()}, inParenthesis, recCount + 1));
					resultNFA.unify(intermediateResult.first);
					it += intermediateResult.second + 1;
					if(recCount != 0)
					{
						return { resultNFA, std::distance(strSpan.begin(), it) };
					}
					break;
					
				case '\\' :
					needEscaping = true;
					break;
					
				case '[' :
					partialResultVector.emplace_back(makeCharRange({it, strSpan.end()}));
					/*std::cout << "Hello" << std::endl;*/
					it += 4;
					break;

				default:
					partialResultVector.emplace_back(c);
			}
		}

		for(const auto& part : partialResultVector){ resultNFA.concatenate(part); }
		return { resultNFA, std::distance(strSpan.begin(), strSpan.end()) };
	}
	
	static TNFA makeCharRange(range<const std::string::const_iterator> strSpan)
	{
		static constexpr int8_t rangeValidSize = 2;
		
		auto first = strSpan.begin() + 1;
		
		TNFA resultNFA;
		
		auto it = strSpan.begin() + 2;
		for(; it != strSpan.end() && *it != ']'; ++it){}
		
		auto last = it - 1;
		
		if(std::distance(first, last) != rangeValidSize)
		{
			std::cout << std::distance(first, last) << std::endl;
			std::string tmpStr{"["};
			for(auto it = first; it != strSpan.end() && it != last + 1; ++it)
			{
				tmpStr += *it;
			}
			tmpStr += ']';
			throw BadRangeException(std::string{"Ill-formed range expression : "} + std::move(tmpStr));
		}
		if(*first > *last)
		{
			throw BadRangeException(std::string{"Invalid range : '"} + *first + "' greater lexicographically than '" + *last + "' ! ");
		}
		
		for(auto current = *first; current != *last + 1; ++current)
		{
			resultNFA.unify(current);
		}
		
		return resultNFA;
	}

};

#endif // PARSER_HXX