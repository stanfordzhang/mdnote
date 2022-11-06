# Inject Dll by SetThreadContext

## Debugging in WinDBG

```bash
dt _PEB @$peb
```
![](https://github.com/stanfordzhang/mdnote/blob/main/win/1.png)

PEB结构偏移为0xC的位置是_PEB_LDR_DATA结构的指针

```bash
dt _PEB_LDR_DATA poi(@$peb+0xc)
```
![](https://github.com/stanfordzhang/mdnote/blob/main/win/2.png)
上图中可以看到三个LIST_ENTRY

> LIST_ENTRY InLoadOrderModuleList; //按加载顺序
> LIST_ENTRY InMemoryOrderModuleList; //按内存顺序
> LIST_ENTRY InInitializationOrderModuleList; //按初始化顺序

**注：这里的类型是LIST_ENTRY，不是指针**

这里InInitializationOrderModuleList的第一个为ntdll.dll的基址。

InInitializationOrderModuleList里的内容为

```bash
dt _LIST_ENTRY poi(@$peb+0xc)+0x1c
```
![](https://github.com/stanfordzhang/mdnote/blob/main/win/3.png)

InInitializationOrderModuleList是LIST_ENTRY的Head

LIST_ENTRY的使用方式参考[3]。

特别要注意CONTAINING_RECORD宏的使用，LIST_ENTRY的偏移计算结构体的起始地址。

由上图可知第一个元素就是InInitializationOrderModuleList->Flink

这里InInitializationOrderModuleList->Flink = poi(@$peb+0xc)+0x1c+0x0

Flink/Blink是个地址

```bash
dt _LDR_DATA_TABLE_ENTRY
```
![](https://github.com/stanfordzhang/mdnote/blob/main/win/4.png)
由上图可知这里InInitializationOrderModuleList在LIST_ENTRY中的偏移为0x10的地方

```bash
dt _LDR_DATA_TABLE_ENTRY poi(poi(@$peb+0xc)+0x1c+0x0)-0x10
```
![](https://github.com/stanfordzhang/mdnote/blob/main/win/5.png)

由上图可以看到

DllBase = poi(poi(@$peb+0xc)+0x1c+0x0)-0x10+0x18

合并之后：poi(poi(@$peb+0xc)+0x1c)+0x8

BaseDllName类型
![](https://github.com/stanfordzhang/mdnote/blob/main/win/6.png)
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

## Reference

[1] [windbg学习23（!peb和PEB结构）](http://t.zoukankan.com/hgy413-p-3693439.html)

[2] [枚举windows进程模块的几种方法—PEB内核结构详解](https://blog.csdn.net/kongguoqing791025/article/details/121408205)

[3] [LIST_ENTRY链表学习](https://blog.csdn.net/hjxyshell/article/details/38742117)

[4] [Windows中FS段寄存器](https://blog.csdn.net/misterliwei/article/details/1655290)

[5] [Win32之隐藏DLL隐藏模块技术](http://t.zoukankan.com/iBinary-p-9601860.html)

[6] [Understanding the PEB Loader Data Structure](http://sandsprite.com/CodeStuff/Understanding_the_Peb_Loader_Data_List.html)

[7] [PEB及LDR链](https://www.cnblogs.com/gd-luojialin/p/11862767.html)