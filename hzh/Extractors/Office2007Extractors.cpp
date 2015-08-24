#include "FileExtractor.h"
#include <unzip.h>
#include <CStringUtil.h>

//#define TIXML_USE_STL
#include <tinyxml.h>

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//
class DocPathsVisitor : public TiXmlVisitor{
	const char*		m_contentType;
	vector<string>	m_paths;
public:
	DocPathsVisitor(const char* contentType)
		: m_contentType(contentType)
	{}
	~DocPathsVisitor(){}
	
	// Got to stay in scope...
	const vector<string>&	paths() const	{ return m_paths; }

protected:
	bool VisitEnter(const TiXmlElement& element, const TiXmlAttribute* firstAttribute)
	{		
		const char* cType = element.Attribute("ContentType");
		if (cType && strcmp(cType, m_contentType) == 0)
		{
			const char* docPath = element.Attribute("PartName");
			if (docPath){
				// skip the leading '/'
				string path(docPath[0] == '/' ? docPath + 1 : docPath);
				m_paths.push_back(path);
			}
		}
		
		// Always keep looking as there might be more.
		return true; 
	}
};

class DocContentVisitor : public TiXmlVisitor{
public:
	string	strDocContent;
	bool Visit(const TiXmlText& text)
	{
		strDocContent += text.Value();
		strDocContent += " ";
		return true;
	}
};

class Office2007Extractor : public IExtractor
{
protected:
	Office2007Extractor(const vector<string>& exts)	: IExtractor(exts)	{}
	~Office2007Extractor()	{}

	virtual const char* docContentType() const = 0;

	wstring extractText(const wstring& wpath)
	{
		// The unzipping and locating of actualy document content can be 
		// shared among other extracts such as pptx, xlsx etc.

		// unzip
		//		find [Content_Types].xml
		//		from which read the location of document.xml
		//		read document.xml
		//

		string mcpath = CStringUtil::WideStringToUtf8(wpath);
		unzFile zf = unzOpen64(mcpath.c_str());
		string result;
		if (zf)
		{
			string schema = unzReadFile(zf, schemaName());
			TiXmlDocument xmlSchema;
			xmlSchema.Parse(schema.c_str());

			DocPathsVisitor pathVisitor(docContentType());
			xmlSchema.Accept(&pathVisitor);
			for (const string& docPath : pathVisitor.paths())
			{
				if (!docPath.empty())
				{
					string xmlContent = unzReadFile(zf, docPath.c_str());
					if (!xmlContent.empty())
					{
						TiXmlDocument xmldoc;
						xmldoc.Parse(xmlContent.c_str());

						DocContentVisitor contentVisitor;
						xmldoc.Accept(&contentVisitor);
						result += contentVisitor.strDocContent;
					}
				}
			}

			unzClose(zf);
		}
		return CStringUtil::Utf8ToWideString(result);
	}

private:
	static const char* schemaName()
	{
		return "[Content_Types].xml";
	}
	string unzReadFile(unzFile zf, const char* fileName)
	{
		string result;
		if (unzLocateFile(zf, fileName, 0) == UNZ_OK)
		{
			if (unzOpenCurrentFile(zf) == UNZ_OK){
				unz_file_info fileInfo;
				memset(&fileInfo, 0, sizeof(unz_file_info));

				if (unzGetCurrentFileInfo(zf, &fileInfo, NULL, 0, NULL, 0, NULL, 0) == UNZ_OK)
				{
					char* buffer = new char[fileInfo.uncompressed_size];
					int readSize = unzReadCurrentFile(zf, buffer, fileInfo.uncompressed_size);
					if (readSize > 0)
						result.assign(buffer, readSize);
					delete[] buffer;
				}
				unzCloseCurrentFile(zf);
			}
		}
		return result;
	}
};

class DocxExtractor : public Office2007Extractor{
public:
	DocxExtractor()	: Office2007Extractor(std::initializer_list<string>({ "docx", "dotx", "docm", "dotm" }))	{}
	~DocxExtractor(){}
protected:
	const char* docContentType() const
	{
		return "application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml";
	}
};
// Disable file extractors. If we need to enable them later, we need to use a 
// factory sort of construction mechnism so they're not put on the global scope. 
// CSingletons right now doesn't work on global scope.
//DocxExtractor s_docxExtractor;

class XlsxExtractor : public Office2007Extractor{
public:
	XlsxExtractor()	: Office2007Extractor(std::initializer_list<string>({ "xlsx", "xlsm" }))	{}
	~XlsxExtractor(){}
protected:
	const char* docContentType() const
	{
		// We're only interested in strings and Excel seems to do taking care of
		// that very well in sharedStrings.xml
		return "application/vnd.openxmlformats-officedocument.spreadsheetml.sharedStrings+xml";
	}
};
// Disable file extractors. If we need to enable them later, we need to use a 
// factory sort of construction mechnism so they're not put on the global scope. 
// CSingletons right now doesn't work on global scope.
//XlsxExtractor s_xlsxExtractor;

class PptxExtractor : public Office2007Extractor{
public:
	PptxExtractor()	: Office2007Extractor(std::initializer_list<string>({ "pptx", "ppsx", "pptm", "ppsm" }))	{}
	~PptxExtractor(){}
protected:
	const char* docContentType() const
	{
		return "application/vnd.openxmlformats-officedocument.presentationml.slide+xml";
	}
};
// Disable file extractors. If we need to enable them later, we need to use a 
// factory sort of construction mechnism so they're not put on the global scope. 
// CSingletons right now doesn't work on global scope.
//PptxExtractor s_pptxExtractor;
