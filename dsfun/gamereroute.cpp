#include "gamereroute.h"
#define strlens(s) (s==NULL?0:strlen(s))

int baseReroute::attachProc() {
    // Find PID of requested process
    if (!(PID = get_PID(PrName)))
    {
        printf("Process does not exist\n");
        return 1;
    }
    printf("Process found!\n");
    printf("PID: %d\n\n", PID);

    // Open requested process and get its handle
    if (!(hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, PID)))
    {
        auto test = GetLastError();
        printf("OpenProcess error\n");
        return 1;
    }

    printf("OpenProcess is ok\n");

    // Get the address for the module base
    if (!(BaseAddress = GetModuleBase(PrName, PID)))
    {
        printf("GetModuleBase error\n");
        return 1;
    }
    printf("GetModuleBase is ok\n");
    return 0;
}

// Function to get the PID of requested proccess
DWORD baseReroute::get_PID(wchar_t PrName[])
{
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    if (Process32First(snapshot, &entry) == TRUE)
    {
        while (Process32Next(snapshot, &entry) == TRUE)
        {
            //printf("%s\n", entry.szExeFile);
            if (wcscmp(entry.szExeFile, PrName) == 0)
            {
                return entry.th32ProcessID;
            }
        }
    }
    CloseHandle(snapshot);
    return NULL;
}

// Function to get the address of the base module.
UINT64 baseReroute::GetModuleBase(wchar_t lpModuleName[], DWORD dwProcessId)
{
    MODULEENTRY32 lpModuleEntry = { 0 };
    HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwProcessId);

    if (!hSnapShot)
        return NULL;
    lpModuleEntry.dwSize = sizeof(lpModuleEntry);
    BOOL bModule = Module32First(hSnapShot, &lpModuleEntry);
    while (bModule)
    {
        if (!wcscmp(lpModuleEntry.szModule, lpModuleName))
        {
            CloseHandle(hSnapShot);
            return (UINT64)lpModuleEntry.modBaseAddr;
        }
        bModule = Module32Next(hSnapShot, &lpModuleEntry);
    }
    CloseHandle(hSnapShot);
    return NULL;
}

// Function that obtains the address of a multi-level pointer
UINT64 baseReroute::CalcPtr_Ext(HANDLE hProc, UINT64 base, DWORD offs[], int lvl, int bit)
{
    UINT64 product = base;
    for (int i = 0; i < lvl; ++i)
    {
        if(!bit)
            ReadProcessMemory(hProc, (LPCVOID)product, &product, sizeof(DWORD), 0);
        else
            ReadProcessMemory(hProc, (LPCVOID)product, &product, sizeof(UINT64), 0);
        product += offs[i];
    }
    return product;
}

UINT64 baseReroute::FindPattern(std::byte *data, UINT64 pBaseAddress, std::byte pbMask[], const char* pszMask, DWORD nLength)
{
    auto DataCompare = [](std::byte* data, const std::byte* mask, const char* cmask, std::byte chLast, std::size_t iEnd) -> bool {
        unsigned int iData = 0;
        if (data[iEnd - 1] != chLast) return false;
        for (; *cmask; ++cmask, ++iData) {
            if (*cmask == 'x' && data[iData] != mask[iData]) {
                return false;
            }
        }

        return true;
    };

    UINT64 stAdrress = pBaseAddress;

    auto iEnd = strlen(pszMask);
    std::byte chLast = pbMask[iEnd - 1];

    auto pEnd = pBaseAddress + nLength - strlen(pszMask);
    //std::byte data[256];
    for (; pBaseAddress < pEnd; ++pBaseAddress) {
        if (pBaseAddress == stAdrress + 0x40E284)
            printf("");
        //ReadProcessMemory(hProc, (LPCVOID)pBaseAddress, data, iEnd, 0);
        if (DataCompare(data, pbMask, pszMask, chLast, iEnd)) {
            return pBaseAddress;
        }
        data++;
    }

    return 0;
}

void baseReroute::setAddresses() {
    // Get the addresses of the multi-level pointer variables
    //finalHP = CalcPtr_Ext(hProcess, CharAddress, HpOffsets.data(), HpOffsets.size());
    //finalMaxHP = CalcPtr_Ext(hProcess, CharAddress, MHpOffsets.data(), MHpOffsets.size());
    //finalDeath = CalcPtr_Ext(hProcess, CharAddress, DeathOffsets.data(), DeathOffsets.size());
}

UINT64 baseReroute::AllocNearbyMemory(HANDLE hProc, UINT64 nearThisAddr)
{
    UINT64 begin = nearThisAddr;
    UINT64 end = nearThisAddr + 0x7FFF0000;
    MEMORY_BASIC_INFORMATION mbi{};

    auto curr = begin;

    while (VirtualQueryEx(hProc, (LPCVOID)curr, &mbi, sizeof(mbi)))
    {
        if (mbi.State == MEM_FREE)
        {
            UINT64 addr = (UINT64)VirtualAllocEx(hProc, mbi.BaseAddress, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
            if (addr) return addr;
        }
        curr += mbi.RegionSize;
    }

    return 0;
}

void dsiiReroute::setAddresses() {
    const char* patbuf = "\x8B\xF1\x8B\x0D\x00\x00\x00\x01\x8B\x01\x8B\x50\x28\xFF\xD2\x84\xC0\x74\x0C";
    const char* patbuf2 = "\x89\x8E\xFC\x00\x00\x00\x8B\x02\x50";
    std::byte* pbPattern = (std::byte*)patbuf;
    std::byte* pbPattern2 = (std::byte*)patbuf2;
    std::byte* data = new std::byte[DSIISCANSIZE];
    SIZE_T readAmt = 0;
    DWORD totalRead = 0;

    ReadProcessMemory(hProcess, (LPCVOID)BaseAddress, data, DSIISCANSIZE, &readAmt);
    totalRead += readAmt;
    while (totalRead < DSIISCANSIZE) {
        ReadProcessMemory(hProcess, (LPCVOID)(BaseAddress + totalRead), data + totalRead, DSIISCANSIZE - totalRead, &readAmt);
        if (readAmt)
            totalRead += readAmt;
        else totalRead++;
    }

    const char* pszMask = "xxxx???xxxxxxxxxxxx";
    const char* pszMask2 = "xxxxxxxxx";
    DWORD pAddress = (DWORD)FindPattern(data, BaseAddress, pbPattern, pszMask, DSIISCANSIZE) + 4;
    HookAddr = FindPattern(data, BaseAddress, pbPattern2, pszMask2, DSIISCANSIZE);
    if (!HookAddr) {
        const char* patbuf3 = "\xE8\x00\x00\x00\x00\x90\x8B\x02\x50";
        std::byte* pbPattern3 = (std::byte*)patbuf3;
        const char* pszMask3 = "x????xxxx";
        HookAddr = FindPattern(data, BaseAddress, pbPattern3, pszMask3, DSIISCANSIZE);
    }

    ReadProcessMemory(hProcess, (LPCVOID)pAddress, &CharAddress, 4, 0);
    finalHP = CalcPtr_Ext(hProcess, CharAddress, HpOffsets, sizeof(HpOffsets)/sizeof(HpOffsets[0]), 0);
    finalMaxHP = CalcPtr_Ext(hProcess, CharAddress, MHpOffsets, sizeof(MHpOffsets) / sizeof(MHpOffsets[0]), 0);
    finalDeath = CalcPtr_Ext(hProcess, CharAddress, DeathOffsets, sizeof(DeathOffsets) / sizeof(DeathOffsets[0]), 0);
    delete[] data;
}

void dsiiReroute::injectRoutine() {
    int injLen = 0;
    // This is shellcode that replaces a bit of code in base module for rerouting (pop and ret for absolute jump)
    char HookShellcode[8] = { 0 };
    // This is the code that will be executed after rerouting.
    char InjectShellcode[128] = { 0 };
    // Allocate memory for injected code
    DWORD pInjectedFunction = (DWORD)VirtualAllocEx(hProcess, NULL, 128, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    // Allocate memory for a variable (damage)
    DWORD pVar = (DWORD)VirtualAllocEx(hProcess, NULL, 12, MEM_COMMIT, PAGE_READWRITE);
    // Allocate memory for base pointer address
    DWORD bPointerAddress = (DWORD)VirtualAllocEx(hProcess, NULL, 4, MEM_COMMIT, PAGE_READWRITE);

    // This is a pop code
    memcpy(HookShellcode, "\xe8", 1);
    // Copy address to where you want the jump to be. Memcpy used so as to make little endian
    DWORD relJump = pInjectedFunction - HookAddr - 5;
    memcpy(HookShellcode + 1, &relJump, sizeof(DWORD));
    // A ret
    memcpy(HookShellcode + 5, "\x90", 1);

    // This generates a mov with appropriate variable address and a pop and return back to base module.
    // Yes you can do this with a for loop probably, but I want to be careful with injections.
    memcpy(InjectShellcode, iCode1, 1);
    injLen += 1;
    memcpy(InjectShellcode + injLen, &bPointerAddress, sizeof(DWORD));
    injLen += sizeof(DWORD);
    memcpy(InjectShellcode + injLen, iCode2, 11);
    injLen += 11;
    memcpy(InjectShellcode + injLen, &pVar, sizeof(DWORD));
    injLen += sizeof(DWORD);
    memcpy(InjectShellcode + injLen, iCode3, 2);
    injLen += 2;
    pVar += 0x04;
    memcpy(InjectShellcode + injLen, &pVar, sizeof(DWORD));
    injLen += sizeof(DWORD);
    memcpy(InjectShellcode + injLen, iCode4, 7);
    injLen += 7;
    pVar += 0x04;
    memcpy(InjectShellcode + injLen, &pVar, sizeof(DWORD));
    injLen += sizeof(DWORD);
    memcpy(InjectShellcode + injLen, iCode5, 7);
    //injLen += 7;
    //HookAddr += 0x06;
    //memcpy(InjectShellcode + injLen, &HookAddr, sizeof(DWORD));
    //HookAddr -= 0x06;
    //injLen += sizeof(DWORD);
    //memcpy(InjectShellcode + injLen, iCode6, 2);
    //injLen += strlen(iCode5);

    // Inject the code
    DWORD playerOffs[] = { 0x74, 0x00 };
    DWORD playerPointer = CalcPtr_Ext(hProcess, CharAddress, playerOffs, 2, 0);
    WriteProcessMemory(hProcess, (LPVOID)(pInjectedFunction), InjectShellcode, sizeof(InjectShellcode), 0);
    WriteProcessMemory(hProcess, (LPVOID)(bPointerAddress), &playerPointer, sizeof(InjectShellcode), 0);
    WriteProcessMemory(hProcess, (LPVOID)(HookAddr), HookShellcode, sizeof(HookShellcode)-2, 0);
    // Return the address of damage variable.
    std::ofstream ofile("address.txt", std::ofstream::trunc);
    ofile << pVar;
    ofile.close();
    DmgAddress = pVar-8;
}

int dsiiReroute::injAssert(char assertChar, UINT64 Addr) {
    unsigned char resAssert = 0;
    // check first whether the code is already injected
    ReadProcessMemory(hProcess, (LPCVOID)(Addr), &resAssert, 1, 0);
    if (resAssert != assertChar) return 0;
    else {
        std::ifstream ifile("address.txt");
        std::string addr;
        std::getline(ifile, addr);
        const char* str = addr.c_str();
        DmgAddress = strtoul(str, NULL, 0);
        return 1;
    }
}

void dsiiReroute::getStats() {
    ReadProcessMemory(hProcess, (LPCVOID)finalHP, stats, sizeof(int), 0);
    ReadProcessMemory(hProcess, (LPCVOID)finalMaxHP, stats + 1, sizeof(int), 0);
    ReadProcessMemory(hProcess, (LPCVOID)finalDeath, stats + 2, sizeof(int), 0);

    if (!firstRunFlag) {
        lastHP = stats[0];
        lastDeathCnt = stats[2];
        firstRunFlag = 1;
    }
    else {
        if (lastHP != stats[0]) {
            int dif = lastHP - stats[0];
            lastHP = stats[0];
            if (dif > 0) {
                if (stats[2] - lastDeathCnt > 0) {
                    dmgRecStr = 1;
                    lastDeathCnt = stats[2];
                }
                else if (stats[2] - lastDeathCnt == 0 && stats[0] == 0)
                    dmgRecStr = 0;
                else dmgRecStr = (double)dif / (double)stats[1];
            }
            else dmgRecStr = 0;
        }
        else dmgRecStr = 0;
    }

    static signed long int lastCurrHP = 0;
    static signed long int lastNewHP = 0;
    static signed long int lastMaxHP = 0;
    ReadProcessMemory(hProcess, (LPCVOID)DmgAddress, &currEnHP, sizeof(DWORD), 0);
    ReadProcessMemory(hProcess, (LPCVOID)(DmgAddress+4), &newEnHP, sizeof(DWORD), 0);
    ReadProcessMemory(hProcess, (LPCVOID)(DmgAddress+8), &maxEnHP, sizeof(DWORD), 0);

    if (currEnHP > newEnHP && currEnHP != lastCurrHP && newEnHP != lastNewHP) {
        dmgDealtStr = (double)(currEnHP - newEnHP) / (float)maxEnHP;
        lastCurrHP = currEnHP;
        lastNewHP = newEnHP;
        lastMaxHP = maxEnHP;
    }
    else dmgDealtStr = 0;
}

void dsiisotfsReroute::setAddresses() {
    const char* patbuf = "\x48\x8B\x05\x00\x00\x00\x00\x48\x8B\x58\x38\x48\x85\xDB\x74\x00\xF6";
    const char* patbuf2 = "\x89\x83\x68\x01\x00\x00\x49\x8B\x0E\xE8\x00\x00\x00\x00\x84\xC0";
    std::byte* pbPattern = (std::byte*)patbuf;
    std::byte* pbPattern2 = (std::byte*)patbuf2;
    std::byte* data = new std::byte[DSIISOTFSSCANSIZE];
    SIZE_T readAmt = 0;
    UINT64 totalRead = 0;

    ReadProcessMemory(hProcess, (LPCVOID)BaseAddress, data, DSIISOTFSSCANSIZE, &readAmt);
    auto test = GetLastError();
    totalRead += readAmt;
    while (totalRead < DSIISOTFSSCANSIZE) {
        ReadProcessMemory(hProcess, (LPCVOID)(BaseAddress + totalRead), data + totalRead, DSIISOTFSSCANSIZE - totalRead, &readAmt);
        if (readAmt)
            totalRead += readAmt;
        else totalRead++;
    }

    const char* pszMask = "xxx????xxxxxxxx?x";
    const char* pszMask2 = "xxxxxxxxxx????xx";
    UINT64 pAddress = FindPattern(data, BaseAddress, pbPattern, pszMask, DSIISOTFSSCANSIZE);
    HookAddr = FindPattern(data, BaseAddress, pbPattern2, pszMask2, DSIISOTFSSCANSIZE);
    if (!HookAddr) {
        const char* patbuf3 = "\x49\xBA\x00\x00\x00\x00\x00\x00\x00\x00\x41\xFF\xD2";
        std::byte* pbPattern3 = (std::byte*)patbuf3;
        const char* pszMask3 = "xx????????xxx";
        HookAddr = FindPattern(data, BaseAddress, pbPattern3, pszMask3, DSIISOTFSSCANSIZE);
    }

    ReadProcessMemory(hProcess, (LPCVOID)(pAddress+3), &CharAddress, 4, 0); // +3 offsets the mov rax to grab relative address
    CharAddress = pAddress + 7 + CharAddress; // +7 offsets whole instruction and gets RIP since relative address to RIP
    finalHP = CalcPtr_Ext(hProcess, CharAddress, this->HpOffsets, sizeof(this->HpOffsets) / sizeof(this->HpOffsets[0]), 1);
    finalMaxHP = CalcPtr_Ext(hProcess, CharAddress, this->MHpOffsets, sizeof(this->MHpOffsets) / sizeof(this->MHpOffsets[0]), 1);
    finalDeath = CalcPtr_Ext(hProcess, CharAddress, this->DeathOffsets, sizeof(this->DeathOffsets) / sizeof(this->DeathOffsets[0]), 1);
    delete[] data;
}

void dsiisotfsReroute::injectRoutine() {
    int injLen = 0;
    // This is shellcode that replaces a bit of code in base module for rerouting (pop and ret for absolute jump)
    char HookShellcode[14] = { 0 };
    char OldCode[14] = { 0 };
    // This is the code that will be executed after rerouting.
    char InjectShellcode[128] = { 0 };
    // Allocate memory for injected code
    UINT64 pInjectedFunction = (UINT64)VirtualAllocEx(hProcess, NULL, 128, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    // Allocate memory for a variable (damage)
    UINT64 pVar = (UINT64)VirtualAllocEx(hProcess, NULL, 24, MEM_COMMIT, PAGE_READWRITE);
    // Allocate memory for base pointer address
    UINT64 bPointerAddress = (UINT64)VirtualAllocEx(hProcess, NULL, 8, MEM_COMMIT, PAGE_READWRITE);

    //UINT64 varsAddr = AllocNearbyMemory(hProcess, HookAddr);

    int totalWritten = 0;

    char iCode1[] = { 0x52, 0x51, 0x48, 0xB9 };
    char iCode2[] = { 0x48, 0x39, 0xCB, 0x74, 0x24, 0x48, 0xB9 };
    char iCode3[] = { 0x8B, 0x93, 0x68, 0x01, 0x00, 0x00, 0x89, 0x11, 0x48, 0x83, 0xC1, 0x04, 0x89, 0x01, 0x48, 0x83, 0xC1, 0x04, 0x8B, 0x93, 0x70, 0x01, 0x00, 0x00, 0x89, 0x11, 0x59, 0x5A };
    char iCode4[] = { 0xC3 };

    // This is a pop code
    memcpy(HookShellcode, "\x49\xba", 2);
    // Copy address to where you want the jump to be. Memcpy used so as to make little endian
    memcpy(HookShellcode + 2, &pInjectedFunction, sizeof(UINT64));
    // A ret
    memcpy(HookShellcode + 10, "\x41\xff\xd2\x90", 4);
    ReadProcessMemory(hProcess, (LPCVOID)(HookAddr), &OldCode, 14, 0);

    DWORD playerOffs[] = { 0xD0, 0x00 };
    UINT64 playerPointer = CalcPtr_Ext(hProcess, CharAddress, playerOffs, 2, 1);
    memcpy(InjectShellcode+totalWritten, iCode1, sizeof(iCode1));
    totalWritten += sizeof(iCode1);
    memcpy(InjectShellcode+totalWritten, &playerPointer, sizeof(UINT64));
    totalWritten += sizeof(UINT64);
    memcpy(InjectShellcode+totalWritten, iCode2, sizeof(iCode2));
    totalWritten += sizeof(iCode2);
    memcpy(InjectShellcode+totalWritten, &pVar, sizeof(UINT64));
    totalWritten += sizeof(UINT64);
    memcpy(InjectShellcode + totalWritten, iCode3, sizeof(iCode3));
    totalWritten += sizeof(iCode3);

    memcpy(InjectShellcode + totalWritten, OldCode, sizeof(OldCode)-5);
    totalWritten += sizeof(OldCode)-5;
    UINT64 oldCall = 0;
    memcpy(&oldCall, OldCode + 10, sizeof(DWORD));
    oldCall += HookAddr + 8 + 6;
    memcpy(InjectShellcode + totalWritten, "\x49\xba", 2);
    totalWritten += 2;
    memcpy(InjectShellcode + totalWritten, &oldCall, sizeof(UINT64));
    totalWritten += sizeof(UINT64);
    memcpy(InjectShellcode + totalWritten, "\x41\xff\xd2", 3);
    totalWritten += 3;

    memcpy(InjectShellcode + totalWritten, iCode4, sizeof(iCode4));
    totalWritten += sizeof(iCode4);

    // Inject the code
    WriteProcessMemory(hProcess, (LPVOID)(pInjectedFunction), InjectShellcode, sizeof(InjectShellcode), 0);
    //WriteProcessMemory(hProcess, (LPVOID)(bPointerAddress), &playerPointer, sizeof(InjectShellcode), 0);
    WriteProcessMemory(hProcess, (LPVOID)(HookAddr), HookShellcode, sizeof(HookShellcode), 0);
    // Return the address of damage variable.
    std::ofstream ofile("address.txt", std::ofstream::trunc);
    ofile << pVar;
    ofile.close();
    DmgAddress = pVar;
}