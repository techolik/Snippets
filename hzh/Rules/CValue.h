#pragma once

#include <vector>
#include <string>

using namespace std;

namespace Json{
	class Value;
}

class CValue
{
public:
	enum Type{
		eNone = 0,
		eBool,
		eInt,
		eUInt,
		eInt64,
		eUInt64,
		eDouble,
		eString,
		eIntVec,
		eStringVec
	};

	CValue()						{ Init(); _type = eNone; }
	CValue(bool b)					{ _value.bValue = b;	_type = eBool; }
	CValue(int n)					{ _value.n64Value = n; _value.nValue = n;	_type = eInt; }
	CValue(unsigned int u)			{ _value.u64Value = u; _value.uValue = u; 	_type = eUInt; }
	CValue(__int64 n64)				{ _value.n64Value = n64; _type = eInt64; }
	CValue(unsigned __int64 u64)	{ _value.u64Value = u64; _type = eUInt64; }
	CValue(double d)				{ _value.dValue = d;	_type = eDouble; }
	CValue(const string& s);		// stores and processes in lower case
	CValue(const vector<int>& ns)	{ Init(); _ints = ns; _type = eIntVec; }
	CValue(const vector<string>& ss);// stores and processes in lower case

	Type				type() const		{ return _type; }
	bool				asBool() const		{ return _value.bValue; }
	int					asInt() const		{ return _value.nValue; }
	unsigned int		asUInt() const		{ return _value.uValue; }
	__int64				asInt64() const		{ return _value.n64Value; }
	unsigned __int64	asUInt64() const	{ return _value.u64Value; }
	double				asDouble() const	{ return _value.dValue; }
	const string&		asString() const	{ return _strValue; }
	const vector<int>&	asIntVec() const	{ return _ints; }
	const vector<string>& asStringVec() const{ return _strings; }

	string				toString() const;
	static CValue		fromString(const string& rawStr);
	static CValue		fromJsonValue(const Json::Value& jv);

	bool				operator < (const CValue& rv);
	bool				operator > (const CValue& rv);
	bool				operator == (const CValue& rv);

private:
	void Init()	{ _value.n64Value = 0; }
	union {
		bool				bValue;
		int					nValue;
		unsigned int		uValue;
		__int64				n64Value;
		unsigned __int64	u64Value;
		double				dValue;
	}		_value;
	Type			_type;
	string			_strValue;
	vector<int>		_ints;
	vector<string>	_strings;
};
