#pragma once

class BinaryWriter
{
public:
	BinaryWriter();
	~BinaryWriter();

	void Open(wstring filePath, UINT openOption = CREATE_ALWAYS);
	void Close();

	void Bool(bool data);
	void Word(WORD data);
	void Int(int data);
	void UInt(UINT data);
	void Float(float data);
	void Double(double data);

	void F_Vector2(const Vector2& data);
	void F_Vector3(const Vector3& data);
	void F_Vector4(const Vector4& data);
	void Color3f(const Color4& data);
	void Color4f(const Color4& data);
	void F_Matrix(const Matrix& data);

	void String(const string& data);
	void Byte(void* data, UINT dataSize);

protected:
	HANDLE fileHandle;
	DWORD size;
};

//////////////////////////////////////////////////////////////////////////

class BinaryReader
{
public:
	BinaryReader();
	~BinaryReader();

	void Open(wstring filePath);
	void Close();

	bool Bool();
	WORD Word();
	int Int();
	UINT UInt();
	float Float();
	double Double();

	Vector2 F_Vector2();
	Vector3 F_Vector3();
	Vector4 F_Vector4();
	Color4 Color3f();
	Color4 Color4f();
	Matrix F_Matrix();

	string String();
	void Byte(void** data, UINT dataSize);

protected:
	HANDLE fileHandle;
	DWORD size;
};