#pragma once
#include "CSingleton.h"
#include <unordered_map>

class IExtractor
{
protected:
	IExtractor(const vector<string>& exts);
	virtual ~IExtractor();
public:
	virtual wstring extractText(const wstring& wpath) = 0;
private:
	vector<string> extensions;
};

class CFileExtractor : public CSingleton<CFileExtractor>
{
public:
	CFileExtractor(void);
	~CFileExtractor(void);

	bool extractText(const wstring& path, string& result) const;

	// TODO
	//bool extractImage(const wstring& path) const = 0;

private:
	friend class IExtractor;
	void regExtractor(const string& ext, IExtractor* pExtractor);
	void unregExtractor(const string& ext);
	typedef std::unordered_map<string, IExtractor*> Extractors;
	Extractors	extractors;
};

