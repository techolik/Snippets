#include "FileExtractor.h"
#include <iostream>
#include <CFileUtil.h>
#include <CStringUtil.h>
#include <CPath.h>
#include <Shlwapi.h>

CFileExtractor::CFileExtractor(void)
{
}


CFileExtractor::~CFileExtractor(void)
{

}

bool CFileExtractor::extractText(const wstring& wpath, string& result) const
{
	string path = CStringUtil::WideStringToUtf8(wpath);
	if (CFileUtil::FileExist(path) != FS::FS_FILE)
		return false;
	
	string ext = CPath::GetFileExtension(path);
	CStringUtil::ToLower(ext);
	auto iter = extractors.find(ext);
	if (iter != extractors.end()){
		wstring wres = iter->second->extractText(wpath);
		result = CStringUtil::WideStringToUtf8(wres);
		return true;
	}

	return false;
}

void CFileExtractor::regExtractor(const string& ext, IExtractor* pExtractor)
{
	auto res = extractors.insert(Extractors::value_type(ext, pExtractor));
	//assert(res.second == false);
}
void CFileExtractor::unregExtractor(const string& ext)
{
	extractors.erase(ext);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
IExtractor::IExtractor(const vector<string>& exts)
	: extensions(exts)
{
	for (const auto& ext : extensions)
		CFileExtractor::GetInstance().regExtractor(ext, this);
}
IExtractor::~IExtractor()
{
	for (const auto& ext : extensions)
		CFileExtractor::GetInstance().unregExtractor(ext);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
class TextExtractor : public IExtractor{
public:
	TextExtractor()
		: IExtractor(initializer_list<string>({"txt", "xml" , "html", "htm"}))
		// will need separate extractors for html docs as the encoding can be very different
	{}
	~TextExtractor()
	{}

	wstring extractText(const wstring& wpath)
	{
		wifstream fi(wpath);
		wstring wres;
		fi >> wres;
		return wres;
	}
};
// Disable file extractors. If we need to enable them later, we need to use a 
// factory sort of construction mechnism so they're not put on the global scope. 
// CSingletons right now doesn't work on global scope.
//TextExtractor s_textExtractor;

