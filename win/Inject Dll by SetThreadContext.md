# Inject Dll by SetThreadContext

## Debugging in WinDBG

```bash
0:004> dt _PEB @$peb
ntdll!_PEB
   +0x000 InheritedAddressSpace : 0 ''
   +0x001 ReadImageFileExecOptions : 0 ''
   +0x002 BeingDebugged    : 0x1 ''
   +0x003 BitField         : 0x4 ''
   +0x003 ImageUsesLargePages : 0y0
   +0x003 IsProtectedProcess : 0y0
   +0x003 IsImageDynamicallyRelocated : 0y1
   +0x003 SkipPatchingUser32Forwarders : 0y0
   +0x003 IsPackagedProcess : 0y0
   +0x003 IsAppContainer   : 0y0
   +0x003 IsProtectedProcessLight : 0y0
   +0x003 IsLongPathAwareProcess : 0y0
   +0x004 Mutant           : 0xffffffff Void
   +0x008 ImageBaseAddress : 0x00fd0000 Void
   +0x00c Ldr              : 0x77a35d80 _PEB_LDR_DATA // HERE!!!
   +0x010 ProcessParameters : 0x02191e88 _RTL_USER_PROCESS_PARAMETERS
```

PEB结构偏移为0xC的位置是_PEB_LDR_DATA结构的指针

```bash
0:004> dt _PEB_LDR_DATA poi(@$peb+0xc)
ntdll!_PEB_LDR_DATA
   +0x000 Length           : 0x30
   +0x004 Initialized      : 0x1 ''
   +0x008 SsHandle         : (null) 
   +0x00c InLoadOrderModuleList : _LIST_ENTRY [ 0x21939e8 - 0x21d6800 ]
   +0x014 InMemoryOrderModuleList : _LIST_ENTRY [ 0x21939f0 - 0x21d6808 ]
   +0x01c InInitializationOrderModuleList : _LIST_ENTRY [ 0x21938f0 - 0x21d6810 ]
   +0x024 EntryInProgress  : (null) 
   +0x028 ShutdownInProgress : 0 ''
   +0x02c ShutdownThreadId : (null) 

```
在偏移0x00c, 0x014, 0x01c处分别可以看到三个LIST_ENTRY

> LIST_ENTRY InLoadOrderModuleList; //按加载顺序
> LIST_ENTRY InMemoryOrderModuleList; //按内存顺序
> LIST_ENTRY InInitializationOrderModuleList; //按初始化顺序

**注：这里的类型是LIST_ENTRY，不是指针**

这里InInitializationOrderModuleList的第一个为ntdll.dll的基址。

InInitializationOrderModuleList里的内容为

```bash
0:004> dt _LIST_ENTRY poi(@$peb+0xc)+0x1c
ntdll!_LIST_ENTRY
 [ 0x21938f0 - 0x21d6810 ]
   +0x000 Flink            : 0x021938f0 _LIST_ENTRY [ 0x2194458 - 0x77a35d9c ]
   +0x004 Blink            : 0x021d6810 _LIST_ENTRY [ 0x77a35d9c - 0x21cd7b0 ]

```

InInitializationOrderModuleList是LIST_ENTRY的Head

LIST_ENTRY的使用方式参考[3]。

特别要注意CONTAINING_RECORD宏的使用，LIST_ENTRY的偏移计算结构体的起始地址。

由上面可知第一个元素就是InInitializationOrderModuleList->Flink

这里InInitializationOrderModuleList->Flink = poi(@$peb+0xc)+0x1c+0x0

Flink/Blink是个地址

```bash
0:004> dt _LDR_DATA_TABLE_ENTRY
ntdll!_LDR_DATA_TABLE_ENTRY
   +0x000 InLoadOrderLinks : _LIST_ENTRY
   +0x008 InMemoryOrderLinks : _LIST_ENTRY
   +0x010 InInitializationOrderLinks : _LIST_ENTRY
   +0x018 DllBase          : Ptr32 Void
   +0x01c EntryPoint       : Ptr32 Void
   +0x020 SizeOfImage      : Uint4B

```
从上面输出可知InInitializationOrderModuleList在LIST_ENTRY中的偏移为0x10的地方

```bash
0:004> dt _LDR_DATA_TABLE_ENTRY poi(poi(@$peb+0xc)+0x1c+0x0)-0x10
ntdll!_LDR_DATA_TABLE_ENTRY
   +0x000 InLoadOrderLinks : _LIST_ENTRY [ 0x2194078 - 0x21939e8 ]
   +0x008 InMemoryOrderLinks : _LIST_ENTRY [ 0x2194080 - 0x21939f0 ]
   +0x010 InInitializationOrderLinks : _LIST_ENTRY [ 0x2194458 - 0x77a35d9c ]
   +0x018 DllBase          : 0x77910000 Void
   +0x01c EntryPoint       : (null) 
   +0x020 SizeOfImage      : 0x1a4000
   +0x024 FullDllName      : _UNICODE_STRING "C:\Windows\SYSTEM32\ntdll.dll"
   +0x02c BaseDllName      : _UNICODE_STRING "ntdll.dll"
```

如上面所示

DllBase = poi(poi(@$peb+0xc)+0x1c+0x0)-0x10+0x18

合并之后：poi(poi(@$peb+0xc)+0x1c)+0x8

BaseDllName类型
```bash
0:004> dt _UNICODE_STRING
ntdll!_UNICODE_STRING
   +0x000 Length           : Uint2B
   +0x002 MaximumLength    : Uint2B
   +0x004 Buffer           : Ptr32 Wchar
```
Buffer是存放字符串的地方

所以BaseDllName = poi(poi(@$peb+0xc)+0x1c+0x0)-0x10+0x2c+0x4

合并之后：poi(poi(@$peb+0xc)+0x1c)+0x20

```asm
mov eax,dword ptr fs:[30] // PEB
mov eax,dword ptr ds:[eax+C]
mov edx,dword ptr ds:[eax+1C]
mov eax,edx
mov ecx,dword ptr ds:[eax+20] // ecx是BaseDllName(wchar_t)
mov esi,dword ptr ds:[eax+8] // esi是DllBase
```

## 实现代码

*详情见同目录下的CInjection.h/cpp文件*

## Reference

[1] [windbg学习23（!peb和PEB结构）](http://t.zoukankan.com/hgy413-p-3693439.html)

[2] [枚举windows进程模块的几种方法—PEB内核结构详解](https://blog.csdn.net/kongguoqing791025/article/details/121408205)

[3] [LIST_ENTRY链表学习](https://blog.csdn.net/hjxyshell/article/details/38742117)

[4] [Windows中FS段寄存器](https://blog.csdn.net/misterliwei/article/details/1655290)

[5] [Win32之隐藏DLL隐藏模块技术](http://t.zoukankan.com/iBinary-p-9601860.html)

[6] [Understanding the PEB Loader Data Structure](http://sandsprite.com/CodeStuff/Understanding_the_Peb_Loader_Data_List.html)

[7] [PEB及LDR链](https://www.cnblogs.com/gd-luojialin/p/11862767.html)