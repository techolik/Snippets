#pragma once

#include "CValue.h"

#include <set>
#include <vector>
#include <string>
#include <regex>

using namespace std;

class CRule;
class CRConfig;
class CRuleVec;
class CMonEvent;
class CRuleManger;

//
// "owner.path $end [\".ttf\",\".exe\",\".dll\"]" is represented by
// CCondition, a rule can have a tree of conditions.
//
class CCondition
{
	friend class CRule;
	CCondition() : 
		bNeg(false)
	{}

	enum Verb{
		eNone = 0,
		eIn,
		eNotIn,
		eGreaterThan,
		eGreaterThanOrEquals,
		eLessThan,
		eLessThanOrEquals,
		eEquals,
		eNotEquals,
		eMatches,
		eStartsWith,
		eEndsWith
	};

	bool	Load(const string& con);
	bool	LoadVerb(const string& sv);
	bool	LoadValue(const string& sval);
	bool	Satisfy(CMonEvent* pEvent);

	bool	bNeg;
	string	typeId;
	string	field;
	Verb	verb;
	CValue	value;
	regex	reg;
};

//
// Build a boolean evaluation tree to make sure not effort is
// wasted during the filtering.
//
struct BoolEvalTree
{
	// true for 'and', false for 'or'
	bool			bOp;

	// A condition is only available on a leaf node,
	// in which case both children nodes will be NULL
	CCondition*		pCondition;
	BoolEvalTree*	pLeft;
	BoolEvalTree*	pRight;

	BoolEvalTree()
		: bOp(false), pCondition(NULL), pLeft(NULL), pRight(NULL)
	{}
};

//
// CRule represents a rule like this,
//	{
//		"k": "owner.path $end [\".ttf\",\".exe\",\".dll\"]",
//		"t": "rule",
//		"v": ["pid", "path"]
//	},
//
class CRule{
public:
	~CRule();

private:
	friend class CRuleSet;
	friend class CRuleManager;
	bool				Parse(string& strRule);
	bool				Filter(CMonEvent* pEvent);
	bool				Evaluate(BoolEvalTree* pEvalTree, CMonEvent* pEvent);

	void				DeleteTree(BoolEvalTree*		pEvalTree);

	BoolEvalTree*		pEvalTree;

	string				typeId;
	string				raw;

	// The expected resulting fields of an event if it satisfies the condition(s)
	// if empty, the event will be ignored
	set<string>			fields;
};

//
// Simple wrawpper of rules such that one event is only checked against
// one set of rules
//
class CRuleSet
{
	friend class CRuleManager;
public:
	CRuleSet();
	CRuleSet(const CRuleSet&) = delete;
	CRuleSet& operator = (const CRuleSet&) = delete;
	CRuleSet(CRuleSet&& r);
	~CRuleSet();
	bool				Filter(CMonEvent* pEvent) const;

	string				typeId;
	vector<CRule*>		m_rules;
	vector<CRConfig*>	m_configs;
};
