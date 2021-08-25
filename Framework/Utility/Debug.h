#pragma once
#include "Framework.h"


class Debug
{
public:
	static void DebugVector(const Vector3& pos, string string,char key)
	{
		if (Keyboard::Get()->Down(key))
		{
			cout << string + ".x :";
			cout << pos.x << endl;
			cout << string + ".y :";
			cout << pos.y << endl;
			cout << string + ".z :";
			cout << pos.z << endl;
		}
	}
	static void DebugVector(const Vector3& pos, char key)
	{
		if (Keyboard::Get()->Down(key))
		{
			cout << "Pos.x :";
			cout << pos.x << endl;
			cout << "Pos.y :";
			cout << pos.y << endl;
			cout << "Pos.z :";
			cout << pos.z << endl;
		}

	}
	static void DebugVector(const Vector3& pos,string string)
	{
		
			cout << string+".x :";
			cout << pos.x << endl;
			cout << string+".y :";
			cout << pos.y << endl;
			cout << string+".z :";
			cout << pos.z << endl;
		
	}

	static void DebugVector(const Vector3& pos)
	{

		cout <<  "Pos.x :";
		cout << pos.x << endl;
		cout <<  "Pos.y :";
		cout << pos.y << endl;
		cout <<  "Pos.z :";
		cout << pos.z << endl;

	}
	//////////////////////////////////////////////////////////////////////////////////////
	static void DebugInt(const int& val, string string, char key)
	{
		if (Keyboard::Get()->Down(key))
		{
			cout << string+":";
			cout << val << endl;
			
		}
	}
	static void DebugInt(const int& val, char key)
	{
		if (Keyboard::Get()->Down(key))
		{
			cout << "IntVal:";
			cout << val << endl;
		}

	}
	static void DebugInt(const int& val, string string)
	{	
		cout << string + ":";
		cout << val << endl;
	
	}
	
	static void DebugInt(const int& val)
	{
		cout << "IntVal:";
		cout << val << endl;

	}


};
