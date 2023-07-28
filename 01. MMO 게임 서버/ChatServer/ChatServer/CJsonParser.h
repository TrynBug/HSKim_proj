#pragma once

#include <string>
#include <vector>
#include <unordered_map>


/*******************************************
Json �����͸� ��Ÿ���� ���� CJsonObject �θ� Ŭ������ CJsonObject Ŭ������ ��ӹ޴� ������ Ŭ������
�� Json ������ Ÿ�� ���� �ϳ��� Ŭ������ �ִ�.
Key�� ���� Object �˻�,
Key:Value �� ������ ����,
Ŭ���� ��ü�κ��� c++ �⺻ �ڷ����� ������ ��� ���� ����� �ִ�.
********************************************/

#define ExceptionGetInvalidValue() RaiseException(0xE06D7363, EXCEPTION_NONCONTINUABLE, 0, NULL)

enum class JSON_OBJECT_TYPE
{
	OBJECT,   // �ֻ��� �߻�Ÿ��
	STRUCT,   // {}
	ARRAY,    // []
	STRING,   // ���ڿ�
	NUMBER,   // ����
	NONE,     // ������
	END
};


// ������ ������ ���� �θ� Ŭ����
class CJsonObject
{
public:
	virtual JSON_OBJECT_TYPE GetObjectType() { return JSON_OBJECT_TYPE::OBJECT; }   // ��ü Ÿ��
	virtual UINT Size() { return 0; }                                               // ���� ��ü�� CJsonStruct�� CJsonArray Ŭ������ ��� element ���� ������

public:
	virtual CJsonObject& Key(const std::wstring& _key);     // CJsonStruct Ŭ������ ������. key�� �ش��ϴ� ��ü�� ������. �⺻�����δ� CJsonNone Ŭ���� ��ü�� ������.
	virtual CJsonObject& Index(int _index);                 // CJsonArray Ŭ������ ������. index ��ġ�� ��ü�� ������. �⺻�����δ� CJsonNone Ŭ���� ��ü�� ������.

	virtual int Int() { ExceptionGetInvalidValue(); return 0; }             // CJsonNumber Ŭ������ ������. �ڽ��� float ����� int�� ������.
	virtual float Float() { ExceptionGetInvalidValue(); return 0; }         // CJsonNumber Ŭ������ ������. �ڽ��� float�� ������.
	virtual const std::wstring& Str() { ExceptionGetInvalidValue(); return nullptr; }   // CJsonString Ŭ������ ������. �ڽ��� wstring ����� ������.
	virtual WCHAR Char() { ExceptionGetInvalidValue(); return 0; }          // CJsonString Ŭ������ ������. �ڽ��� wstring ����� ù ��° ���ڸ� ������.

	virtual bool Empty() = 0;    // �迭�� ����ְų�, ���� ������ true ����
	virtual void Print() = 0;    // print

public:
	// [] ������ �����ε�
	CJsonObject& operator[] (const std::wstring& _key) { return Key(_key); }
	CJsonObject& operator[] (UINT _index) { return Index(_index); }

public:
	virtual ~CJsonObject() {};
};




// json ����ü. json text ���� { } �� �ش��Ѵ�.
// Key:Value ���� ������ �� �ִ� map�� ������ �ִ�.
class CJsonStruct : public CJsonObject
{
private:
	std::unordered_map<std::wstring, CJsonObject*> m_mapObjects;

public:
	virtual JSON_OBJECT_TYPE GetObjectType() { return JSON_OBJECT_TYPE::STRUCT; }
	virtual UINT Size() { return (UINT)m_mapObjects.size(); }

public:
	// key�� �Է¹޾� �ش��ϴ� ��ü�� ������
	virtual CJsonObject& Key(const std::wstring& _key);

	virtual bool Empty();
	virtual void Print();

public:
	// map�� Key:Value ���� ������
	void Insert(std::wstring& _key, CJsonObject* _value);

public:
	CJsonStruct();
	virtual ~CJsonStruct();
};


// json array. json text ���� [ ] �� �ش��Ѵ�.
// ��ü�� ���� �� �ִ� vector�� ������ �ִ�.
class CJsonArray : public CJsonObject
{
private:
	std::vector<CJsonObject*> m_vecObjects;
public:
	virtual JSON_OBJECT_TYPE GetObjectType() { return JSON_OBJECT_TYPE::ARRAY; }
	virtual UINT Size() { return (UINT)m_vecObjects.size(); }

public:
	// ���޹��� index ��ġ�� ��ü�� �����Ѵ�.
	virtual CJsonObject& Index(int _index);

	virtual bool Empty();
	virtual void Print();

public:
	// vector�� ��ü�� �����Ѵ�.
	void Insert(CJsonObject* _object);

public:
	CJsonArray();
	virtual ~CJsonArray();
};


// ���ڿ� ���� Ŭ����. json text ���� ���ڿ� �����Ϳ� �ش���.
class CJsonString : public CJsonObject
{
private:
	std::wstring m_value;

public:
	virtual JSON_OBJECT_TYPE GetObjectType() { return JSON_OBJECT_TYPE::STRING; }

public:
	virtual const std::wstring& Str() { return m_value; }
	virtual WCHAR Char() { return m_value.front(); }

	virtual bool Empty() { return false; }
	virtual void Print() { wprintf(L"%s", m_value.c_str()); }

public:
	CJsonString(std::wstring& _str);
	virtual ~CJsonString();
};


// ���� ���� Ŭ����. json text ���� ���� �����Ϳ� �ش���.
class CJsonNumber : public CJsonObject
{
private:
	float m_value;

public:
	virtual JSON_OBJECT_TYPE GetObjectType() { return JSON_OBJECT_TYPE::NUMBER; }

public:
	virtual int Int() { return (int)m_value; }
	virtual float Float() { return m_value; }

	virtual bool Empty() { return false; }
	virtual void Print() { wprintf(L"%f", m_value); }

public:
	CJsonNumber(std::wstring& _strNumber);
	virtual ~CJsonNumber();
};


// None ���� �ش��ϴ� Ŭ����
// �߸��� Key�� �˻��ϰų�, �߸��� index�� �˻����� �� global CJsonNone ��ü�� g_jsonNone �� ���ϵ�
// �� Ŭ���� ��ü�� �ٸ� ������ �������� ����.
class CJsonNone : public CJsonObject
{
public:
	virtual JSON_OBJECT_TYPE GetObjectType() { return JSON_OBJECT_TYPE::NONE; }

public:
	virtual bool Empty() { return true; }
	virtual void Print() { wprintf(L"None"); }

public:
	CJsonNone();
	virtual ~CJsonNone();
};













/*********************************
json text parsing Ŭ����
Ŭ���� ��ü�� ������ �� Parse �Լ��� json text�� �������ָ� parsing �Ѵ�.
parsing �� �� Key �Լ��� ���ϴ� Key�� ���� Value�� ���� �� �ִ�.
**********************************/
class CJsonParser
{
public:


private:
	const WCHAR* m_szJsonText;           // json text
	CJsonStruct* m_pRootObject;          // �ֻ��� object
	std::vector<std::wstring> m_vecTokens;   // ��ū

	// Parse �Լ� ���������� ���Ǵ� ����
	CJsonObject* _m_pParentObject;
	size_t _m_curIndex;

public:
	// json parsing
	void ParseJsonFile(const WCHAR* _szJsonFilePath);
	void Parse(const WCHAR* _szJsonText);

	// Parse �� �� Key �Լ� �Ǵ� [] �����ڷ� Key�� �ش��ϴ� object�� ������
	CJsonObject& Key(const std::wstring& _key) { return m_pRootObject->Key(_key); }
	CJsonObject& operator[] (const std::wstring& _key) { return Key(_key); }

	// print
	void PrintJson() { PrintJson(m_pRootObject); }
	void PrintJson(CJsonObject* _jsonObject) { _jsonObject->Print(); };
	void PrintTokens();
private:
	// json text�� token���� �и���
	void GetTokens();

	// parsing function
	void ParseObject(std::wstring _escapeToken);
	void ParseStruct();
	void ParseKey();
	void ParseArray();
	void ParseString();
	void ParseNumber();

	// clear, init
public:
	void Clear();
	void Init();

public:
	CJsonParser();
	~CJsonParser();
};









