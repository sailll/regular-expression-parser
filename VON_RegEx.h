/*! \file Von_RegEx.h
    \brief Regular Expression Parser
	\author 蔡仲谋

	Use it as following:					\n
											\n
	string strPattern;						\n
	int nPos = 0;							\n
											\n
	if(SetRegEx("....."))					\n
	{										\n
		if(FindFirst(...))					\n
		{									\n
			// Do something with result		\n
			while(FindNext(...))			\n
			{								\n
				// Process Result			\n
			}								\n
		}									\n
	}										\n
*/

#pragma once

// for debug STL exceeds 255 chars and MS compiler complains, so disable it
#pragma warning(disable:4786)
#pragma warning(disable:4503)

#include <string>
#include <stack>
#include <map>
#include <deque>
#include <vector>
#include <set>
#include <algorithm>
#include <list>
#include <assert.h>
#include <iostream>

using namespace std;

class VON_RegEx
{
protected:
	//! State Class
	class VON_State
	{
	protected:
		//! Transitions from this state to other 
		multimap<char, VON_State*> m_Transition;

		//! State ID
		int m_nStateID;

		//! Set of NFA state from which this state is constructed
		set<VON_State*> m_NFAStates;

	public:
		//! Default constructor
		VON_State() : m_nStateID(-1), m_bAcceptingState(false) {};

		//! parameterized constructor
		VON_State(int nID) : m_nStateID(nID), m_bAcceptingState(false) {};

		//! Constructs new state from the set of other states
		/*! This is necessary for subset construction algorithm
			because there a new DFA state is constructed from 
			one or more NFA states
		*/
		VON_State(set<VON_State*> NFAState, int nID)
		{
			m_NFAStates			= NFAState;
			m_nStateID			= nID;
			
			// DFA state is accepting state if it is constructed from 
			// an accepting NFA state
			m_bAcceptingState	= false;
			for(auto iter=NFAState.begin(); iter!=NFAState.end(); ++iter)
				if((*iter)->m_bAcceptingState)
					m_bAcceptingState = true;
		};

		//! Copy Constructor
		VON_State(const VON_State &other)
		{ *this = other; };

		//! Destructor
		virtual ~VON_State() {};

		//! True if this state is accepting state
		bool m_bAcceptingState;

		//! Adds a transition from this state to the other
		void AddTransition(char chInput, VON_State *pState)
		{
			if(pState != NULL){
				m_Transition.insert(make_pair(chInput, pState));
			}
		};

		//! Removes all transition that go from this state to pState
		void RemoveTransition(VON_State* pState)
		{
			multimap<char, VON_State*>::iterator iter;
			for(iter=m_Transition.begin(); iter!=m_Transition.end();)
			{
				VON_State *toState = iter->second;
				if(toState == pState)
					m_Transition.erase(iter++);
				else ++iter;
			}
		};

		//! Returns all transitions from this state on specific input
		void GetTransition(char chInput, vector<VON_State*> &States)
		{
			// clear the array first
			States.clear();

			// Iterate through all values with the key chInput
			multimap<char, VON_State*>::iterator iter;
			for(iter = m_Transition.lower_bound(chInput);
				iter!= m_Transition.upper_bound(chInput);
				++iter)
				{
					VON_State *pState = iter->second;
					if(pState != NULL){
						States.push_back(pState);
					}
					else return;
				}
		};

		static string convertInttoStr(int n){
			string ans;
			while(n){
				ans+=n%10+'0';
				n/=10;
			}
			reverse(ans.begin(),ans.end());
			return ans;
		}
		string GetStateID()
		{
			return convertInttoStr(m_nStateID);
		};

		/*! Returns the set of NFA states from 
			which this DFA state was constructed
		*/
		set<VON_State*>& GetNFAState()
		{ return m_NFAStates; };

		//! Returns true if this state is dead end
		/*! By dead end I mean that this state is not
			accepting state and there are no transitions
			leading away from this state. This function
			is used for reducing the DFA.
		*/
		bool IsDeadEnd()
		{
			if(m_bAcceptingState)
				return false;
			if(m_Transition.empty())
				return true;
			
			for(auto iter=m_Transition.begin(); iter!=m_Transition.end(); ++iter)
			{
				VON_State *toState = iter->second;
				if(toState != this)
					return false;
			}
			return true;
		};	

		//! Override the assignment operator
		VON_State operator=(const VON_State& other)
		{ 
			this->m_Transition	= other.m_Transition;
			this->m_nStateID		= other.m_nStateID;
			this->m_NFAStates		= other.m_NFAStates;
            return *this;
		};

		//! Override the comparison operator
		bool operator==(const VON_State& other)
		{
			if(m_NFAStates.empty())
				return(m_nStateID == other.m_nStateID);
			else return(m_NFAStates == other.m_NFAStates);
		};
	};
    public:
        void show();

    private:
		//! Structure to track the pattern recognition
	class VON_PatternState
	{
	public:
		//! Default Constructor
		VON_PatternState() : m_pState(NULL), m_nStartIndex(-1) {};

		//! Copy Constructor
		VON_PatternState(const VON_PatternState &other)
		{ *this = other; };

		//! Destructor
		virtual ~VON_PatternState() {};

		//! Pointer to current state in DFA
		VON_State* m_pState;

		//! 0-Based index of the starting position
		int m_nStartIndex;

		//! Override the assignment operator
		VON_PatternState& operator=(const VON_PatternState& other)
		{
			assert(other.m_pState != NULL);
			m_pState		= other.m_pState;
			m_nStartIndex	= other.m_nStartIndex;
			return *this;
		};
	};

	//! NFA Table
	typedef deque<VON_State*> FSA_TABLE;

	/*! NFA Table is stored in a deque of CAG_States.
		Each CAG_State object has a multimap of 
		transitions where the key is the input
		character and values are the references to
		states to which it transfers.
	*/
	FSA_TABLE m_NFATable;

	//! DFA table is stores in same format as NFA table
	FSA_TABLE m_DFATable;

	//! Operand Stack
	/*! If we use the Thompson's Algorithm then we realize
		that each operand is a NFA automata on its own!
	*/
	stack<FSA_TABLE> m_OperandStack;

	//! Operator Stack
	/*! Operators are simple characters like "*" & "|" */
	stack<char> m_OperatorStack;

	//! Keeps track of state IDs
	int m_nNextStateID;

	//! Set of input characters
	set<char> m_InputSet;

	//! List of current partially found patterns
	list<VON_PatternState*> m_PatternList;

	//! Contains the current patterns accessed
	int m_nPatternIndex;

	//! Contains the text to check
	string m_strText;

	//! Contains all found patterns
	vector<string> m_vecPattern;

	//! Contains all position where these pattern start in the text
	vector<int> m_vecPos;

	//! Constructs basic Thompson Construction Element
	/*! Constructs basic NFA for single character and
		pushes it onto the stack.
	*/
	void Push(char chInput);

	//! Pops an element from the operand stack
	/*! The return value is TRUE if an element 
		was poped successfully, otherwise it is
		FALSE (syntax error) 
	*/
	bool Pop(FSA_TABLE &NFATable);

	//! Checks is a specific character and operator
	bool IsOperator(char ch) { return((ch == 42) || (ch == 124) || (ch == 40) || (ch == 41) || (ch == 8)); };

	//! Returns operator presedence
	/*! Returns TRUE if presedence of opLeft <= opRight.
	
			Kleens Closure	- highest
			Concatenation	- middle
			Union			- lowest
	*/
	bool Presedence(char opLeft, char opRight)
	{
		if(opLeft == opRight)
			return true;

		if(opLeft == '*')
			return false;

		if(opRight == '*')
			return true;

		if(opLeft == 8)
			return false;

		if(opRight == 8)
			return true;

		if(opLeft == '|')
			return false;
		
		return true;
	};

	//! Checks if the specific character is input character
	bool IsInput(char ch) { return(!IsOperator(ch)); };

	//! Checks is a character left parantheses
	bool IsLeftParanthesis(char ch) { return(ch == 40); };

	//! Checks is a character right parantheses
	bool IsRightParanthesis(char ch) { return(ch == 41); };

	//! Evaluates the next operator from the operator stack
	bool Eval();

	//! Evaluates the concatenation operator
	/*! This function pops two operands from the stack 
		and evaluates the concatenation on them, pushing
		the result back on the stack.
	*/
	bool Concat();

	//! Evaluates the Kleen's closure - star operator
	/*! Pops one operator from the stack and evaluates
		the star operator on it. It pushes the result
		on the operand stack again.
	*/
	bool Star();

	//! Evaluates the union operator
	/*! Pops 2 operands from the stack and evaluates
		the union operator pushing the result on the
		operand stack.
	*/
	bool Union();

	//! Inserts char 8 where the concatenation needs to occur
	/*! The method used to parse regular expression here is 
		similar to method of evaluating the arithmetic expressions.
		A difference here is that each arithmetic expression is 
		denoted by a sign. In regular expressions the concatenation
		is not denoted by ny sign so I will detect concatenation
		and insert a character 8 between operands.
	*/
	string ConcatExpand(string strRegEx);

	//! Creates Nondeterministic Finite Automata from a Regular Expression
	bool CreateNFA(string strRegEx);

	//! Calculates the Epsilon Closure 
	/*! Returns epsilon closure of all states
		given with the parameter.
	*/
	void EpsilonClosure(set<VON_State*> T, set<VON_State*> &Res); 

	//! Calculates all transitions on specific input char
	/*! Returns all states rechible from this set of states
		on an input character.
 	*/
	void Move(char chInput, set<VON_State*> T, set<VON_State*> &Res);

	//! Converts NFA to DFA using the Subset Construction Algorithm
	void ConvertNFAtoDFA();

	//! Optimizes the DFA
	/*! This function scanns DFA and checks for states that are not
		accepting states and there is no transition from that state
		to any other state. Then after deleting this state we need to
		go through the DFA and delete all transitions from other states
		to this one.
	*/
	void ReduceDFA();

	//! Cleans up the memory
	void CleanUp()
	{
		// Clean up all allocated memory for NFA
		for(int i=0; i<m_NFATable.size(); ++i)
			delete m_NFATable[i];
		m_NFATable.clear();

		// Clean up all allocated memory for DFA
		for(int i=0; i<m_DFATable.size(); ++i)
			delete m_DFATable[i];
		m_DFATable.clear();

		// Reset the id tracking
		m_nNextStateID = 0;

		// Clear both stacks
		while(!m_OperandStack.empty())
			m_OperandStack.pop();
		while(!m_OperatorStack.empty())
			m_OperatorStack.pop();

		// Clean up the Input Character set
		m_InputSet.clear();

		// Clean up all pattern states
		for(auto iter=m_PatternList.begin(); iter!=m_PatternList.end(); ++iter)
			delete *iter;
		m_PatternList.clear();
	};

	//! Searches the text for all occurences of the patterns
	/*! Returns TRUE if single pattern is found or FALSE if 
	    no patterns is found.
	*/
	bool Find();

public:
	//! Default Constructor
	VON_RegEx();

	//! Destructor
	virtual ~VON_RegEx();

	//! Set Regular Expression
	/*! Set the string pattern to be searched for.
		Returns TRUE if success othewise it returns FALSE.
	*/
	bool SetRegEx(string strRegEx);

	//! Searches for the first occurence of a text pattern
	/*! If the pattern is found the function returns TRUE
		together with starting positions (vecPos) and the patterns
		(vecPattern) found. THIS FUNCTION MUST BE CALLED FIRST.
		The return value is TRUE if a pattern is found or FALSE
		if nothing is found.
	*/
	bool FindFirst(string strText, int &nPos, string &strPattern);

	//! Searches for the next occurence of a text pattern
	/*! If the pattern is found the function returns TRUE
		together with starting position array (vecPos) and the patterns
		(vecPattern) found. THIS FUNCTION MUST BE CALLED AFTER 
		THE FUNCTION FindFirst(...). 
		The return value is TRUE if a pattern is found or FALSE
		if nothing is found.
	*/
	bool FindNext(int &nPos, string &strPattern);

#ifdef _DEBUG
	//! For Debuging purposes only
	void SaveNFATable()
	{
		string strNFATable;

		// First line are input characters
		for(auto iter = m_InputSet.begin(); iter != m_InputSet.end(); ++iter)
			strNFATable += "\t\t" + *iter;
		// add epsilon
		strNFATable += "\t\tepsilon";
		strNFATable += "\n";

		// Now go through each state and record the transitions
		for(int i=0; i<m_NFATable.size(); ++i)
		{
			VON_State *pState = m_NFATable[i];
			
			// Save the state id
			strNFATable += pState->GetStateID();

			// now write all transitions for each character
			vector<VON_State*> State;
			for(auto iter = m_InputSet.begin(); iter != m_InputSet.end(); ++iter)
			{
				pState->GetTransition(*iter, State);
				string strState;
				if(State.size()>0)
				{
					strState = State[0]->GetStateID();
					for(int i=1; i<State.size(); ++i)
						strState += "," + State[i]->GetStateID();
				}
				strNFATable += "\t\t" + strState;
			}

			// Add all epsilon transitions
			pState->GetTransition(0, State);
			string strState;
			if(State.size()>0)
			{
				strState = State[0]->GetStateID();
				for(int i=1; i<State.size(); ++i)
					strState += "," + State[i]->GetStateID();
			}
			strNFATable += "\t\t" + strState + "\n";
		}
	};

	//! For Debuging purposes only
	void SaveDFATable()
	{
		string strDFATable;

		// First line are input characters

		for(auto iter = m_InputSet.begin(); iter != m_InputSet.end(); ++iter)
			strDFATable += "\t\t" + *iter;
		strDFATable += "\n";

		// Now go through each state and record the transitions
		for(int i=0; i<m_DFATable.size(); ++i)
		{
			VON_State *pState = m_DFATable[i];
			
			// Save the state id
			strDFATable += pState->GetStateID();

			// now write all transitions for each character
			vector<VON_State*> State;
			for(auto iter = m_InputSet.begin(); iter != m_InputSet.end(); ++iter)
			{
				pState->GetTransition(*iter, State);
				string strState;
				if(State.size()>0)
				{
					strState = State[0]->GetStateID();
					for(int i=1; i<State.size(); ++i)
						strState += "," + State[i]->GetStateID();
				}
				strDFATable += "\t\t" + strState;
			}
			strDFATable += "\n";
		}
	};

	void SaveNFAGraph() 
	{
		string strNFAGraph = "digraph{\n";

		// Final states are double circled
		for(int i=0; i<m_NFATable.size(); ++i)
		{
			VON_State *pState = m_NFATable[i];
			if(pState->m_bAcceptingState)
			{
				string strStateID = pState->GetStateID();
				strNFAGraph += "\t" + strStateID + "\t[shape=doublecircle];\n";
			}
		}
		
		strNFAGraph += "\n";

		// Record transitions
		for(int i=0; i<m_NFATable.size(); ++i)
		{
			VON_State *pState = m_NFATable[i];
			vector<VON_State*> State;

			pState->GetTransition(0, State);
			for(int j=0; j<State.size(); ++j)
			{
				// Record transition
				string strStateID1 = pState->GetStateID();
				string strStateID2 = State[j]->GetStateID();
				strNFAGraph += "\t" + strStateID1 + " -> " + strStateID2;
				strNFAGraph += "\t[label=\"epsilon\"];\n";
			}

			for(auto iter = m_InputSet.begin(); iter != m_InputSet.end(); ++iter)
			{
				pState->GetTransition(*iter, State);
				for(int j=0; j<State.size(); ++j)
				{
					// Record transition
					string strStateID1 = pState->GetStateID();
					string strStateID2 = State[j]->GetStateID();
					strNFAGraph += "\t" + strStateID1 + " -> " + strStateID2;
					strNFAGraph += "\t[label=\"" + *iter + "\"];\n";
				}
			}
		}

		strNFAGraph += "}";
	};

	
	void SaveDFAGraph()
	{
		string strDFAGraph = "digraph{\n";

		// Final states are double circled
		for(int i=0; i<m_DFATable.size(); ++i)
		{
			VON_State *pState = m_DFATable[i];
			if(pState->m_bAcceptingState)
			{
				string strStateID = pState->GetStateID();
				strDFAGraph += "\t" + strStateID + "\t[shape=doublecircle];\n";
			}
		}
		
		strDFAGraph += "\n";

		// Record transitions
		for(int i=0; i<m_DFATable.size(); ++i)
		{
			VON_State *pState = m_DFATable[i];
			vector<VON_State*> State;

			pState->GetTransition(0, State);
			for(int j=0; j<State.size(); ++j)
			{
				// Record transition
				string strStateID1 = pState->GetStateID();
				string strStateID2 = State[j]->GetStateID();
				strDFAGraph += "\t" + strStateID1 + " -> " + strStateID2;
				strDFAGraph += "\t[label=\"epsilon\"];\n";
			}

			for(auto iter = m_InputSet.begin(); iter != m_InputSet.end(); ++iter)
			{
				pState->GetTransition(*iter, State);
				for(int j=0; j<State.size(); ++j)
				{
					// Record transition
					string strStateID1 = pState->GetStateID();
					string strStateID2 = State[j]->GetStateID();
					strDFAGraph += "\t" + strStateID1 + " -> " + strStateID2;
					strDFAGraph += "\t[label=\"" + *iter + "\"];\n";
				}
			}
		}

		strDFAGraph += "}";
	};
#else
	void SaveNFATable() { } ;
	void SaveDFATable() { } ;
	void SaveNFAGraph() { } ;
	void SaveDFAGraph() { } ;
#endif
};

