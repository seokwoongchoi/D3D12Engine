#pragma once

class String
{
public:
	static void SplitString(vector<string>* result, string origin, string tok);
	static void SplitString(vector<wstring>* result, wstring origin, wstring tok);

	static bool StartsWith(string str, string comp);
	static bool StartsWith(wstring str, wstring comp);

	static bool Contain(string str, string comp);
	static bool Contain(wstring str, wstring comp);

	static void Replace(string* str, string comp, string rep);
	static void Replace(wstring* str, wstring comp, wstring rep);

	static wstring ToWString(string value);
	static string ToString(wstring value);

	//static const string& Format(const string& format, ...);
	//static const wstring& Format(const wstring& format, ...);
};