#include "CMonEvent.h"

#include "CTime.h"
#include "CRuleManager.h"
#include "CStringUtil.h"
#include "MessageDefines.h"
#include "..\CJsonLib.h"

CMonEvent::CMonEvent(const string& st)
	: m_strTypeId(st)
{

}

CMonEvent::~CMonEvent()
{

}

CValue CMonEvent::GetValue(const string& key)
{
	if (!_hasValue(key))
		m_map[key] = RetrieveValue(key);
	
	return m_map[key];
}

bool CMonEvent::FillFields(const vector<string>& keys)
{
	for (const string& key : keys)
	{
		// TODO: may need to drop the keys in the map that are not in the requested vector.
		// those are usually cached by call to GetValue().
		if (!_hasValue(key))
			m_map[key] = RetrieveValue(key);
	}

	// TODO: error handling
	return true;
}

string CMonEvent::ToJsonString() const
{
	CJsonBuild json;
	for (const auto& it : m_map)
	{
		json.Add(it.first.c_str(), it.second.toString());
	}
	return json.WriteString();
}

bool CMonEvent::PreSendEvent()
{
	return true;
}

void CMonEvent::PostSendEvent()
{

}

bool CMonEvent::_hasValue(const string& key) const
{
	return m_map.find(key) != m_map.end();
}

void CMonEvent::AddValue(const string& key, const CValue& val)
{
	m_map[key] = val;
}

CMonEventSet::CMonEventSet(const string& typeId)
	: CMonEvent(typeId)
{

}

CMonEventSet::~CMonEventSet()
{
	for (const auto& e : m_events)
		delete e;
}

void CMonEventSet::AddEvent(CMonEvent* pEvent)
{
	m_events.insert(pEvent);
}

void CMonEventSet::DropEvent(CMonEvent* pEvent)
{
	m_events.erase(pEvent);
	delete pEvent;
}

string CMonEventSet::ToJsonString() const
{
	vector<string> temp;
	for (const auto& e : m_events)
		temp.push_back(e->ToJsonString());

	return CStringUtil::ToJsonArrayString(temp);
}

CValue CMonEventSet::RetrieveValue(const string& key)
{
	// We should not get here
	assert(0);
	return CValue();
}
