#include "currentThread.h"
#include <thread>
#include <intrin.h>
#include <dbghelp.h>
#include <sstream>

#pragma comment(lib, "dbghelp.lib")

namespace CurrentThread {

	__thread int t_cachedTid = 0;
	__thread char t_tidString[32];
	__thread int t_tidStringLength = 6;
	__thread char t_threadName[256];

	void cacheTid() {
		if (t_cachedTid == 0) {
			std::thread::id id = std::this_thread::get_id();
			t_cachedTid =  *reinterpret_cast<int*>(&id);
			snprintf(t_tidString, sizeof t_tidString, "%5d", t_cachedTid);
		}
	}

#ifdef WIN32
	void setThreadName(DWORD dwThreadID, LPCSTR szThreadName) {
		THREADNAME_INFO info;
		info.dwType = 0x1000;
		info.szName = szThreadName;
		info.dwThreadID = dwThreadID;
		info.dwFlags = 0;

		__try {
			RaiseException(0x406D1388, 0, sizeof(info) / sizeof(DWORD), (ULONG_PTR*)&info);
		}
		__except (EXCEPTION_CONTINUE_EXECUTION) {
		}
	}
#endif
        
    std::string basename(const std::string& file) {
        unsigned int i = file.find_last_of("\\/");
        if (i == std::string::npos) {
            return file;
        }
        else {
            return file.substr(i + 1);
        }
    }

    std::vector<StackFrame> stacktrace() {
#if _WIN64
        DWORD machine = IMAGE_FILE_MACHINE_AMD64;
#else
        DWORD machine = IMAGE_FILE_MACHINE_I386;
#endif
        HANDLE process = GetCurrentProcess();
        HANDLE thread = GetCurrentThread();

        if (SymInitialize(process, NULL, TRUE) == FALSE) {
            //DBG_TRACE(__FUNCTION__ ": Failed to call SymInitialize.");
            return std::vector<StackFrame>();
        }

        SymSetOptions(SYMOPT_LOAD_LINES);

        CONTEXT    context = {};
        context.ContextFlags = CONTEXT_FULL;
        RtlCaptureContext(&context);

#if _WIN64
        STACKFRAME frame = {};
        frame.AddrPC.Offset = context.Rip;
        frame.AddrPC.Mode = AddrModeFlat;
        frame.AddrFrame.Offset = context.Rbp;
        frame.AddrFrame.Mode = AddrModeFlat;
        frame.AddrStack.Offset = context.Rsp;
        frame.AddrStack.Mode = AddrModeFlat;
#else
        STACKFRAME frame = {};
        frame.AddrPC.Offset = context.Eip;
        frame.AddrPC.Mode = AddrModeFlat;
        frame.AddrFrame.Offset = context.Ebp;
        frame.AddrFrame.Mode = AddrModeFlat;
        frame.AddrStack.Offset = context.Esp;
        frame.AddrStack.Mode = AddrModeFlat;
#endif

        bool first = true;
        std::vector<StackFrame> frames;
        while (StackWalk(machine, process, thread, &frame, &context, NULL, SymFunctionTableAccess, SymGetModuleBase, NULL)) {
            StackFrame f = {};
            f.address = frame.AddrPC.Offset;

#if _WIN64
            DWORD64 moduleBase = 0;
#else
            DWORD moduleBase = 0;
#endif
            moduleBase = SymGetModuleBase(process, frame.AddrPC.Offset);

            char moduelBuff[MAX_PATH];
            if (moduleBase && GetModuleFileNameA((HINSTANCE)moduleBase, moduelBuff, MAX_PATH)) {
                f.module = basename(moduelBuff);
            }
            else {
                f.module = "Unknown Module";
            }
#if _WIN64
            DWORD64 offset = 0;
#else
            DWORD offset = 0;
#endif
            char symbolBuffer[sizeof(IMAGEHLP_SYMBOL) + 255];
            PIMAGEHLP_SYMBOL symbol = (PIMAGEHLP_SYMBOL)symbolBuffer;
            symbol->SizeOfStruct = (sizeof IMAGEHLP_SYMBOL) + 255;
            symbol->MaxNameLength = 254;

            if (SymGetSymFromAddr(process, frame.AddrPC.Offset, &offset, symbol)) {
                f.name = symbol->Name;
            }
            else {
                DWORD error = GetLastError();
                //DBG_TRACE(__FUNCTION__ ": Failed to resolve address 0x%X: %u\n", frame.AddrPC.Offset, error);
                f.name = "Unknown Function";
            }

            IMAGEHLP_LINE line;
            line.SizeOfStruct = sizeof(IMAGEHLP_LINE);

            DWORD offset_ln = 0;
            if (SymGetLineFromAddr(process, frame.AddrPC.Offset, &offset_ln, &line)) {
                f.file = line.FileName;
                f.line = line.LineNumber;
            }
            else {
                DWORD error = GetLastError();
                //DBG_TRACE(__FUNCTION__ ": Failed to resolve line for 0x%X: %u\n", frame.AddrPC.Offset, error);
                f.line = 0;
            }

            if (!first) {
                frames.push_back(f);
            }
            first = false;
        }

        SymCleanup(process);

        return frames;
    }

    std::string convertStackFrame(std::vector<StackFrame>&& frames) {
        std::stringstream buff;

        for (int i = 0; i < frames.size(); i++) {
            buff << "0x" << std::hex << frames[i].address << ": " << std::dec <<
                frames[i].name << "(" << frames[i].line << ") in " << 
                frames[i].module << "\n";
        }

        return buff.str();
    }
}