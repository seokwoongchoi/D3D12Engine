#pragma once


const char* Application_GetName();
void Application_Initialize(ID3D12Device* device);
void Application_Finalize();
void Application_Frame(bool* show);


void SaveAllNodes();
void LoadAllNodes(const wstring & file);
void Compile();