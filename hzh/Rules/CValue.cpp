#include "CValue.h"

#include "..\CJsonLib.h"
#include "..\CStringUtil.h"

CValue::CValue(const string& s)
{
	Init();
	_strValue = s;
	CStringUtil::ToLowerUtf8(_strValue);
	_type = eString;
}

CValue::CValue(const vector<string>& ss)
{
	Init();
	_strings = ss;
	for (string& s : _strings)
		std::transform(s.begin(), s.end(), s.begin(), ::tolower);
	_type = eStringVec;
}

CValue CValue::fromJsonValue(const Json::Value& jv)
{
	switch (jv.type())
	{
		case Json::booleanValue:
			return CValue(jv.asBool());
		case Json::intValue:
			return CValue(jv.asInt());
		case Json::uintValue:
			return CValue(jv.asUInt());
		case Json::realValue:
			return CValue(jv.asDouble());
		case Json::stringValue:
			return CValue(jv.asString());
		default:
		{
			if (jv.isInt64())
				return CValue(jv.asInt64());
			if (jv.isUInt64())
				return CValue(jv.asUInt64());
			if (jv.isArray() && jv.size() > 0)
			{
				auto j0 = jv[0];
				if (/*j0.isInt() || */j0.isUInt()){
					vector<int> ints;
					for (const auto& ji : jv)
						ints.push_back(ji.asUInt());
					return CValue(ints);
				}
				else if (j0.isString())	{
					vector<string> strings;
					for (const auto& ji : jv)
						strings.push_back(ji.asString());
					return CValue(strings);
				}
			}
			return CValue();
		}
	}
}

CValue CValue::fromString(const string& rawStr)
{
	try{
		// Test for int, if it's cut off, then go for double.
		size_t pos = string::npos;
		int x = stoi(rawStr, &pos);
		if (pos == rawStr.size())
			return CValue(x);
		else
			return CValue(stod(rawStr));
	}
	catch (...){
		// If int fails, try long long (int64), and fall back to raw string
		try{
			return CValue(std::stoll(rawStr));
		}
		catch (...){
			return CValue(CStringUtil::Strip(rawStr));
		}
	}
}

string CValue::toString() const
{
	switch (type())
	{
		case eBool:
			return asBool() ? "true" : "false";
		case eInt:
			return to_string(asInt());
		case eUInt:
			return to_string(asUInt());
		case eInt64:
			return to_string(asInt64());
		case eUInt64:
			return to_string(asUInt64());
		case eDouble:
			return to_string(asDouble());
		case eString:
			return _strValue;
		case eIntVec:
			return CStringUtil::ToJsonArrayString(_ints);
		case eStringVec:
			return CStringUtil::ToJsonArrayString(_strings);
		default:
			return "";
	}
}

bool CValue::operator < (const CValue& rv)
{
	// TODO: miss-match among int/uint/int64/uint64

	//if (type() != rv.type())
	//	return false;

	if (type() == eBool)
		return asBool() < rv.asBool();
	if (type() == eInt)
		return asInt() < rv.asInt();
	if (type() == eUInt)
		return asUInt() < rv.asUInt();
	if (type() == eInt64)
		return asInt64() < rv.asInt64();
	if (type() == eUInt64)
		return asUInt64() < rv.asUInt64();
	if (type() == eDouble)
		return asDouble() < rv.asDouble();
	if (type() == eString)
		return asString() < rv.asString();
	if (type() == eIntVec)
		return asIntVec() < rv.asIntVec();
	if (type() == eStringVec)
		return asStringVec() < rv.asStringVec();

	return false;
}

bool CValue::operator > (const CValue& rv)
{
	return !(*this < rv || *this == rv);
}

bool CValue::operator == (const CValue& rv)
{
	// TODO: miss-match among int/uint/int64/uint64

	//if (type() != rv.type())
	//	return false;

	if (type() == eBool)
		return asBool() == rv.asBool();
	if (type() == eInt)
		return asInt() == rv.asInt();
	if (type() == eUInt)
		return asUInt() == rv.asUInt();
	if (type() == eInt64)
		return asInt64() == rv.asInt64();
	if (type() == eUInt64)
		return asUInt64() == rv.asUInt64();
	if (type() == eDouble)
		return asDouble() == rv.asDouble();
	if (type() == eString)
		return asString() == rv.asString();
	if (type() == eIntVec)
		return asIntVec() == rv.asIntVec();
	if (type() == eStringVec)
		return asStringVec() == rv.asStringVec();

	return false;
}
