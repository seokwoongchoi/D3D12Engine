#pragma once

class Path
{
public:
	static bool ExistFile(string path);
	static bool ExistFile(wstring path);

	static bool ExistDirectory(string path);
	static bool ExistDirectory(wstring path);

	
	

	static string GetDirectoryName(string path);
	static wstring GetDirectoryName(wstring path);

	static string GetExtension(string path);
	static wstring GetExtension(wstring path);

	static string GetFileName(string path);
	static wstring GetFileName(wstring path);

	static string GetFileNameWithoutExtension(string path);
	static wstring GetFileNameWithoutExtension(wstring path);
	

	const static WCHAR* ImageFilter;
	const static WCHAR* BinModelFilter;
	const static WCHAR* FbxModelFilter;
	const static WCHAR* ShaderFilter;
	const static WCHAR* XmlFilter;
	const static WCHAR* MeshFilter;
	const static WCHAR* EveryFilter;
	const static WCHAR* LevelFilter;

	static void OpenFileDialog(wstring file, const WCHAR* filter,  wstring folder, function<void(wstring)> func, HWND hwnd = NULL);
	static void OpenFileDialog(wstring file, const WCHAR* filter, wstring folder, uint num, function<void(wstring,uint)> func, HWND hwnd = NULL);
	static void OpenFileDialog(wstring file, const WCHAR* filter, wstring folder, uint num, shared_ptr<class Material> material,function<void(wstring, uint, shared_ptr<class Material>)> func, HWND hwnd = NULL);
	static void SaveFileDialog(wstring file, const WCHAR* filter, wstring folder, function<void(wstring)> func, HWND hwnd = NULL);
	static void OpenFileDialog(wstring file, const WCHAR* filter, wstring folder, bool isLevel, function<void(wstring, uint)> func, HWND hwnd = NULL);

	static void GetFiles(vector<string>* files, string path, string filter, bool bFindSubFolder);
	static void GetFiles(vector<wstring>* files, wstring path, wstring filter, bool bFindSubFolder);

	static void CreateFolder(string path);
	static void CreateFolder(wstring path);

	static void CreateFolders(string path);
	static void CreateFolders(wstring path);

	static bool IsRelativePath(string path);
	static bool IsRelativePath(wstring path);
};