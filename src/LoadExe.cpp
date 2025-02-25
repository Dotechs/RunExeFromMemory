#include "pch.h"
#include "LoadExe.h"

std::vector<uint8_t> ReadIntoBuffer(const char* filename) {
    FILE* fileptr = fopen(filename, "rb");
    if (!fileptr) return {};  // Return empty vector if file can't be opened

    fseek(fileptr, 0, SEEK_END);
    size_t filelen = ftell(fileptr);
    rewind(fileptr);

    std::vector<BYTE> buffer(filelen);
    fread(buffer.data(), 1, filelen, fileptr);
    fclose(fileptr);

    return buffer;
}

int LoadExeFromMemory(LPPROCESS_INFORMATION ProcessInfo, LPSTARTUPINFO StartupInfo, LPVOID ImageBYTES, LPWSTR Args, SIZE_T szArgs) {
    // Retrieve the file path of the current exe
    char szFilePath[MAX_PATH];
    if (!GetModuleFileNameA(NULL, szFilePath, sizeof (szFilePath)))
        return -1;

    // Construct the command line arguments
    char ArgsBuffer[MAX_PATH + 2048] = { 0 };
    SIZE_T Len = strlen(szFilePath);
    memcpy(ArgsBuffer, szFilePath, Len);
    ArgsBuffer[Len] = ' ';
    memcpy(ArgsBuffer + Len + 1, Args, szArgs);

    // Validate the provided EXE image in memory
    PIMAGE_DOS_HEADER lpDOSHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(ImageBYTES);
    if (lpDOSHeader->e_magic != IMAGE_DOS_SIGNATURE)
        return -2;

    PIMAGE_NT_HEADERS lpNTHeader = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<uintptr_t>(ImageBYTES) + lpDOSHeader->e_lfanew);
    if (lpNTHeader->Signature != IMAGE_NT_SIGNATURE)
        return -3; // Return error if not a valid exe file

    // Create a suspend ed process
    if (!CreateProcess(NULL, ArgsBuffer, NULL, NULL, TRUE, CREATE_SUSPENDED, NULL, NULL, StartupInfo, ProcessInfo))
        return -4;

    // Get thread context of the newly created process
    CONTEXT stCtx = { 0 };
    stCtx.ContextFlags = CONTEXT_FULL;

    if (!GetThreadContext(ProcessInfo->hThread, &stCtx)) {
        TerminateProcess(ProcessInfo->hProcess, -5);
        return -5; // Return error if unable to retrieve thread context
    }

    // Allocate memory in the remote process for the new executable
    LPVOID ImageBase = VirtualAllocEx(ProcessInfo->hProcess,
        reinterpret_cast<LPVOID>(lpNTHeader->OptionalHeader.ImageBase),
        lpNTHeader->OptionalHeader.SizeOfImage,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE);

    if (!ImageBase) {
        TerminateProcess(ProcessInfo->hProcess, -6);
        return -6;
    }

    // Write the PE headers to the allocated memory
    if (!WriteProcessMemory(ProcessInfo->hProcess, ImageBase, ImageBYTES, lpNTHeader->OptionalHeader.SizeOfHeaders, NULL)) {
        TerminateProcess(ProcessInfo->hProcess, -7);
        return -7;
    }

    // Write each section to the allocated memory
    PIMAGE_SECTION_HEADER stSectionHeader = reinterpret_cast<PIMAGE_SECTION_HEADER>(reinterpret_cast<uintptr_t>(ImageBYTES) +
        lpDOSHeader->e_lfanew +
        sizeof(IMAGE_NT_HEADERS));

    for (SIZE_T iSection = 0; iSection < lpNTHeader->FileHeader.NumberOfSections; ++iSection) {

        if (!WriteProcessMemory(
            ProcessInfo->hProcess,
            reinterpret_cast<LPVOID>(reinterpret_cast<uintptr_t>(ImageBase) + stSectionHeader[iSection].VirtualAddress),
            reinterpret_cast<LPVOID>(reinterpret_cast<uintptr_t>(ImageBYTES) + stSectionHeader[iSection].PointerToRawData),
            stSectionHeader[iSection].SizeOfRawData,
            NULL
        )) {
            TerminateProcess(ProcessInfo->hProcess, -8);
            return -8;
        }
    }

    //This is the main block that differs x64 from x32
    // Modify the remote process stack to point to the new image base
#ifdef _WIN64
    if (!WriteProcessMemory(ProcessInfo->hProcess,
        reinterpret_cast<LPVOID>(stCtx.Rdx + sizeof(LPVOID) * 2),
        &ImageBase,
        sizeof(LPVOID),
        NULL)) {
        TerminateProcess(ProcessInfo->hProcess, -9);
        return -9; // Return error if stack modification fails
    }
    stCtx.Rcx = reinterpret_cast<uintptr_t>(ImageBase) + lpNTHeader->OptionalHeader.AddressOfEntryPoint;
#else
    if (!WriteProcessMemory(ProcessInfo->hProcess,
        reinterpret_cast<LPVOID>(stCtx.Ebx + 8),
        &ImageBase,
        sizeof(LPVOID),
        NULL)) {
        TerminateProcess(ProcessInfo->hProcess, -9);
        return -9; // Return error if stack modification fails
    }
    stCtx.Eax = reinterpret_cast<uintptr_t>(ImageBase) + lpNTHeader->OptionalHeader.AddressOfEntryPoint;
#endif // _WIN64

    // Set the entry point to the new executable's entry point
    if (!SetThreadContext(ProcessInfo->hThread, &stCtx)) {
        TerminateProcess(ProcessInfo->hProcess, -10);
        return -10;
    }

    // Resume the thread to start execution from the new entry point
    if (!ResumeThread(ProcessInfo->hThread)) {
        TerminateProcess(ProcessInfo->hProcess, -11);
        return -11;
    }
    return 0; // Success
}

void RunExe(std::vector<uint8_t>& ExeBytes) {
    PROCESS_INFORMATION ProcessInfo = { 0 };
    STARTUPINFO StartupInfo = { 0 };
    WCHAR wArgs[] = L"";

    //You can check for all error codes. but you probably wont need to, check the important notes.
    if (!LoadExeFromMemory(&ProcessInfo,
        &StartupInfo,
        reinterpret_cast<LPVOID>(ExeBytes.data()),
        wArgs,
        sizeof wArgs
    ))
    {
        WaitForSingleObject(ProcessInfo.hProcess, INFINITE);

        CloseHandle(ProcessInfo.hThread);
        CloseHandle(ProcessInfo.hProcess);
    }
}