#ifndef DFA_HXX
#define DFA_HXX

#include <unordered_map>
#include <stack>
#include <string>
#include <vector>
#include <utility>

#include <Common.hxx>
#include <NFA.hxx>
#include <Optional.hxx>

/* Simple representation of a deterministe finite automaton.
 * One can use a NFA to build a DFA, but it's not possible to perform basic operations like
 * concatenation, union, star, plus and such to a DFA in this library.
 * In fact, this decision was taken because the scope of the library is to provide a small regex engine,
 * and not a fully fledged finite automata library.
 */
template<class TLayout>
class DFA : public Layout<TLayout>
{
public:
	/* Default constructor */
	DFA() = default;
	
	/* Constructs from a NFA, using the buildFrom() method */
	template<class NFALayout>
	DFA(const NFA<NFALayout>& nfa)
	{
		buildFrom(nfa);
	}
	
	/* Build the DFA from an NFA, using the classic algorithm */
	template<class NFALayout>
	void buildFrom(const NFA<NFALayout>& nfa)
	{
		/* Compute the epsilon closure from the entry state. This will be the first state of our DFA */
		std::vector<StateId> currentClosure = nfa.computeEpsilonClosure(std::vector<StateId>{ nfa.getEntryState() });
		
		/* Vector to map the resulting states of the DFA */
		std::vector<std::vector<StateId>> mappedDfaStates;
		entryState_ = 0;

		/* Add our first state, and set up the requires value for the algorithm */
		this->addState();
		mappedDfaStates.push_back(std::move(currentClosure));

		std::stack<StateId> dfaStates;
		dfaStates.push(0);

		/* While we still have unexplored states */
		while (!dfaStates.empty())
		{
			StateId currentState = dfaStates.top();
			dfaStates.pop();

			/* If the DFA state contains a final state of the NFA, mark it final */
			if (std::find(mappedDfaStates[currentState].begin(), mappedDfaStates[currentState].end(), nfa.getStateCount() - 1) != mappedDfaStates[currentState].end())
			{
				finalStates_.push_back(currentState);
			}
			for (Input input : nfa.getPossibleInputs())
			{
				/* Build the next DFA state from the result of the transition */
				auto newState = nfa.computeEpsilonClosure(nfa.makeTransition(mappedDfaStates[currentState], input));

				/* If the transition yields something */
				if (!newState.empty()) {
					/* Look in the DFA state table if we already saw this state */
					auto newStateIdIterator = std::find(mappedDfaStates.begin(), mappedDfaStates.end(), newState);
					StateId newStateId;
					
					/* If not, add a state add a transition, and push the new state id to the unexplored states stack. 
					 * Else, retrieve the state id.
					 */
					if (newStateIdIterator == mappedDfaStates.end())
					{
						this->addState();
						newStateId = this->getStateCount() - 1;
						mappedDfaStates.push_back(newState);
						dfaStates.push(newStateId);
					}
					else
					{
						newStateId = std::distance(mappedDfaStates.begin(), newStateIdIterator);
					}
					this->addTransition(currentState, newStateId, input);
				}

			}
		}
	}
	
	// TODO : Implement !
	//void buildFrom(NFA&&);
	
	/* Simulate the DFA */
	bool simulate(std::string str)
	{
		optional<StateId> currentState{ getEntryState() };
		for (auto c : str)
		{
			currentState = makeTransition(*currentState, c);
			if (!currentState) {
				return false;
			}
		}
		if (std::find(finalStates_.begin(), finalStates_.end(), *currentState) != finalStates_.end())
		{
			return true;
		}
		return false;
	}

	/* Perform the transition. Return a nullopt optional if no transition exists from this state for this input */
	optional<StateId> makeTransition(StateId from, Input input)
	{
		optional<StateId> result;
		for (StateId s = 0; s < this->getStateCount(); ++s)
		{
			if (this->getTransition(from, s) == input)
			{
				result = s;
				break;
			}
		}
		return result;
	}

	/* Return the entry state */
	StateId getEntryState() const noexcept
	{
		return entryState_;
	}

private:
	StateId entryState_;
	std::vector<StateId> finalStates_;
};

#endif // DFA_HXX