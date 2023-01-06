#pragma once

#include <Windows.h>
#include <shlobj.h>
#include <winternl.h>
#include <tlhelp32.h>

#pragma comment(lib, "ntdll")

// undocumented
//#define NT_SUCCESS(status)				 (status >= 0)
#define STATUS_INFO_LENGTH_MISMATCH      ((NTSTATUS)0xC0000004L)

/*
enum PROCESSINFOCLASS {
	ProcessHandleInformation = 51
};

enum SYSTEMINFORMATIONCLASS {
	SystemProcessInformation = 5
};
*/

typedef struct _PROCESS_HANDLE_TABLE_ENTRY_INFO {
	HANDLE HandleValue;
	ULONG_PTR HandleCount;
	ULONG_PTR PointerCount;
	ULONG GrantedAccess;
	ULONG ObjectTypeIndex;
	ULONG HandleAttributes;
	ULONG Reserved;
} PROCESS_HANDLE_TABLE_ENTRY_INFO, * PPROCESS_HANDLE_TABLE_ENTRY_INFO;

// private
typedef struct _PROCESS_HANDLE_SNAPSHOT_INFORMATION {
	ULONG_PTR NumberOfHandles;
	ULONG_PTR Reserved;
	PROCESS_HANDLE_TABLE_ENTRY_INFO Handles[1];
} PROCESS_HANDLE_SNAPSHOT_INFORMATION, * PPROCESS_HANDLE_SNAPSHOT_INFORMATION;

typedef struct _THREAD_BASIC_INFORMATION {
	NTSTATUS ExitStatus;
	PTEB TebBaseAddress;
	CLIENT_ID ClientId;
	ULONG_PTR AffinityMask;
	KPRIORITY Priority;
	LONG BasePriority;
} THREAD_BASIC_INFORMATION, * PTHREAD_BASIC_INFORMATION;

extern "C" NTSTATUS NTAPI NtQueryInformationProcess(
	_In_ HANDLE ProcessHandle,
	_In_ PROCESSINFOCLASS ProcessInformationClass,
	_Out_writes_bytes_(ProcessInformationLength) PVOID ProcessInformation,
	_In_ ULONG ProcessInformationLength,
	_Out_opt_ PULONG ReturnLength);

extern "C" NTSTATUS NtQueryInformationThread(
	HANDLE          ThreadHandle,
	THREADINFOCLASS ThreadInformationClass,
	PVOID           ThreadInformation,
	ULONG           ThreadInformationLength,
	PULONG          ReturnLength
);

extern "C" NTSTATUS NtQueryInformationFile(
	HANDLE                 FileHandle,
	PIO_STATUS_BLOCK       IoStatusBlock,
	PVOID                  FileInformation,
	ULONG                  Length,
	FILE_INFORMATION_CLASS FileInformationClass
);

typedef struct _FILE_NAME_INFORMATION {
	ULONG FileNameLength;
	WCHAR FileName[1];
} FILE_NAME_INFORMATION, * PFILE_NAME_INFORMATION;

/*copy from processhacker */
typedef struct _OBJECT_TYPE_INFORMATION
{
	UNICODE_STRING TypeName;
	ULONG TotalNumberOfObjects;
	ULONG TotalNumberOfHandles;
	ULONG TotalPagedPoolUsage;
	ULONG TotalNonPagedPoolUsage;
	ULONG TotalNamePoolUsage;
	ULONG TotalHandleTableUsage;
	ULONG HighWaterNumberOfObjects;
	ULONG HighWaterNumberOfHandles;
	ULONG HighWaterPagedPoolUsage;
	ULONG HighWaterNonPagedPoolUsage;
	ULONG HighWaterNamePoolUsage;
	ULONG HighWaterHandleTableUsage;
	ULONG InvalidAttributes;
	GENERIC_MAPPING GenericMapping;
	ULONG ValidAccessMask;
	BOOLEAN SecurityRequired;
	BOOLEAN MaintainHandleCount;
	UCHAR TypeIndex; // since WINBLUE
	CHAR ReservedByte;
	ULONG PoolType;
	ULONG DefaultPagedPoolCharge;
	ULONG DefaultNonPagedPoolCharge;
} OBJECT_TYPE_INFORMATION, * POBJECT_TYPE_INFORMATION;

typedef struct _OBJECT_TYPES_INFORMATION
{
	ULONG NumberOfTypes;
} OBJECT_TYPES_INFORMATION, * POBJECT_TYPES_INFORMATION;

#define PH_FIRST_OBJECT_TYPE(ObjectTypes) \
    (POBJECT_TYPE_INFORMATION)((PCHAR)(ObjectTypes) + ALIGN_UP(sizeof(OBJECT_TYPES_INFORMATION), ULONG_PTR))

#define PH_NEXT_OBJECT_TYPE(ObjectType) \
    (POBJECT_TYPE_INFORMATION)((PCHAR)(ObjectType) + sizeof(OBJECT_TYPE_INFORMATION) + \
    ALIGN_UP(ObjectType->TypeName.MaximumLength, ULONG_PTR))

#define ALIGN_UP_BY(Address, Align) (((ULONG_PTR)(Address) + (Align) - 1) & ~((Align) - 1))
#define ALIGN_UP_POINTER_BY(Pointer, Align) ((PVOID)ALIGN_UP_BY(Pointer, Align))
#define ALIGN_UP(Address, Type) ALIGN_UP_BY(Address, sizeof(Type))
#define ALIGN_UP_POINTER(Pointer, Type) ((PVOID)ALIGN_UP(Pointer, Type))


/*
Module:  Toolhelp.h
Notices : Copyright(c) 2008 Jeffrey Richter & Christophe Nasarre
*/

class CToolhelp {
private:
	HANDLE m_hSnapshot;

public:
	CToolhelp(DWORD dwFlags = 0, DWORD dwProcessID = 0);
	~CToolhelp();

	BOOL CreateSnapshot(DWORD dwFlags, DWORD dwProcessID = 0);

	BOOL ProcessFirst(PPROCESSENTRY32 ppe) const;
	BOOL ProcessNext(PPROCESSENTRY32 ppe) const;
	BOOL ProcessFind(DWORD dwProcessId, PPROCESSENTRY32 ppe) const;
	BOOL ThreadFirst(PTHREADENTRY32 pte) const;
	BOOL ThreadNext(PTHREADENTRY32 pte) const;
};

#define OB_TYPE_INDEX_TYPE 1 // [ObjT] "Type"
#define OB_TYPE_INDEX_DIRECTORY 2 // [Dire] "Directory"
#define OB_TYPE_INDEX_SYMBOLIC_LINK 3 // [Symb] "SymbolicLink"
#define OB_TYPE_INDEX_TOKEN 4 // [Toke] "Token"
#define OB_TYPE_INDEX_PROCESS 5 // [Proc] "Process"
#define OB_TYPE_INDEX_THREAD 6 // [Thre] "Thread"
#define OB_TYPE_INDEX_JOB 7 // [Job ] "Job"
#define OB_TYPE_INDEX_EVENT 8 // [Even] "Event"
#define OB_TYPE_INDEX_EVENT_PAIR 9 // [Even] "EventPair"
#define OB_TYPE_INDEX_MUTANT 10 // [Muta] "Mutant"
#define OB_TYPE_INDEX_CALLBACK 11 // [Call] "Callback"
#define OB_TYPE_INDEX_SEMAPHORE 12 // [Sema] "Semaphore"
#define OB_TYPE_INDEX_TIMER 13 // [Time] "Timer"
#define OB_TYPE_INDEX_PROFILE 14 // [Prof] "Profile"
#define OB_TYPE_INDEX_WINDOW_STATION 15 // [Wind] "WindowStation"
#define OB_TYPE_INDEX_DESKTOP 16 // [Desk] "Desktop"
#define OB_TYPE_INDEX_SECTION 17 // [Sect] "Section"
#define OB_TYPE_INDEX_KEY 18 // [Key ] "Key"
#define OB_TYPE_INDEX_PORT 19 // [Port] "Port"
#define OB_TYPE_INDEX_WAITABLE_PORT 20 // [Wait] "WaitablePort"
#define OB_TYPE_INDEX_ADAPTER 21 // [Adap] "Adapter"
#define OB_TYPE_INDEX_CONTROLLER 22 // [Cont] "Controller"
#define OB_TYPE_INDEX_DEVICE 23 // [Devi] "Device"
#define OB_TYPE_INDEX_DRIVER 24 // [Driv] "Driver"
#define OB_TYPE_INDEX_IO_COMPLETION 25 // [IoCo] "IoCompletion"
#define OB_TYPE_INDEX_FILE 26 // [File] "File"
#define OB_TYPE_INDEX_WMI_GUID 27 // [WmiG] "WmiGuid"