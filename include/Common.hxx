#ifndef COMMON_HXX
#define COMMON_HXX

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include <GSL/gsl_assert.h>

#include <iostream>

#include <MetaUtils.hxx>

/* A set of class used to represent the in memory layout of the finite automaton, be it
 * deterministic or non-deterministic 
 */

// Made to enable the any '.' character support. But impedes a bit the performances, as
// the code was not thought to support it in the first place.
#define ANY_CHARACTER_SUPPORT
#define GSL_THROW_ON_CONTRACT_VIOLATION

using StateId = size_t;
using Input = char;

constexpr char epsilon = -1;
constexpr char any = -2;
constexpr char none = 0;

enum class NFAComponent : uint8_t
{
	Eps,
	Symbol,
	Union,
	Concatenation,
	Star
};

template<NFAComponent>
class NFAManipulation;

template<>
class NFAManipulation<NFAComponent::Eps>
{

};

class MatrixLayout
{
public:
	using State = std::vector<Input>;
	using InternalMatrix = std::vector<State>;

public:
	MatrixLayout() = default;
	MatrixLayout(const MatrixLayout&) = default;
	MatrixLayout(MatrixLayout&&) = default;

	MatrixLayout& operator=(const MatrixLayout&) = default;
	MatrixLayout& operator=(MatrixLayout&&) = default;

	void addState();
	void removeLastState();
	void addTransition(StateId from, StateId to, Input input);

	template<class T>
	auto addStatesOf(T&& other)
		-> std::enable_if_t<std::is_base_of<MatrixLayout, std::remove_reference_t<T>>::value, void>
	{
		size_t oldStateCount = getStateCount();
		size_t newStateCount = oldStateCount + other.getStateCount();

		using RowRefType = std::conditional_t<!std::is_reference<T>::value, State&&, const State&>;

		for (auto& row : container_)
		{
			row.resize(newStateCount);
		}
		for (RowRefType row : other.container_)
		{
			auto newRow = std::forward<RowRefType>(row);
			container_.push_back(State(newStateCount));
			std::move(newRow.begin(), newRow.end(), container_.back().begin() + oldStateCount);
		}
	}

	Input getTransition(StateId from, StateId to) const;

	size_t getStateCount() const noexcept;
	void debugDisplay() const;

private:
	InternalMatrix container_;
};

class MapLayout
{
	using MapType = std::multimap<Input, StateId>;
	using InternalLayoutType = std::vector<MapType>;

public:
	MapLayout() = default;
	MapLayout(const MapLayout&) = default;
	MapLayout(MapLayout&&) = default;

	MapLayout& operator=(const MapLayout&) = default;
	MapLayout& operator=(MapLayout&&) = default;

	void addState();
	void removeLastState();
	void addTransition(StateId from, StateId to, Input input);

	template<class T>
	auto addStatesOf(T&& other)
		-> std::enable_if_t<std::is_base_of<MapLayout, std::remove_reference_t<T>>::value, void>
	{
		size_t oldSize = getStateCount();

		for (auto map : other.internalMap_)
		{
			for (auto& elem : map)
			{
				elem.second += oldSize;
			}
			internalMap_.push_back(std::move(map));
		}
	}

	Input getTransition(StateId from, StateId to) const;
	size_t getStateCount() const;

	std::vector<StateId> makeTransition(std::vector<StateId> origin, Input input) const;

private:
	InternalLayoutType internalMap_;
};

template<class T, class = void>
struct has_transition_handler : std::false_type
{};

template<class T>
struct has_transition_handler < T, void_t<decltype(std::declval<T>().makeTransition(std::declval<std::vector<StateId>>(), std::declval<Input>())) >>
	: std::true_type{};


template<class TLayout>
class BaseLayout : public TLayout
{

public:
	BaseLayout() = default;
	BaseLayout(const BaseLayout&) = default;
	BaseLayout(BaseLayout&&) = default;

	BaseLayout& operator=(const BaseLayout&) = default;
	BaseLayout& operator=(BaseLayout&&) = default;

	void addState()
	{
		TLayout::addState();
	}

	template<class T>
	void addStatesOf(T&& other)
	{
		TLayout::addStatesOf(std::forward<T>(other));
	}

	void addTransition(StateId from, StateId to, Input input)
	{
		TLayout::addTransition(from, to, input);
	}

	void removeLastState()
	{
		TLayout::removeLastState();
	}

	Input getTransition(StateId from, StateId to) const
	{
		return TLayout::getTransition(from, to);
	}

	size_t getStateCount() const noexcept
	{
		return TLayout::getStateCount();
	}



#ifdef DEBUG
	void debugDisplay() const
	{
		TLayout::debugDisplay();
	}
#endif

};

template<class TLayout, class = void>
class Layout;

template<class TLayout>
class Layout<TLayout, std::enable_if_t<!has_transition_handler<TLayout>::value, void>> : public BaseLayout<TLayout>
{
public:
	using BaseLayout<TLayout>::BaseLayout;

	std::vector<StateId> makeTransition(std::vector<StateId> origin, Input input) const
	{
		std::vector<StateId> reachableStateSet;

		for (StateId state : origin)
		{
			for (StateId s = 0; s < this->getStateCount(); ++s)
			{
				if (this->getTransition(state, s) == input && (std::find(reachableStateSet.begin(), reachableStateSet.end(), s) == reachableStateSet.end()))
				{
					reachableStateSet.push_back(s);
				}

#ifdef ANY_CHARACTER_SUPPORT
				if (this->getTransition(state, s) == any && (std::find(reachableStateSet.begin(), reachableStateSet.end(), s) == reachableStateSet.end()))
				{
					reachableStateSet.push_back(s);
				}
#endif
			}
		}

		return reachableStateSet;
	}
};

template<class TLayout>
class Layout<TLayout, std::enable_if_t<has_transition_handler<TLayout>::value, void>> : public BaseLayout<TLayout>
{
public:
	using BaseLayout<TLayout>::BaseLayout;

	std::vector<StateId> makeTransition(std::vector<StateId> origin, Input input) const
	{
		return static_cast<const TLayout*>(this)->makeTransition(origin, input);
	}

};

#endif // COMMON_HXX