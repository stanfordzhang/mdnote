#include "Injection.h"

#include <Windows.h>
#include <iostream>
#include <stdlib.h>

DWORD GetKernel32ImageBase() {
    PLIST_ENTRY Head, Cur;
    PPEB_LDR_DATA ldr;
    PLDR_MODULE ldm;
    __asm
    {
        mov eax, dword ptr fs : [0x30] // PEB
        mov ecx, [eax + 0x0c] //Ldr                     //��ȡ_PEB_LDR_DATA�ṹ
        mov ldr, ecx
    }
    Head = &(ldr->InInitializationOrderModuleList);               //��ȡģ��������ַ
    Cur = Head->Flink;                                  //��ȡָ��Ľ��.
    do
    {
        ldm = CONTAINING_RECORD(Cur, LDR_MODULE, InInitializationOrderModuleList); //��ȡ _LDR_DATA_TABLE_ENTRY�ṹ���ַ
        //ldm = (LDR_MODULE*)((PCHAR)(Cur)-(ULONG_PTR)(&((LDR_MODULE*)0)->InInitializationOrderModuleList));
        //printf("EntryPoint [0x%X]",ldm->BaseAddress);
        //wprintf(L"Full: %s, Base: %s\n", ldm->FullDllName.Buffer, ldm->BaseDllName.Buffer);
        OutputDebugStringW(L"\n");
        OutputDebugStringW(ldm->BaseDllName.Buffer);
        if (_wcsicmp(ldm->BaseDllName.Buffer, L"kernel32.dll") == 0) {
            return (DWORD)ldm->BaseAddress;
        }
        OutputDebugStringW(L"\n");
        Cur = Cur->Flink;
    } while (Head != Cur);

    return 0;
}

void InjectDll(HANDLE hProcess, HANDLE hThread, WCHAR *dll) {
    BYTE shellcode[0x58] = {};

    DWORD LoadLibraryWOffset = (DWORD)&LoadLibraryW - GetKernel32ImageBase();

    CONTEXT context = {};
    context.ContextFlags = CONTEXT_ALL;
    assert(GetThreadContext(hThread, &context));

    LPVOID param = VirtualAllocEx(hProcess, NULL, 0x3A4, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    assert(param);
    assert(WriteProcessMemory(hProcess, param, (LPCVOID)dll, (wcslen(dll) + 1) * sizeof(WCHAR), 0));
    DWORD LoadLibraryWParamAddress = (DWORD)param;

    shellcode[0] = 0x68; // 07B30000 | 68 B0503E77 | push <ntdll.RtlUserThreadStart> |
    *(DWORD*)(shellcode + 1) = context.Eip;
    shellcode[5] = 0x9C; // 07B30005 | 9C | pushfd |
    shellcode[6] = 0x60; // 07B30006 | 60 | pushad |
    shellcode[7] = 0x64; // 07B30007 | 64:A1 30000000 | mov eax, dword ptr fs : [30] |
    shellcode[8] = 0xA1;
    *(DWORD*)(shellcode + 9) = 0x30;
    shellcode[13] = 0x3E; // 07B3000D | 3E : 8B40 0C | mov eax, dword ptr ds : [eax + C] |
    shellcode[14] = 0x8B;
    shellcode[15] = 0x40;
    shellcode[16] = 0x0C;
    shellcode[17] = 0x3E; // 07B30011 | 3E : 8B50 1C | mov edx, dword ptr ds : [eax + 1C] | ѭ����kernel32.dll�Ļ�ַ
    shellcode[18] = 0x8B;
    shellcode[19] = 0x50;
    shellcode[20] = 0x1C;
    shellcode[21] = 0x8B; // 07B30015 | 8BC2 | mov eax, edx |
    shellcode[22] = 0xC2;
    shellcode[23] = 0x3E; // 07B30017 | 3E : 8B48 20 | mov ecx, dword ptr ds : [eax + 20] |
    shellcode[24] = 0x8B;
    shellcode[25] = 0x48;
    shellcode[26] = 0x20;
    shellcode[27] = 0x3E; // 07B3001B | 3E : 8B70 08 | mov esi, dword ptr ds : [eax + 8] | [eax + 8] : "���u"
    shellcode[28] = 0x8B;
    shellcode[29] = 0x70;
    shellcode[30] = 0x08;
    shellcode[31] = 0x85; // 07B3001F | 85C9 | test ecx, ecx |
    shellcode[32] = 0xC9;
    shellcode[33] = 0x74; // 07B30021 | 74 08 | je 7B3002B |
    shellcode[34] = 0x08;
    shellcode[35] = 0x66; // 07B30023 | 663E:8379 18 00 | cmp word ptr ds : [ecx + 18] , 0 | �ж�Kernel32.dll�ĳ���
    shellcode[36] = 0x3E;
    shellcode[37] = 0x83;
    shellcode[38] = 0x79;
    shellcode[39] = 0x18;
    shellcode[40] = 0x00;
    shellcode[41] = 0x74; // 07B30029 | 74 09 | je 7B30034 |
    shellcode[42] = 0x09;
    shellcode[43] = 0x3E; // 07B3002B | 3E:8B00 | mov eax, dword ptr ds : [eax] |
    shellcode[44] = 0x8B;
    shellcode[45] = 0x00;
    shellcode[46] = 0x3B; // 07B3002E | 3BD0 | cmp edx, eax |
    shellcode[47] = 0xD0;
    shellcode[48] = 0x75; // 07B30030 | 75 E5 | jne 7B30017 |
    shellcode[49] = 0xE5;
    shellcode[50] = 0xEB; // 07B30032 | EB 02 | jmp 7B30036 |
    shellcode[51] = 0x02;
    shellcode[52] = 0x8B; // 07B30034 | 8BDE | mov ebx, esi |
    shellcode[53] = 0xDE;
    shellcode[54] = 0xB8; // 07B30036 | B8 C0160200 | mov eax, 216C0 | ����0x216c0����LoadLibraryW�ĵ�ַ
    *(DWORD*)(shellcode + 55) = LoadLibraryWOffset;
    shellcode[59] = 0x03; // 07B3003B | 03C3 | add eax, ebx |
    shellcode[60] = 0xC3;
    shellcode[61] = 0xE8; // 07B3003D | E8 00000000 | call 7B30042 | call $0 == PUSH EIP
    *(DWORD*)(shellcode + 62) = 0x0;
    shellcode[66] = 0x59; // 07B30042 | 59 | pop ecx | ecx = 07B30042
    shellcode[67] = 0x81; // 07B30043 | 81C1 0E000000 | add ecx, E | ecx = 07B30042 + 0xE = 07B30050
    shellcode[68] = 0xC1;
    *(DWORD*)(shellcode + 69) = 0xE;
    shellcode[73] = 0xFF; // 07B30049 | FF31 | push dword ptr ds : [ecx] |
    shellcode[74] = 0x31;
    shellcode[75] = 0xFF; // 07B3004B | FFD0 | call eax |
    shellcode[76] = 0xD0;
    shellcode[77] = 0x61; // 07B3004D | 61 | popad |
    shellcode[78] = 0x9D; // 07B3004E | 9D | popfd |
    shellcode[79] = 0xC3; // 07B3004F | C3 | ret | ret ֮�����ntdll.RtlUserThreadStart
    *(DWORD*)(shellcode + 80) = LoadLibraryWParamAddress; // 07B30050 | 0000B207 | Param for LoadLibraryW |

    LPVOID shellmem = VirtualAllocEx(hProcess, NULL, 0x58, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    assert(shellmem);
    assert(WriteProcessMemory(hProcess, shellmem, shellcode, 0x58, NULL));

    FlushInstructionCache(hProcess, shellmem, 0x58);
    context.Eip = (DWORD)shellmem;
    assert(SetThreadContext(hThread, &context));
    WaitForMultipleObjects(0, &hProcess, TRUE, INFINITE);
}

void InjectDllWhenCreatingProcess(WCHAR *ExeFullPath, WCHAR *DllFullPath) {
    STARTUPINFOW startupInfo = {};
    PROCESS_INFORMATION processInformation = {};

    if (CreateProcessW(NULL, ExeFullPath, 0, 0, 0, CREATE_SUSPENDED, 0, 0, &startupInfo, &processInformation)) {

        InjectDll(processInformation.hProcess, processInformation.hThread, DllFullPath);

        ResumeThread(processInformation.hThread);
    }
}
