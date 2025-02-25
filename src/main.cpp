#include "pch.h"
#include "LoadExe.h"

int main() {

#ifdef _WIN64
    std::vector<uint8_t> bytes = ReadIntoBuffer("C:\\Windows\\System32\\cmd.exe"); //System32 cmd is x64 exe XD.
    //std::vector<uint8_t> bytes = ReadIntoBuffer("D:\\Apps\\Reversing\\x64dbg\\release\\x64\\x64dbg.exe");
#else
    std::vector<uint8_t> bytes = ReadIntoBuffer("C:\\Windows\\SysWOW64\\cmd.exe");
    //std::vector<uint8_t> bytes = ReadIntoBuffer("D:\\Apps\\Reversing\\x64dbg\\release\\x32\\x32dbg.exe");
#endif

    std::thread MyExeThread(RunExe, std::ref(bytes));

    //Do other stuff in the main thread, if you want.

	MyExeThread.join();
}


