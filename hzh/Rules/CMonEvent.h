#pragma once

#include "CBaseLibInc.h"
#include <unordered_map>

class CValue;
class CRuleManager;

//
// Base class for all the monitor events. It holds the data to be
// sent to the server. Subclasses are called to fill the data when
// needed.
//
class CMonEvent
{
public:
	virtual ~CMonEvent();

	const string&	TypeId() const { return m_strTypeId; }

	// These will be manipulated by the rule engine, based on the rules.
	CValue			GetValue(const string& key);
	bool			FillFields(const vector<string>& keys);

	virtual string	ToJsonString() const;

	// Some preprocessing can be done here, for example CMonEvent will fill
	// in the basic fields such as 'eventtime'. And the subclass can do some
	// basic filtering, such as checking for validity.
	// If the return value is false, the event will be dropped. 
	// It's best for subclass to 'chain' the call to base class implementation,
	//
	virtual bool	PreSendEvent();

	// Do cleanups, if any. Chain the call to base class implementation.
	virtual void	PostSendEvent();

protected:
	CMonEvent(const string& st);

	virtual	CValue	RetrieveValue(const string& key) = 0;

	void			AddValue(const string& key, const CValue& val);

private:
	bool			_hasValue(const string& key) const;

	//typedef unordered_map<string, CValue>		Map;
	typedef map<string, CValue>		Map;
	Map				m_map;
	string			m_strTypeId;
};

//
// This represents an array of events that's collected and
// reported together.
//
class CMonEventSet : public CMonEvent
{
protected:
	CMonEventSet(const string& typeId);
	virtual ~CMonEventSet();

public:
	// CMonEventSet owns the CMonEvent* passed in.
	void			AddEvent(CMonEvent* pEvent);

	// During filtering if a member event is filtered off, it will be dropped
	void			DropEvent(CMonEvent* pEvent);
	const set<CMonEvent*>&	Events() const{ return m_events; }

	virtual string	ToJsonString() const;

protected:
	virtual	CValue	RetrieveValue(const string& key) final;

private:
	set<CMonEvent*>	m_events;
};
