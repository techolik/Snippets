#include "Rules.h"

#include "CMonEvent.h"
#include "CJsonLib.h"
#include "CStlSugars.h"
#include "CStringUtil.h"
#include "CRuleManager.h"

#include <algorithm>

bool CCondition::LoadVerb(const string& sv)
{
	if (sv == "in")
		verb = eIn;
	else if (sv == "nin")
		verb = eNotIn;
	else if (sv == "gt")
		verb = eGreaterThan;
	else if (sv == "gte")
		verb = eGreaterThanOrEquals;
	else if (sv == "lt")
		verb = eLessThan;
	else if (sv == "lte")
		verb = eLessThanOrEquals;
	else if (sv == "eq")
		verb = eEquals;
	else if (sv == "neq")
		verb = eNotEquals;
	else if (sv == "start")
		verb = eStartsWith;
	else if (sv == "end")
		verb = eEndsWith;
	else if (sv == "match")
		verb = eMatches;
	else
		return false;
	return true;
}

bool CCondition::LoadValue(const string& sval)
{
	switch (verb)
	{
		case eGreaterThan:
		case eGreaterThanOrEquals:
		case eLessThan :
		case eLessThanOrEquals:
		case eEquals:
		case eNotEquals:
			value = CValue::fromString(sval);
			return true;
		case eStartsWith:
		case eEndsWith:
		{
			string temp = sval;
			CStringUtil::Strip(temp);
			vector<string> splitted;
			CStringUtil::Split(temp, ',', splitted);
			if (splitted.empty()){
				// Single string
				value = CValue(temp);
			}
			else{
				// Vector of strings
				for (string& sp : splitted)
					CStringUtil::Strip(sp);
				value = CValue(splitted);
			}
			return true;
		}
		case eIn:
		case eNotIn:
		{
			string temp = sval;
			CStringUtil::Strip(temp);
			vector<string> splitted;
			CStringUtil::Split(temp, ',', splitted);
			for (string& sp : splitted)
				CStringUtil::Strip(sp);
			value = CValue(splitted);
			return true;
		}
		case eMatches:
			reg.assign(CStringUtil::Strip(sval), 
				regex_constants::icase | regex_constants::ECMAScript);
			return true;
		default:
			value = CValue(sval);
			return true;
	}
}

bool CCondition::Load(const string& con)
{
	// If there's no space then the whole expression is a typeId
	size_t pos = con.find(' ');
	if (pos == string::npos){
		typeId = con;
		return true;
	}

	// Otherwise break it into two parts
	typeId = con.substr(0, pos);
	string remainder = con.substr(pos + 1);

	// Split off the typeId and field
	pos = typeId.find('.');
	if (pos == string::npos)
		return false;
	field = typeId.substr(pos + 1);
	typeId = typeId.substr(0, pos);

	// Then split off the verb
	CStringUtil::Trim(remainder);
	pos = remainder.find(' ');
	if (pos == string::npos)
		return false;
	string strVerb = remainder.substr(0, pos);
	CStringUtil::Trim(strVerb);
	if (!LoadVerb(strVerb))
		return false;

	// Then value
	string strValue = remainder.substr(pos + 1);
	if (strValue.empty())
		return false;

	CStringUtil::Trim(strValue);
	return LoadValue(strValue);
}

bool CCondition::Satisfy(CMonEvent* pEvent)
{
	if (field.empty())
		return true;

	CValue val = pEvent->GetValue(field);
	if (val.type() == CValue::eNone)
		// TODO ?
		return false;

	bool bSatisfy = false;
	switch (verb)
	{
		case CCondition::eIn:
			if (val.type() == CValue::eInt ||
				val.type() == CValue::eUInt)
			{
				bSatisfy = CStlSugars::contains(value.asIntVec(), val.asInt());
			}
			else if (val.type() == CValue::eString)
			{
				bSatisfy = CStlSugars::contains(value.asStringVec(), val.asString());
			}
			else
				bSatisfy = false;
			break;
		case CCondition::eNotIn:
			if (val.type() == CValue::eInt ||
				val.type() == CValue::eUInt)
			{
				bSatisfy = !CStlSugars::contains(value.asIntVec(), val.asInt());
			}
			else if (val.type() == CValue::eString)
			{
				bSatisfy = !CStlSugars::contains(value.asStringVec(), val.asString());
			}
			else
				bSatisfy = false;
			break;
		case CCondition::eGreaterThan:
			bSatisfy = val > value;
			break;
		case CCondition::eGreaterThanOrEquals:
			bSatisfy = !(val < value);
			break;
		case CCondition::eLessThan:
			bSatisfy = val < value;
			break;
		case CCondition::eLessThanOrEquals:
			bSatisfy = !(val > value);
			break;
		case CCondition::eEquals:
			bSatisfy = val == value;
			break;
		case CCondition::eNotEquals:
			bSatisfy = !(val == value);
			break;
		case CCondition::eMatches:
			bSatisfy = regex_match(val.asString(), reg);
			break;
		case CCondition::eStartsWith:
			if (val.type() == CValue::eString)
			{
				if (value.type() == CValue::eString)
				{
					bSatisfy = CStlSugars::startsWith(val.asString(), value.asString());
				}
				else if (value.type() == CValue::eStringVec)
				{
					for (const string& v : value.asStringVec())
					{
						if (CStlSugars::startsWith(val.asString(), v))
						{
							bSatisfy = true;
							break;
						}
					}
				}
			}
			break;
		case CCondition::eEndsWith:
			if (val.type() == CValue::eString)
			{
				if (value.type() == CValue::eString)
				{
					bSatisfy = CStlSugars::endsWith(val.asString(), value.asString());
				}
				else if (value.type() == CValue::eStringVec)
				{
					for (const string& v : value.asStringVec())
					{
						if (CStlSugars::endsWith(val.asString(), v))
						{
							bSatisfy = true;
							break;
						}
					}
				}
			}
			break;
		default:
			break;
	}
	return bNeg ? !bSatisfy : bSatisfy;
}

CRule::~CRule()
{
	DeleteTree(pEvalTree);
}

void CRule::DeleteTree(BoolEvalTree* pEvalTree)
{
	if (pEvalTree){
		DeleteTree(pEvalTree->pLeft);
		DeleteTree(pEvalTree->pRight);
		if (pEvalTree->pCondition)
			delete pEvalTree->pCondition;
		delete pEvalTree;
	}
}

bool CRule::Parse(string& strRule)
{
	//
	// A simple logic expression parser, supports 'and', 'or' with '(' and ')',
	// limited support for 'not' - only before an expression that's not '()'d
	//

	// Logical operators
	enum EOp{
		eNone = 0,
		eAnd,
		eOr
	};

	stack<EOp> stkOp;
	stack<BoolEvalTree*> stkExp;

	// Expressions are made up of words, words are speparated by space or '(', ')'
	string expression;
	string word;

	// Whether there is a 'not'. No stack for this because it will bind to the
	// next expression.
	bool bNeg = false;

	// Lambdas that capture scoped variables work much like member functions that
	// use member variables, but without the need to create a separate class.
	auto MakeLeaf = [&expression, &bNeg, &stkExp]()
	{
		// Trim to make sure we have a valid expression
		CStringUtil::Trim(expression);
		if (!expression.empty())
		{
			BoolEvalTree* pLeaf = new BoolEvalTree;
			pLeaf->pCondition = new CCondition;
			if (!pLeaf->pCondition->Load(expression))
				return false;
			expression.clear();

			pLeaf->pCondition->bNeg = bNeg;
			bNeg = false;
			stkExp.push(pLeaf);
		}
		return true;
	};

	auto MakeBranch = [&stkExp, &stkOp]()
	{
		if (stkExp.size() < 2)
			return false;
		if (stkOp.empty())
			return false;

		BoolEvalTree* pNode = new BoolEvalTree;
		pNode->bOp = stkOp.top() == eAnd;
		// Remove used op
		stkOp.pop();

		// Top of the stack as the right child, then the next
		pNode->pRight = stkExp.top();
		stkExp.pop();
		pNode->pLeft = stkExp.top();
		stkExp.pop();
		stkExp.push(pNode);

		return true;
	};

	size_t i = 0;
	strRule += ' '; // add a space to ease parsing
	while (i < strRule.size())
	{
		char c = strRule[i++];
		switch (c)
		{
			case '(':
				stkOp.push(eNone);
				break;
			case ')':
				// There might be no space before ')' so need to close the expression
				if (!expression.empty())
					expression += ' ';
				expression += word;
				word.clear();

				if (!MakeLeaf())
					goto cleanup;
				while (!stkOp.empty() && stkOp.top() != eNone)
					if (!MakeBranch())
						goto cleanup;
				if (!stkOp.empty() && stkOp.top() == eNone)
					stkOp.pop();
				break;
			case ' ':
				if (word == "not"){
					// not x
					bNeg = true;
				}
				else if (word == "and"){
					if (!MakeLeaf())
						goto cleanup;
					if (!stkOp.empty() && (stkOp.top() != eNone))
						if (stkOp.top() == eAnd)
							// In case of "x and y and z", make a branch for "x and y"
							if (!MakeBranch())
								goto cleanup;
					stkOp.push(eAnd);
				}
				else if (word == "or"){
					if (!MakeLeaf())
						goto cleanup;
					if (!stkOp.empty() && (stkOp.top() != eNone))
						if (!MakeBranch())
							goto cleanup;
					stkOp.push(eOr);
				}
				else{
					if (!expression.empty())
						expression += ' ';
					expression += word;
				}
				word.clear();
				break;
			default:
				word += c;
				break;
		}
	}

	// We might still have a leaf to make
	if (!MakeLeaf())
		goto cleanup;
	
	// At this point the remaining of the stack is in increasing order of precedence
	// so walk down and make branches.
	while (stkExp.size() > 1){
		if (stkOp.empty() || stkOp.top() == eNone)
			goto cleanup;
		if (!MakeBranch())
			goto cleanup;
	}

	// We should now have the root of the tree as the only node in the stack.
	if (stkExp.size() == 1){
		pEvalTree = stkExp.top();
		stkExp.pop();

		return true;
	}

cleanup:
	while (!stkExp.empty()){
		DeleteTree(stkExp.top());
		stkExp.pop();
	}
	return false;
}

bool CRule::Evaluate(BoolEvalTree* pEvalTree, CMonEvent* pEvent)
{
	if (pEvalTree){
		if (pEvalTree->pCondition)
			return pEvalTree->pCondition->Satisfy(pEvent);
		else{
			// Logical minimum evaluate
			if (bool bLeft = Evaluate(pEvalTree->pLeft, pEvent)){
				if (pEvalTree->bOp == false)
					return true;
				if (pEvalTree->bOp)
					return Evaluate(pEvalTree->pRight, pEvent);
			}
			else{
				if (pEvalTree->bOp)
					return false;
				else
					return Evaluate(pEvalTree->pRight, pEvent);
			}
		}
	}
	return false;
}

bool CRule::Filter(CMonEvent* pEvent)
{
	return Evaluate(pEvalTree, pEvent);
}

CRuleSet::CRuleSet()
{}

CRuleSet::CRuleSet(CRuleSet&& r)
{
	typeId = r.typeId;
	m_rules = r.m_rules;
	m_configs = r.m_configs;
	r.typeId.clear();
	r.m_rules.clear();
	r.m_configs.clear();
}

CRuleSet::~CRuleSet()
{
	for (auto pC : m_configs)
		delete pC;
	for (auto pR : m_rules)
		delete pR;
}

bool CRuleSet::Filter(CMonEvent* pEvent) const
{
	if (m_rules.empty())
		// TODO ?
		return true;

	// Find the first satisfying rule
	set<string> fields;
	size_t i = 0;
	for (; i < m_rules.size(); i++)
	{
		if (m_rules[i]->Filter(pEvent)){
			fields = m_rules[i]->fields;
			i++;
			break;
		}
	}

	// If there's one satisfying rule and it says empty fields, or
	// there's no satisfying rule at all, then we're done.
	if (fields.empty())
		return false;

	// Find subsequent satisfying rules and reduce the fields.
	for (; i < m_rules.size(); i++)
	{
		if (m_rules[i]->Filter(pEvent))
		{
			set<string> resultFields;
			set_intersection(
				fields.cbegin(), fields.cend(),
				m_rules[i]->fields.cbegin(), m_rules[i]->fields.cend(),
				inserter(resultFields, resultFields.end())
				);

			// After set_intersection, resultFields = fields & m_rules[i]->fields
			fields.swap(resultFields);

			// Give up as soon as possible
			if (fields.empty())
				return false;
		}
	}
	if (fields.empty())
		return false;

	pEvent->FillFields(vector<string>(fields.begin(), fields.end()));
	return true;
}
