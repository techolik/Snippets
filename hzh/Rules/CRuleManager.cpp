#include "CRuleManager.h"
#include "Config.h"
#include "CEncrypt.h"
#include "CFileUtil.h"
#include "CStringUtil.h"
#include "CJsonLib.h"
#include "Module.h"
#include "CMonEvent.h"
#include "CStlSugars.h"

IConfigHandler::IConfigHandler(const string& typeId, CRuleManager* pMgr)
	: m_pMgr(pMgr), m_strTypeId(typeId)
{
	m_pMgr->regHandler(m_strTypeId, this);
}

IConfigHandler::~IConfigHandler(void)
{
	if (m_pMgr)
		m_pMgr->unregHandler(m_strTypeId);
}

bool IConfigHandler::LoadConfigs(const vector<CRConfig>& configs)
{
	return  OnLoadConfigs(configs);
}

void IConfigHandler::ManagerGone()
{
	m_pMgr = NULL;
}

IAsynchConfigHandler::IAsynchConfigHandler(const string& typeId, CRuleManager* pMgr)
	: IConfigHandler(typeId, pMgr)
{

}

IAsynchConfigHandler::~IAsynchConfigHandler()
{

}

bool IAsynchConfigHandler::OnLoadConfigs(const vector<CRConfig>& configs)
{
	SGUARD(&m_cfglock);
	m_configs = configs;
	return true;
}

bool IAsynchConfigHandler::SafeLoad()
{
	vector<CRConfig> configs;
	{
		SGUARD(&m_cfglock);
		m_configs.swap(configs);
	}

	if (!configs.empty())
		return OnSafeLoadConfigs(configs);

	return true;
}

IRuleProxy::IRuleProxy(CRuleManager* pMgr)
	: m_pMgr(pMgr)
{
	m_pMgr->setProxy(this);
}

IRuleProxy::~IRuleProxy()
{
	m_pMgr->unsetProxy(this);
}

CRuleManager::CRuleManager(void)
{
}

CRuleManager::~CRuleManager(void)
{
	for (auto it : m_handlers)
		it.second->ManagerGone();
}

string CRuleManager::ParseRules(const string& strJson)
{
	SGUARD(&m_updateLock);

	CJsonParser json;
	if (!json.Load(strJson))
		return "failed to load Json";

	enum {
		eFullUpdate = 0,
		eUpdate = 1,
		eDelete = 2
	} action;
	Json::Value& jvAction = json.GetRoot()["action"];
	if (!jvAction.isString())
		return "failed to load Json";
	string strAction = jvAction.asString();
	if (strAction == "fullupdate")
		action = eFullUpdate;
	else if (strAction == "update")
		action = eUpdate;
	else if (strAction == "delete")
		action = eDelete;
	else
		return "bad action type";

	Json::Value& jsonRules = json.GetRoot()["rules"];
	if (!jsonRules.isArray())
		return "bad rule file";
	
	if (action == eFullUpdate)
		m_rules.clear();

	for (auto& v : jsonRules)
	{
		Json::Value& jsonKey = v["k"];
		if (!jsonKey.isString())
			return "bad key: " + _dumpJsonValue(v);
		Json::Value& jsonType = v["t"];
		if (!jsonType.isString())
			return "bad type: " + _dumpJsonValue(v);

		string key = jsonKey.asString();
		size_t pos = key.find(".");
		string typeId, name;
		if (pos != string::npos)		{
			typeId = key.substr(0, pos);
			pos = typeId.find("not ");
			if (pos != string::npos)
				typeId = typeId.substr(4);
			name = key.substr(pos + 1);
		}
		else{
			typeId = key;
		}
		if (typeId.empty()/* || name.empty()*/)
			return "bad key: " + _dumpJsonValue(v);

		string type = jsonType.asString();
		if (type == "config"){
			if (name.empty())
				return "bad key: " + key;

			CRConfig* pConfig = _findConfig(typeId, key);
			if (pConfig){
				// If found, it's 'delete' or 'update'
				if (action == eUpdate){
					pConfig->value = CValue::fromJsonValue(v["v"]);
					pConfig->dirty = true;
				}
				else
					_deleteConfig(pConfig);
			}
			else{
				// Otherwise it's 'fullupdate'
				pConfig = new CRConfig(typeId, name, key, CValue::fromJsonValue(v["v"]));
				_addConfig(pConfig);
			}
		}
		else if (type == "rule")
		{
			CRule* pRule = _findRule(typeId, key);
			if (pRule){
				// If found, it's 'delete' or 'update'
				if (action == eUpdate){
					CValue cv = CValue::fromJsonValue(v["v"]);
					if (cv.type() != CValue::eStringVec && cv.type() != CValue::eNone)
						return "bad fields" + _dumpJsonValue(v);
					pRule->fields.clear();
					pRule->fields.insert(cv.asStringVec().begin(), cv.asStringVec().end());
					if (!pRule->Parse(key)){
						// Have to delete because the state has been changed
						_deleteRule(pRule);
						return CStringUtil::format("parsing rule failed for: %s", key.c_str());
					}
				}
				else
					_deleteRule(pRule);
			}
			else{
				// Otherwise it's 'fullupdate'
				pRule = new CRule;
				pRule->typeId = typeId;
				if (!pRule->Parse(key)){
					delete pRule;
					return CStringUtil::format("parsing rule failed for: %s", key.c_str());
				}

				pRule->raw = key;
				CValue cv = CValue::fromJsonValue(v["v"]);
				if (cv.type() != CValue::eStringVec && cv.type() != CValue::eNone){
					delete pRule;
					return "bad fields" + _dumpJsonValue(v);
				}
				pRule->fields.insert(cv.asStringVec().begin(), cv.asStringVec().end());

				_addRule(pRule);
			}
		}
	}

	if (m_pProxy)
		return m_pProxy->LoadRules(strJson) ? "" : "proxy failed";
	
	return "";
}

void CRuleManager::_addConfig(CRConfig* pCfg)
{
	auto it = m_rules.find(pCfg->typeId);
	if (it == m_rules.end()){
		CRuleSet rules;
		rules.typeId = pCfg->typeId;
		rules.m_configs.push_back(pCfg);
		m_rules.emplace(make_pair(rules.typeId, std::move(rules)));
	}
	else
		it->second.m_configs.push_back(pCfg);
}

void CRuleManager::_addRule(CRule* pRule)
{
	auto it = m_rules.find(pRule->typeId);
	if (it == m_rules.end()){
		CRuleSet rules;
		rules.typeId = pRule->typeId;
		rules.m_rules.push_back(pRule);
		m_rules.emplace(make_pair(rules.typeId, std::move(rules)));
	}
	else
		it->second.m_rules.push_back(pRule);
}

void CRuleManager::_deleteConfig(CRConfig* pCfg)
{
	auto it = m_rules.find(pCfg->typeId);
	if (it != m_rules.end())
		CStlSugars::erase(it->second.m_configs, pCfg);
	delete pCfg;
}

void CRuleManager::_deleteRule(CRule* pRule)
{
	auto it = m_rules.find(pRule->typeId);
	if (it != m_rules.end())
		CStlSugars::erase(it->second.m_rules, pRule);
	delete pRule;
}

CRConfig* CRuleManager::_findConfig(const string& typeId, const string& raw)
{
	auto it = m_rules.find(typeId);
	if (it != m_rules.end()){
		for (auto pConfig : it->second.m_configs)
		{
			if (pConfig->raw == raw)
				return pConfig;
		}
	}
	return NULL;
}

CRule* CRuleManager::_findRule(const string& typeId, const string raw)
{
	auto it = m_rules.find(typeId);
	if (it != m_rules.end()){
		for (auto pRule : it->second.m_rules)
		{
			if (pRule->raw == raw)
				return pRule;
		}
	}
	return NULL;
}

string CRuleManager::_dumpJsonValue(const Json::Value& v)
{
	CJsonBuild json;
	json.SetRoot(v);
	return json.WriteString();
}

bool CRuleManager::DispatchConfigs()
{
	for (const auto& it : m_rules)
	{
		if (IConfigHandler* pHandler = GetHandler(it.second.typeId))
		{
			// Only send for load the 'dirty' configs
			vector<CRConfig> configs;
			for (auto pConfig : it.second.m_configs)
				if (pConfig->dirty){
					configs.push_back(*pConfig);
					pConfig->dirty = false;
				}
			
			if (!pHandler->LoadConfigs(configs))
				return false;
		}
	}
	return true;
}

bool CRuleManager::DispatchAsynchConfigs() const
{
	for (const auto& it : m_handlers)
	{
		if (IAsynchConfigHandler* pAsynchHandler = dynamic_cast<IAsynchConfigHandler*>(it.second))
			if (!pAsynchHandler->SafeLoad())
				return false;
	}
	return true;
}

bool CRuleManager::Filter(CMonEvent* pEvent) const
{
	SGUARD(&m_updateLock);

	const auto it = m_rules.find(pEvent->TypeId());
	if (it != m_rules.end())
	{
		return it->second.Filter(pEvent);
	}

	return false;
}

void CRuleManager::regHandler(const string& typeId, IConfigHandler* pHandler)
{
	auto res = m_handlers.insert(Handlers::value_type(typeId, pHandler));
	//assert(res.second == false);
}
void CRuleManager::unregHandler(const string& typeId)
{
	m_handlers.erase(typeId);
}

IConfigHandler* CRuleManager::GetHandler(const string& typeId)
{
	auto res = m_handlers.find(typeId);
	if (res == m_handlers.end())
		return NULL;
	return res->second;
}
