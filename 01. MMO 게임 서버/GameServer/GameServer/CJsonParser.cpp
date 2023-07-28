#include <Windows.h>
#include <map>

#include "CJsonParser.h"



/*******************************************
Json �����͸� ��Ÿ���� ���� CJsonObject �θ� Ŭ������ CJsonObject Ŭ������ ��ӹ޴� ������ Ŭ������
�� Json ������ Ÿ�� ���� �ϳ��� Ŭ������ �ִ�.
Key�� ���� Object �˻�,
Key:Value �� ������ ����,
Ŭ���� ��ü�κ��� c++ �⺻ �ڷ����� ������ ��� ���� ����� �ִ�.
********************************************/

// ��ȿ���� ���� key �Ǵ� index�� �˻����� �� ������ None ��ü
CJsonNone g_jsonNone;


/* CJsonObject */
// (virtual) CJsonStruct Ŭ������ ������. key�� �ش��ϴ� ��ü�� ������. �⺻�����δ� CJsonNone Ŭ���� ��ü�� ������.
CJsonObject& CJsonObject::Key(const std::wstring& _key)
{ 
	return g_jsonNone; 
} 

// (virtual) CJsonArray Ŭ������ ������. index ��ġ�� ��ü�� ������. �⺻�����δ� CJsonNone Ŭ���� ��ü�� ������.
CJsonObject& CJsonObject::Index(int _index) 
{
	return g_jsonNone; 
}




/* CJsonStruct */
CJsonStruct::CJsonStruct()
{
}

// (virtual)
CJsonStruct::~CJsonStruct()
{
	for (auto iter = m_mapObjects.begin(); iter != m_mapObjects.end(); ++iter)
	{
		delete iter->second;
	}
}

// (virtual) key�� �Է¹޾� �ش��ϴ� ��ü�� ������
CJsonObject& CJsonStruct::Key(const std::wstring& _key)
{
	std::wstring key = L'"' + _key + L'"';
	auto iter = m_mapObjects.find(key);
	if (iter == m_mapObjects.end())
		return g_jsonNone;

	return *iter->second;
}

// (virtual)
bool CJsonStruct::Empty()
{
	if (Size() == 0)
		return true;
	else
		return false;
}

// (virtual)
void CJsonStruct::Print()
{
	wprintf(L"{");
	for (auto iter = m_mapObjects.begin(); iter != m_mapObjects.end(); ++iter)
	{
		wprintf(L"%s: ", iter->first.c_str());
		iter->second->Print();
		wprintf(L", ");
	}
	wprintf(L" }");
}


// map�� Key:Value ���� ������
void CJsonStruct::Insert(std::wstring& _key, CJsonObject* _value)
{
	m_mapObjects.insert(std::make_pair(_key, _value));
}



/* CJsonArray */
CJsonArray::CJsonArray()
{
}

// (virtual)
CJsonArray::~CJsonArray()
{
	for (size_t i = 0; i < m_vecObjects.size(); i++)
	{
		delete m_vecObjects[i];
	}
}

// (virtual) ���޹��� index ��ġ�� ��ü�� �����Ѵ�.
CJsonObject& CJsonArray::Index(int _index)
{
	if (_index >= m_vecObjects.size() || _index < 0)
		return g_jsonNone;

	return *m_vecObjects[_index];
}

// (virtual)
bool CJsonArray::Empty()
{
	if (Size() == 0)
		return true;
	else
		return false;
}

// (virtual)
void CJsonArray::Print()
{
	wprintf(L"[");
	for (size_t i = 0; i < m_vecObjects.size(); i++)
	{
		m_vecObjects[i]->Print();
		wprintf(L", ");
	}
	wprintf(L" ]");
}

// vector�� ��ü�� �����Ѵ�.
void CJsonArray::Insert(CJsonObject* _object)
{
	m_vecObjects.push_back(_object);
}



/* CJsonString */
CJsonString::CJsonString(std::wstring& _str)
{
	m_value = _str.substr(1, _str.size() - 2);
}
// (virtual)
CJsonString::~CJsonString()
{
}


/* CJsonNumber */
CJsonNumber::CJsonNumber(std::wstring& _strNumber)
{
	m_value = std::stof(_strNumber);
}
// (virtual)
CJsonNumber::~CJsonNumber()
{
}


/* CJsonNone */
CJsonNone::CJsonNone()
{
}
// (virtual)
CJsonNone::~CJsonNone()
{
}












/*********************************
json text parsing Ŭ����
Ŭ���� ��ü�� ������ �� Parse �Լ��� json text�� �������ָ� parsing �Ѵ�.
parsing �� �� Key �Լ��� ���ϴ� Key�� ���� Value�� ���� �� �ִ�.
**********************************/

CJsonParser::CJsonParser()
	: m_pRootObject(nullptr), m_szJsonText(nullptr), _m_pParentObject(nullptr), _m_curIndex(0)
{
}

CJsonParser::~CJsonParser()
{
	Clear();
}

// ���Ϸκ��� parsing
void CJsonParser::ParseJsonFile(const WCHAR* _szJsonFilePath)
{
	FILE* fJson;
	_wfopen_s(&fJson, _szJsonFilePath, L"r");
	if (fJson == nullptr)
	{
		wprintf(L"Json File doesn't exists : %s", _szJsonFilePath);
		return;
	}

	// .json ���� �б�
	fseek(fJson, 0, SEEK_END);
	int sizeFile = ftell(fJson);
	WCHAR* szJsonText = (WCHAR*)malloc(sizeFile);
	rewind(fJson);
	fread_s(szJsonText, sizeFile, 1, sizeFile, fJson);
	fclose(fJson);

	// parse
	Parse(szJsonText);
	
	// �޸�����
	free(szJsonText);
}


// Parsing
void CJsonParser::Parse(const WCHAR* _szJsonText)
{
	Init();

	m_szJsonText = _szJsonText;
	// json text�� ��ū���� �и���
	GetTokens();

	// parent object�� root object�� ����
	_m_pParentObject = m_pRootObject;
	// ���� ù ��ū�� { �� �ƴ϶�� ���� ����
	if (m_vecTokens[0] != L"{")
	{
		RaiseException(0xE06D7363, EXCEPTION_NONCONTINUABLE, 0, NULL);
	}

	// parsing ����
	_m_curIndex = 1;
	ParseObject(L"}");

	// parse�� ���� �ڿ��� ��ü �ؽ�Ʈ�� ���̻� �ʿ����
	m_szJsonText = nullptr;
}




// ��ū ���
void CJsonParser::PrintTokens()
{
	wprintf(L"\n");
	for (size_t i = 0; i < m_vecTokens.size(); i++)
	{
		wprintf(L"%s, ", m_vecTokens[i].c_str());
	}
	wprintf(L"\n");
}


// json text�� ��ū���� �и�
// ��� ��ū�� wstring Ÿ������ ����Ǹ�, ���ڿ� ��ū�� �翷�� " �� ����.
void CJsonParser::GetTokens()
{
	const WCHAR* json = m_szJsonText;
	std::wstring token;
	while (*json != L'\0')
	{
		if (*json == L'{' || *json == L'}' || *json == L'[' || *json == L']' || *json == L':' || *json == L',')
		{
			token.clear();
			token.append(1, *json);
			m_vecTokens.push_back(token);
			json++;
		}
		else if (*json == L'"')
		{
			json++;
			int len = 2;
			while (*json != '"')
			{
				len++;
				json++;				
			}
			json++;
			token.clear();
			token.append(json - len, len);
			m_vecTokens.push_back(token);
		}
		else if ((*json >= L'0' && *json <= L'9') || *json == L'-')
		{
			json++;
			int len = 1;
			while ((*json >= L'0' && *json <= L'9') || *json == L'.')
			{
				len++;
				json++;
			}
			token.clear();
			token.append(json - len, len);
			m_vecTokens.push_back(token);
		}
		else
		{
			json++;
		}
	}
}



// ��� ��ū�� ��ȸ�ϸ� json �����͸� parsing ��
// ���� ���� ��ū�� _escapeToken �̶�� �����
// ���� ȣ�� �� _escapeToken �� L"}" �̾�� ��
void CJsonParser::ParseObject(std::wstring _escapeToken)
{
	while (m_vecTokens[_m_curIndex] != _escapeToken)
	{
		// ���� ��ū�� { �̸� json ����ü ����
		if (m_vecTokens[_m_curIndex] == L"{")
		{
			ParseStruct();
		}
		// ���� ��ū�� [ �̸� json array ����
		else if (m_vecTokens[_m_curIndex] == L"[")
		{
			ParseArray();
		}
		// ���� ��ū�� ���ڿ�("�ν���) �̰�, ���� ��ū�� : �̶�� ���� ��ū�� key��
		else if (m_vecTokens[_m_curIndex].front() == L'"' && m_vecTokens[_m_curIndex+1] == L":")
		{
			ParseKey();
		}
		// ���� ��ū�� ���ڿ�("�ν���)��
		else if (m_vecTokens[_m_curIndex].front() == L'"')
		{
			ParseString();
		}
		// ���� ��ū�� ������
		else if (  (m_vecTokens[_m_curIndex].front() >= L'0' && m_vecTokens[_m_curIndex].front() <= L'9') 
			     || m_vecTokens[_m_curIndex].front() == L'-')
		{
			ParseNumber();
		}
		else {
			_m_curIndex++;
			if (_m_curIndex >= m_vecTokens.size())
			{
				RaiseException(0xE06D7363, EXCEPTION_NONCONTINUABLE, 0, NULL);
			}
		}
	}
	return;
}

// ���� ��ū�� key ���� Ȯ����
void CJsonParser::ParseKey()
{
	// key ���� Ȯ���ϱ� ���� ��ū�� ���۰� ���� " ���� Ȯ����
	WCHAR front = m_vecTokens[_m_curIndex].front();
	WCHAR back = m_vecTokens[_m_curIndex].back();
	if (front != L'"' || back != L'"')
	{
		RaiseException(0xE06D7363, EXCEPTION_NONCONTINUABLE, 0, NULL);
	}

	// ���� ��ū�� key�� ��� ���� token�� �ݵ�� : �̰� �״��� ��ū�� �ݵ�� value �̾�� ��
	// value�� ���� ó���� ���⼭ ���� ����
	_m_curIndex += 2;

	return;
}

// ���� ��ū�� �����̹Ƿ� CJsonNumber ��ü�� �����ϰ� �����
void CJsonParser::ParseNumber()
{
	// ���� �θ� struct ��� �� ��ū�� key�� �־����
	if (_m_pParentObject->GetObjectType() == JSON_OBJECT_TYPE::STRUCT && m_vecTokens[_m_curIndex - 1] == L":")
	{
		CJsonNumber* jsonNumber = new CJsonNumber(m_vecTokens[_m_curIndex]);
		reinterpret_cast<CJsonStruct*>(_m_pParentObject)->Insert(m_vecTokens[_m_curIndex - 2], jsonNumber);  // �θ� struct�� key:value ���
	}
	// ���� �θ� array ��� �� ��ū�� key�� �������
	else if (_m_pParentObject->GetObjectType() == JSON_OBJECT_TYPE::ARRAY && m_vecTokens[_m_curIndex - 1] != L":")
	{
		CJsonNumber* jsonNumber = new CJsonNumber(m_vecTokens[_m_curIndex]);
		reinterpret_cast<CJsonArray*>(_m_pParentObject)->Insert(jsonNumber);  // �θ� array�� ��ü ���
	}
	else
	{
		RaiseException(0xE06D7363, EXCEPTION_NONCONTINUABLE, 0, NULL);
	}
	_m_curIndex++;
	return;
}

// ���� ��ū�� ���ڿ��̹Ƿ� CJsonString ��ü�� �����ϰ� �����
void CJsonParser::ParseString()
{
	// ���� �θ� struct ��� �� ��ū�� key�� �־����
	if (_m_pParentObject->GetObjectType() == JSON_OBJECT_TYPE::STRUCT && m_vecTokens[_m_curIndex - 1] == L":")
	{
		CJsonString* jsonString = new CJsonString(m_vecTokens[_m_curIndex]);
		reinterpret_cast<CJsonStruct*>(_m_pParentObject)->Insert(m_vecTokens[_m_curIndex - 2], jsonString);  // �θ� struct�� key:value ���
	}
	// ���� �θ� array ��� �� ��ū�� key�� �������
	else if (_m_pParentObject->GetObjectType() == JSON_OBJECT_TYPE::ARRAY && m_vecTokens[_m_curIndex - 1] != L":")
	{
		CJsonString* jsonString = new CJsonString(m_vecTokens[_m_curIndex]);
		reinterpret_cast<CJsonArray*>(_m_pParentObject)->Insert(jsonString);  // �θ� array�� ��ü ���
	}
	else
	{
		RaiseException(0xE06D7363, EXCEPTION_NONCONTINUABLE, 0, NULL);
	}
	_m_curIndex++;
	return;
}



// ���� ��ū�� { �̹Ƿ� CJsonStruct ��ü�� �����ϰ� �����
void CJsonParser::ParseStruct()
{
	// ���� parent object�� �����. struct ��ü�� ������ ��� �� ���� ���� ��ū���� ���� struct ��ü�� �θ� ��
	CJsonObject* pParentObjectBackup = _m_pParentObject;
	// ���� �θ� struct ��� �� ��ū�� key�� �־����
	if (_m_pParentObject->GetObjectType() == JSON_OBJECT_TYPE::STRUCT && m_vecTokens[_m_curIndex - 1] == L":")
	{
		CJsonStruct* jsonStruct = new CJsonStruct();
		reinterpret_cast<CJsonStruct*>(_m_pParentObject)->Insert(m_vecTokens[_m_curIndex - 2], jsonStruct);  // �θ� struct�� key:value ���
		_m_pParentObject = jsonStruct;
	}
	// ���� �θ� array ��� �� ��ū�� key�� �������
	else if (_m_pParentObject->GetObjectType() == JSON_OBJECT_TYPE::ARRAY && m_vecTokens[_m_curIndex - 1] != L":")
	{
		CJsonStruct* jsonStruct = new CJsonStruct();
		reinterpret_cast<CJsonArray*>(_m_pParentObject)->Insert(jsonStruct);  // �θ� array�� ��ü ���
		_m_pParentObject = jsonStruct;
	}
	else
	{
		RaiseException(0xE06D7363, EXCEPTION_NONCONTINUABLE, 0, NULL);
	}

	// ���� _escapeToken�� } �� �����Ͽ� ����ؼ� parsing
	_m_curIndex++;
	ParseObject(L"}");

	// ���� ��ġ�� ���ƿ��� �� ���� ��ū�� } �̾�� ��
	if (m_vecTokens[_m_curIndex] != L"}")
	{
		RaiseException(0xE06D7363, EXCEPTION_NONCONTINUABLE, 0, NULL);
	}
	// parent object�� �ǵ�����.
	_m_pParentObject = pParentObjectBackup;
	_m_curIndex++;
	return;
}


// ���� ��ū�� [ �̹Ƿ� CJsonArray ��ü�� �����ϰ� �����
void CJsonParser::ParseArray()
{
	// ���� parent object�� �����. array ��ü�� ������ ��� �� ���� ���� ��ū���� ���� array ��ü�� �θ� ��
	CJsonObject* pParentObjectBackup = _m_pParentObject;
	// ���� �θ� struct ��� �� ��ū�� key�� �־����
	if (_m_pParentObject->GetObjectType() == JSON_OBJECT_TYPE::STRUCT && m_vecTokens[_m_curIndex - 1] == L":")
	{
		CJsonArray* jsonArray = new CJsonArray();
		reinterpret_cast<CJsonStruct*>(_m_pParentObject)->Insert(m_vecTokens[_m_curIndex - 2], jsonArray);  // �θ� struct�� key:value ���
		_m_pParentObject = jsonArray;
	}
	// ���� �θ� array ��� �� ��ū�� key�� �������
	else if (_m_pParentObject->GetObjectType() == JSON_OBJECT_TYPE::ARRAY && m_vecTokens[_m_curIndex - 1] != L":")
	{
		CJsonArray* jsonArray = new CJsonArray();
		reinterpret_cast<CJsonArray*>(_m_pParentObject)->Insert(jsonArray);  // �θ� array�� ��ü ���
		_m_pParentObject = jsonArray;
	}
	else
	{
		RaiseException(0xE06D7363, EXCEPTION_NONCONTINUABLE, 0, NULL);
	}
	
	// ���� _escapeToken�� ] �� �����Ͽ� ����ؼ� parsing
	_m_curIndex++;
	ParseObject(L"]");

	// ���� ��ġ�� ���ƿ��� �� ���� ��ū�� } �̾�� ��
	if (m_vecTokens[_m_curIndex] != L"]")
	{
		RaiseException(0xE06D7363, EXCEPTION_NONCONTINUABLE, 0, NULL);
	}
	// parent object�� �ǵ�����.
	_m_pParentObject = pParentObjectBackup;
	_m_curIndex++;
	return;
}



// ��� ��ü ����
void CJsonParser::Clear()
{
	m_szJsonText = nullptr;

	if(m_pRootObject != nullptr)
		delete m_pRootObject;
	m_pRootObject = nullptr;
	_m_pParentObject = nullptr;

	m_vecTokens.clear();
}

// �ʱ�ȭ. �ֻ��� object�� �����Ѵ�.
void CJsonParser::Init()
{
	Clear();
	m_pRootObject = new CJsonStruct;
	_m_pParentObject = m_pRootObject;
}








