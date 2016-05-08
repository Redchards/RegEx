#ifndef NFA_HXX
#define NFA_HXX

#include <algorithm>
#include <cstdint>
#include <set>
#include <string>
#include <vector>

#include <iostream>

#include <Common.hxx>



/* Simple representation of a nondeterminist finite automaton, allowing different
 * internal data layouts. The basic layouts can be seen in 'Common.hxx'
 */
template<class TLayout>
class NFA : public Layout<TLayout>
{
public:
	/* Default constructor. Will build an empty automaton. */
	NFA()
	: Layout<TLayout>(),
		entryState_{ 0 },
		possibleInputs_{}
	{}

	/* Converting constructor from single input.
	 * Will build an automaton matching only this input.
	 */
	NFA(Input input)
	: Layout<TLayout>(),
	  entryState_{0},
	  possibleInputs_{}
	{
		this->addState();
		this->addState();

		this->addTransition(entryState_, this->getStateCount() - 1, input);

		if (input != epsilon)
		{
			possibleInputs_.insert(input);
		}
	}
	
	/* Defaulted copy/move constructors */
	NFA(const NFA& other) = default;
	NFA(NFA&& other) = default;
	
	/* Defaulted copy/move operators */
	NFA& operator=(const NFA& other) = default;
	NFA& operator=(NFA&&) = default;

	/* Concatenation with an other automaton. */
	void concatenate(const NFA& other)
	{
		this->insertNewInputs(other);
		
		size_t oldSize = this->getStateCount();
		StateId oldLastState = oldSize > 0 ? oldSize - 1 : 0;

		StateId otherEntryState = other.getEntryState();
		
		/* Handle special cases */
		if(oldSize == 1)
		{
			this->addStatesOf(other);
			this->addTransition(getEntryState(), otherEntryState + oldSize, epsilon);
			return;
		}
		else if(oldSize > 1 && this->getTransition(oldLastState, oldLastState) != none)
		{
			this->addStatesOf(other);
			this->addTransition(oldLastState, otherEntryState + oldSize, epsilon);
			return;
		}
		else if (oldSize > 1 && this->getTransition(oldLastState, oldLastState) == none)
		{
			this->removeLastState();
		}
		else if(oldSize == 0)
		{
			entryState_ = other.getEntryState();
			oldSize = 1;
		}

		this->addStatesOf(other);

		for (StateId s = 0; s < oldSize - 1; ++s)
		{
			auto oldTransition = this->getTransition(s, oldLastState);
			if (oldTransition != none)
			{
				this->addTransition(s, oldLastState, none);
				this->addTransition(s, otherEntryState + oldSize - 1, oldTransition);
			}
		}
	}

	/* Union with an other automaton */
	void unify(const NFA& other)
	{
		this->insertNewInputs(other);
		
		/* If the automaton to make union with is empty, do nothing.
		 * In the other hand, if we are empty, make concatenation
		 */
		if (other.getStateCount() == 0)
		{
			return;
		}
		else if (this->getStateCount() == 0)
		{
			concatenate(other);
			return;
		}
		

		StateId oldLastState = this->getStateCount() - 1;
		StateId oldEntryState = getEntryState();
		StateId firstOtherState = other.getEntryState();
		

		size_t oldSize = this->getStateCount();

		this->addStatesOf(other);
		StateId lastOtherState = other.getStateCount() - 1;

		StateId newFirstState = 0;
		StateId newLastState = oldLastState;
		
		/* Check if this automaton, or the automaton to make union with is already in a "unified" form,
		 * meaning it already had a union operation applied to it.
		 * If so, an optimized operation is performed, resulting in less states in the final automaton.
		 */
		if(other.isUnified())
		{
			entryState_ = other.getEntryState() + oldSize;
			
			this->addTransition(oldLastState, lastOtherState, epsilon);
			this->addTransition(getEntryState(), oldEntryState, epsilon);
		}
		else if(this->isUnified())
		{
			this->addTransition(oldLastState, lastOtherState + oldSize, epsilon);
			this->addTransition(getEntryState(), other.getEntryState() + oldSize, epsilon);
		}
		else
		{
			this->addState();
			entryState_ = this->getStateCount() - 1;
			this->addTransition(getEntryState(), oldEntryState, epsilon);
			
			
			this->addState();
			newLastState = this->getStateCount() - 1;
	
			this->addTransition(lastOtherState + oldSize, newLastState, epsilon);
			this->addTransition(oldLastState, newLastState, epsilon);
			
			this->addTransition(getEntryState(), firstOtherState + oldLastState + 1, epsilon);
		}
	}
	
	/* Plus operation (open Kleene) */
	void plus()
	{
		StateId oldLastState = this->getStateCount() - 1;
		StateId oldEntryState = this->getEntryState();
		
		/* Need to add a state to not break the contract the the last state is the only
		 * final state, and has no transition.
		 */
		this->addState();
		StateId newLastState = this->getStateCount() - 1;

		this->addTransition(oldLastState, newLastState, epsilon);
		this->addTransition(oldLastState, oldEntryState, epsilon);
	}

	/* Star operation (Kleene closure) */
	void star()
	{
		StateId oldLastState = this->getStateCount() - 1;
		StateId oldEntryState = this->getEntryState();
		
		// Possibly costly if state count is badly implemented, or costly by essence in the layout.
		// Also, an additional branch. Make this optional ?
		if(isSimpleCharacter()) 
		{

			Input oldTransition = this->getTransition(oldEntryState, oldLastState);
			this->addTransition(oldEntryState, oldLastState, none);
			this->removeLastState();
			this->addTransition(oldEntryState, oldEntryState, oldTransition);

			return;
		}
		
		this->addTransition(oldLastState, oldEntryState, epsilon);


		this->addState();
		StateId newLastState = this->getStateCount() - 1;
		this->addTransition(oldLastState, newLastState, epsilon);
		this->addTransition(oldEntryState, newLastState, epsilon);
	}

	/* Return the entry state */
	StateId getEntryState() const noexcept
	{
		return entryState_;
	}

	/* Return the set of all possible inputs */
	const std::set<Input>& getPossibleInputs() const noexcept
	{
		return possibleInputs_;
	}
	
	/* Simulate the automaton. Return true if the automaton is matching the given string, false otherwise. */
	bool simulate(std::string str) const
	{
		std::vector<StateId> currentState = this->computeEpsilonClosure(std::vector<StateId>{ getEntryState() });
		for(auto c : str)
		{
			currentState = this->computeEpsilonClosure(this->makeTransition(currentState, c));

			if(currentState.empty())
			{
				return false;
			}
		}
		if(std::find(currentState.begin(), currentState.end(), this->getStateCount() - 1) != currentState.end())
		{
			return true;
		}
		return false;
	}
	
	/* Check if the automaton is in a unified form */
	bool isUnified() const noexcept
	{
		size_t entryStateEpsilonCount = 0;
		
		for(StateId s = 0; s < this->getStateCount(); ++s)
		{
			if(s != getEntryState() && this->getTransition(getEntryState(), s) == epsilon)
			{
				++entryStateEpsilonCount;
				if(entryStateEpsilonCount == 2)
				{
					return true;
				}
			}
		}
		
		return false;
	}
	
	/* Check if the automaton matches a single character. */
	bool isSimpleCharacter()
	{
		return (this->getStateCount() == 2 && this->getTransition(getEntryState(), getEntryState()) == none);
	}

	/* Compute the epsilon closure of the transition. */
	template<class StateSet>
	auto computeEpsilonClosure(StateSet&& reachableStates) const
		-> std::enable_if_t<std::is_same<std::remove_reference_t<StateSet>, std::vector<StateId>>::value, std::vector<StateId>>
	{
		std::vector<StateId> epsilonClosure{ reachableStates };

		if (possibleInputs_.size() == 0)
		{
			return {}; 
		}

		while (!reachableStates.empty())
		{
			StateId state = reachableStates.back();
			reachableStates.pop_back();
			for (StateId s = 0; s < this->getStateCount(); ++s)
			{
				if (this->getTransition(state, s) == epsilon)
				{
					epsilonClosure.push_back(s);
					reachableStates.push_back(s);
				}
			}
		}

		return epsilonClosure;
	}
	
private:
	/* Insert new inputs in the set of possible inputs, ignoring epsilon inputs. */
	void insertNewInputs(const NFA& other)
	{
		for (auto input : other.getPossibleInputs())
		{
			if (input != epsilon)
			{
				possibleInputs_.insert(input);
			}
		}
	}

	StateId entryState_;
	std::set<Input> possibleInputs_;
};

#endif // NFA_HXX