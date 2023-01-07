#include "processInfo.h"
#include "thread.h"

#include <Sddl.h>
#include <time.h>

Timestamp g_startTime = Timestamp::now();

char* wchar2char(const wchar_t* src) {
	if (!src)
		return 0;

	int cp = GetACP();
	int size = WideCharToMultiByte(cp, 0, src, -1, NULL, 0, NULL, NULL);
	char* str = (char*)malloc(size * sizeof(char));
	if (WideCharToMultiByte(cp, 0, src, -1, str, size, NULL, NULL) != size) {
		;
	}

	return str;
}

namespace ProcessInfo {
	
	int pid() {
		DWORD pid = GetCurrentProcessId();
		return pid;
	}

	string pidString() {
		char buf[16];
		snprintf(buf, sizeof buf, "%d", pid());
		return buf;
	}

	string sid() {
		string name = username();
		char domainName[64];
		DWORD cchDomainName = 64;
		char sid[64];
		DWORD cbSid = 64;
		SID_NAME_USE sidType;
		LPSTR sidstring = 0;

		int ret = LookupAccountNameA(NULL, name.c_str(), sid, &cbSid, domainName, &cchDomainName, &sidType);
		if (ret)
			ConvertSidToStringSidA(sid, &sidstring);

		return (sidstring != NULL) ? string(sidstring) : string();
	}

	string username() {
		char buf[32];
		DWORD cbSize;
		GetUserNameA(buf, &cbSize);
		return buf;
	}

	Timestamp startTime() {
		return g_startTime;
	}

	int64_t clockTicksPerSecond() {
		return CLOCKS_PER_SEC;
	}

	int pageSize() {
		SYSTEM_INFO sysInfo;
		GetSystemInfo(&sysInfo);
		return sysInfo.dwPageSize;
	}

	bool isDebugBuild() {
#ifdef NDEBUG
		return false;
#else
		return true;
#endif
	}

	string hostname() {
		char buf[256];
		DWORD size;
		GetComputerNameA(buf, &size);
		return string(buf);
	}

	string procname() {
		DWORD pid = GetCurrentProcessId();
		return procname(pid);
	}

	string procname(int pid) {
		CToolhelp thProcesses(TH32CS_SNAPPROCESS);
		PROCESSENTRY32 pe = { sizeof(pe) };
		BOOL fOk = thProcesses.ProcessFirst(&pe);
		TCHAR sz[1024];

		while (fOk) {
			if (pid == pe.th32ProcessID) {
				lstrcpyW(sz, pe.szExeFile);
				break;
			}
			fOk = thProcesses.ProcessNext(&pe);
		}

		char* name = (char*)wchar2char(sz);
		string str(name);
		free(name);

		return str;
	}

	CpuTime cpuTime() {
		FILETIME kerneltime;
		FILETIME usertime;
		FILETIME starttime;
		FILETIME exittime;
		CpuTime cputime;

		if (GetProcessTimes(GetCurrentProcess(), &starttime, &exittime, &kerneltime, &usertime)) {
			cputime.userSeconds = *((__int64*)&usertime) / 10000000.0f;
			cputime.systemSeconds = *((__int64*)&kerneltime) / 10000000.0f;
		}

		return cputime;
	}

	int numThreads() {
		// get all threads on all process
		int cnt = 0;
		CToolhelp th = CToolhelp(TH32CS_SNAPALL);
		PROCESSENTRY32 pe = { sizeof(pe) };
		BOOL fOk = th.ProcessFirst(&pe);

		while (fOk) {
			cnt += pe.cntThreads;
			fOk = th.ProcessNext(&pe);
		}

		return cnt;
	}

	threadsInfo threads() {
		threadsInfo vec;
		vec.reserve(2048);

		CToolhelp th = CToolhelp(TH32CS_SNAPALL);
		THREADENTRY32 te = { sizeof(te) };
		BOOL fOk = th.ThreadFirst(&te);

		while (fOk) {
			DWORD tid = te.th32ThreadID;
			if (tid) {
				PWSTR threadName = 0;
				HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, te.th32ThreadID);
				if (hThread) {
					GetThreadDescription(hThread, &threadName);
				}
				char* str = wchar2char(threadName);
				string thname = str ? string(str) : "";
				vec.push_back(std::pair(te.th32ThreadID, thname));
				if (str)
					free(str);
			}
			fOk = th.ThreadNext(&te);
		}

		return vec;
	}

	fileInfo openedFiles(int getNamedPipe, int timeout, int print) {
		fileInfo finfo;
		int fileTypeIndex = getFileTypeIndex();
		if (fileTypeIndex == -1) {
			return fileInfo();
		}

		CToolhelp thProcesses(TH32CS_SNAPPROCESS);
		PROCESSENTRY32 pe = { sizeof(pe) };
		BOOL fOk = thProcesses.ProcessFirst(&pe);

		while (fOk) {
			HANDLE hP = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe.th32ProcessID);
			if (hP) {
				ULONG size = 1 << 10;
				std::unique_ptr<BYTE[]> buffer;
				for (;;) {
					buffer = std::make_unique<BYTE[]>(size);
					auto status = ::NtQueryInformationProcess(hP, (PROCESSINFOCLASS)51,
						buffer.get(), size, &size);
					if (NT_SUCCESS(status))
						break;
					else if (status == STATUS_INFO_LENGTH_MISMATCH) {
						size *= 2;
						continue;
					}
					else {
						break;
					}
				}

				auto info = reinterpret_cast<PROCESS_HANDLE_SNAPSHOT_INFORMATION*>(buffer.get());
				for (ULONG i = 0; i < info->NumberOfHandles; i++) {
					auto nameBuffer = std::make_unique<BYTE[]>(size);
					if (info->Handles[i].ObjectTypeIndex == fileTypeIndex) {
						std::vector<std::string>& files = finfo[pe.th32ProcessID];
						HANDLE hv = info->Handles[i].HandleValue;
						HANDLE hTarget;

						if (!::DuplicateHandle(hP, hv, ::GetCurrentProcess(), &hTarget, 0, 0, 0))
							continue;

						DWORD fileType = GetFileType(hTarget);
						if (fileType == FILE_TYPE_PIPE) {
							// query on named pipe will block, so skip it
							// unless it has a kernel driver
							if (!getNamedPipe) {
								char buf1[32];
								sprintf_s(buf1, "0x%016x", hv);
								files.push_back(string(buf1));
								continue;
							}
						}

						NTSTATUS status;
						Thread th([&] {
							status = ::NtQueryObject(hTarget, (OBJECT_INFORMATION_CLASS)1, nameBuffer.get(), size, nullptr);
							::CloseHandle(hTarget);
						});


						th.start();
						WaitForSingleObject(th.handle(), timeout);

						if (!NT_SUCCESS(status)) {
							char buf1[32];
							sprintf_s(buf1, "0x%016x", hv);
							files.push_back(string(buf1));
							continue;
						}

						auto name = reinterpret_cast<UNICODE_STRING*>(nameBuffer.get());
						if (!name->Buffer) {
							char buf1[32];
							sprintf_s(buf1, "0x%016x", hv);
							files.push_back(string(buf1));
						} else {
							char* str = wchar2char(name->Buffer);
							
							char realPath[1024];
							if (GetLogicalDriveStringsA(sizeof(realPath) - 1, realPath)) {
								char szName[MAX_PATH];
								char szDrive[3] = " :";
								bool bFound = 0;
								char* p = realPath;

								do {
									*szDrive = *p;
									if (QueryDosDeviceA(szDrive, szName, sizeof(szName))) {
										size_t uNameLen = strlen(szName);
										bFound = strncmp(str, szName, uNameLen) == 0 &&
											*(str + uNameLen) == '\\';
										if (bFound) {
											char szTempFile[MAX_PATH];
											sprintf_s(szTempFile, MAX_PATH, "%s%s", szDrive, str + uNameLen);
											files.push_back(string(szTempFile));
											if (print)
												printf("%s\n", szTempFile);
										}
									}
									while (*p++);
								} while (!bFound && *p);
								if (!bFound) {
									files.push_back(string(str));
									if (print)
										printf("%s\n", str);
									free(str);
								}
							} else { // end of GetLogicalDriveStringsA
								files.push_back(string(str));
								if (print)
									printf("%s\n", str);
								free(str);
							}
						}
					}
				}
			}

			CloseHandle(hP);
			fOk = thProcesses.ProcessNext(&pe);
		}

		return finfo;
	}

	int getFileTypeIndex() {
		int idx = -1;
		ULONG size = 1 << 12;
		NTSTATUS status;
		POBJECT_TYPES_INFORMATION objectTypes;
		POBJECT_TYPE_INFORMATION objectType;
		std::unique_ptr<BYTE[]> buffer = std::make_unique<BYTE[]>(size);;

		while ((status = NtQueryObject(
			NULL,
			OBJECT_INFORMATION_CLASS(3),
			buffer.get(),
			size,
			NULL
		)) == STATUS_INFO_LENGTH_MISMATCH) {
			size *= 2;
			buffer = std::make_unique<BYTE[]>(size);
		}

		objectTypes = (POBJECT_TYPES_INFORMATION)(buffer.get());
		if (objectTypes) {
			objectType = PH_FIRST_OBJECT_TYPE(objectTypes);
			for (int i = 0; i < objectTypes->NumberOfTypes; i++) {
				if (::_wcsicmp(objectType->TypeName.Buffer, L"File") == 0) {
					idx = objectType->TypeIndex;
					break;
				}
				objectType = PH_NEXT_OBJECT_TYPE(objectType);
			}
		}

		return idx;
	}
}