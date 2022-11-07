#pragma once

typedef struct _UNICODE_STRING
{
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING, * PUNICODE_STRING;

typedef struct _PEB_LDR_DATA {
    ULONG                   Length;
    BOOLEAN                 Initialized;
    PVOID                   SsHandle;
    LIST_ENTRY              InLoadOrderModuleList;
    LIST_ENTRY              InMemoryOrderModuleList;
    LIST_ENTRY              InInitializationOrderModuleList;
} PEB_LDR_DATA, * PPEB_LDR_DATA;

typedef struct _LDR_MODULE
{
    LIST_ENTRY          InLoadOrderModuleList;   //+0x00
    LIST_ENTRY          InMemoryOrderModuleList; //+0x08  
    LIST_ENTRY          InInitializationOrderModuleList; //+0x10
    void* BaseAddress;  //+0x18
    void* EntryPoint;   //+0x1c
    ULONG               SizeOfImage;
    UNICODE_STRING      FullDllName;
    UNICODE_STRING      BaseDllName;
    ULONG               Flags;
    SHORT               LoadCount;
    SHORT               TlsIndex;
    HANDLE              SectionHandle;
    ULONG               CheckSum;
    ULONG               TimeDateStamp;
} LDR_MODULE, * PLDR_MODULE;

DWORD GetKernel32ImageBase();
void InjectDll(HANDLE hProcess, HANDLE hThread, WCHAR* dll);
void InjectDllWhenCreatingProcess(WCHAR* ExeFullPath, WCHAR* DllFullPath);
