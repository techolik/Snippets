#pragma once

#include "CReadWriteLock.h"
#include "Rules.h"

#include <unordered_map>

namespace Json{
	class Value;
}
class CRuleManger;

//
// CRConfig represents a config like this,
//
//	{
//		"k": "typeId.name",
//		"t": "config",
//		"v": value
//	},
//
class CRConfig
{
public:
	string	typeId;
	string	name;
	string	raw;
	CValue	value;
	bool	dirty;

	// The raw string of the config like shown in the class comment
	//string	raw;

	CRConfig(const string& tid, const string& n, const string& r, const CValue& v)
		: typeId(tid), name(n), raw(r), value(v), dirty(true)
	{}
};

//
// Anything configurable can derive from this and get configs.
//
class IConfigHandler
{
protected:
	IConfigHandler(const string& typeId, CRuleManager* pMgr);
	virtual ~IConfigHandler();

private:
	friend class CRuleManager;
	bool			LoadConfigs(const vector<CRConfig>& configs);
	void			ManagerGone();

protected:
	virtual bool	OnLoadConfigs(const vector<CRConfig>& configs) = 0;

private:
	string			m_strTypeId;
	CRuleManager*	m_pMgr;
};

//
// This is put up to solve the problem that the rules update comes from a
// thread (let's say thread A) that is different from the thread (B) that
// is using the rules. The solution is to call CRuleManager::UpdateAsynchRules
// from thread B in a regular basis, such as from within an event loop.
//
class IAsynchConfigHandler : public IConfigHandler
{
protected:
	IAsynchConfigHandler(const string& typeId, CRuleManager* pMgr);
	virtual ~IAsynchConfigHandler();

public:
	// This should be called from the thread that is safe to handle rules,
	// and should only be called by CRuleManager
	bool			SafeLoad();

protected:
	// Saves the update that comes from whichever thread, and cache it.
	// This method is made final to prevent subclasses from mistakenly
	// overriding it.
	virtual bool	OnLoadConfigs(const vector<CRConfig>& configs) final;

	virtual bool	OnSafeLoadConfigs(const vector<CRConfig>& configs) = 0;

private:
	vector<CRConfig>	m_configs;;
	CLock			m_cfglock;
};

//
// This is a special handler that sends rules to other processes
//
class IRuleProxy
{
protected:
	IRuleProxy(CRuleManager* pMgr);
	virtual ~IRuleProxy();

public:
	virtual bool	LoadRules(const string& strRules) = 0;

private:
	CRuleManager*	m_pMgr;
};

class CMonEvent;

class CRuleManager
{
public:
	CRuleManager(void);
	~CRuleManager(void);

	string			ParseRules(const string& strJson);

	// Dispatch all rules to handlers. Handlers live in different
	// places and are hooked up at different time, so this can be
	// called multiple times.
	bool			DispatchConfigs();

	// Update all the asynch rules, call this from the thread that is safe
	// to handle rules.
	bool			DispatchAsynchConfigs() const;

	// Filter the event based on the rules
	bool			Filter(CMonEvent* pEvent) const;

protected:
	IConfigHandler*	GetHandler(const string& typeId);

private:
	void			_addConfig(CRConfig* pCfg);
	void			_addRule(CRule* pRule);
	void			_deleteConfig(CRConfig* pCfg);
	void			_deleteRule(CRule* pRule);
	CRConfig*		_findConfig(const string& typeId, const string& raw);
	CRule*			_findRule(const string& typeId, const string raw);
	string			_dumpJsonValue(const Json::Value& v);

	typedef unordered_map<string, CRuleSet>		Rules;
	Rules			m_rules;

	friend class IConfigHandler;
	void			regHandler(const string& typeId, IConfigHandler* pHandler);
	void			unregHandler(const string& typeId);
	typedef	unordered_map<string, IConfigHandler*> Handlers;
	Handlers		m_handlers;

	// Only expecting one proxy at the moment...
	friend class IRuleProxy;
	void			setProxy(IRuleProxy* pProxy)	{ m_pProxy = pProxy; }
	void			unsetProxy(IRuleProxy* pProxy)	{ m_pProxy = NULL; }
	IRuleProxy*		m_pProxy;

	mutable CLock	m_updateLock;
};
