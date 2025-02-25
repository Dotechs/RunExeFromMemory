#pragma once
#include "pch.h"

std::vector<uint8_t> ReadIntoBuffer(const char* filename);
int LoadExeFromMemory(LPPROCESS_INFORMATION ProcessInfo, LPSTARTUPINFO StartupInfo, LPVOID ImageBYTES, LPWSTR Args, SIZE_T szArgs);
void RunExe(std::vector<uint8_t>& ExeBytes);