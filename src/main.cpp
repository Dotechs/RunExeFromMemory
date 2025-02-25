#include "pch.h"
#include "LoadExe.h"

int main() {

#ifdef _WIN64
    std::vector<uint8_t> bytes = ReadIntoBuffer("C:\\Windows\\System32\\cmd.exe"); //System32 cmd is x64 exe XD.
#else
    std::vector<uint8_t> bytes = ReadIntoBuffer("C:\\Windows\\SysWOW64\\cmd.exe");
#endif

    std::thread MyExeThread(RunExe, std::ref(bytes));

    //Do other stuff in the main thread, if you want.

	MyExeThread.join();
}


