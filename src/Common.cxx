#include <Common.hxx>

void MatrixLayout::addState()
{
	size_t stateCount = getStateCount();

	for (auto& row : container_)
	{
		row.push_back({});
	}

	container_.push_back(State(stateCount + 1));
}

void MatrixLayout::removeLastState()
{
	container_.pop_back();
}

void MatrixLayout::addTransition(StateId from, StateId to, Input input)
{
	Ensures(from < getStateCount());
	Ensures(to < getStateCount());

#ifndef ANY_CHARACTER_SUPPORT
	Ensures(input >= 0 || input == epsilon);
#else
	Ensures(input >= 0 || input == epsilon || input == any);
#endif

	container_[from][to] = input;
}

Input MatrixLayout::getTransition(StateId from, StateId to) const
{
	Ensures(from < getStateCount());
	Ensures(to < getStateCount());

	return container_[from][to];
}

size_t MatrixLayout::getStateCount() const noexcept
{
	return container_.size();
}

void MatrixLayout::debugDisplay() const
{
	for (size_t i = 0; i < container_.size(); ++i)
	{
		auto& row = container_[i];
		for (size_t j = 0; j < row.size(); ++j)
		{
			if (row[j] != none)
			{
				std::cout << "From : " << i << ",  To : " << j << " = " << (row[j] == epsilon ? "epsilon" : std::string{ row[j] }) << std::endl;
			}
		}
	}
}

void MapLayout::addState()
{
	internalMap_.push_back({});
}

void MapLayout::removeLastState()
{	
	internalMap_.pop_back();
}

void MapLayout::addTransition(StateId from, StateId to, Input input)
{
	Ensures(from < getStateCount());
	Ensures(to < getStateCount());

#ifndef ANY_CHARACTER_SUPPORT
	Ensures(input >= 0 || input == epsilon);
#else
	Ensures(input >= 0 || input == epsilon || input == any);
#endif

	internalMap_[from].insert({ input, to });
}

Input MapLayout::getTransition(StateId from, StateId to) const
{
	auto elem = std::find_if(internalMap_[from].begin(), internalMap_[from].end(), 
		[to](const MapType::value_type& val) {
			return val.second == to;
		});
		
	return (elem != internalMap_[from].end() ? elem->first : none);
}

size_t MapLayout::getStateCount() const
{
	return internalMap_.size();
}

// Special function, allowing to simulate a NFA.
std::vector<StateId> MapLayout::makeTransition(std::vector<StateId> origin, Input input) const
{
	std::vector<StateId> result;

	for (auto state : origin)
	{
		auto range = internalMap_[state].equal_range(input);
	
		for (auto destination = range.first; destination != range.second; ++destination)
		{
			if(std::find(result.begin(), result.end(), destination->second) == result.end())
			{
				result.push_back(destination->second);
			}
		}

#ifdef ANY_CHARACTER_SUPPORT
		auto rangeAny = internalMap_[state].equal_range(any);

		for (auto destination = rangeAny.first; destination != rangeAny.second; ++destination)
		{
			if (std::find(result.begin(), result.end(), destination->second) == result.end())
			{
				result.push_back(destination->second);
			}
		}
#endif
	}

	return result;
}