#include <Windows.h>
#include <wchar.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <iterator>
#include <stdio.h>
#include <stdint.h>
#include <tlhelp32.h>

#define DSIISCANSIZE 0x15D5000
#define DSIISOTFSSCANSIZE 0x72F000

class CharInstances {
public:
    DWORD currentHP = 0;
    DWORD lastHP = 0;
    DWORD maxHP = 0;
    DWORD offset = 0;
    int active = 0;
};

class baseReroute {
public:
    void init(wchar_t PName[]) {
        wcscpy(PrName, PName);
    }
    int attachProc();

    virtual void setAddresses();
    virtual void injectRoutine() {};
    virtual void getStats() {};
    virtual int injAssert(char assertChar, UINT64 Addr) { return 0; };

    UINT64 HookAddr;
    char InjAssertChar = 0;
    UINT64 BaseAddress = 0;
protected:
    // Proccess name
    wchar_t PrName[20];
    // Process variables
    DWORD PID = 0;
    HANDLE hProcess = 0;
    // Address for base of the module and for where the damage is stored
    UINT64 DmgAddress = 0;
    // Address of HP (technically I think this is the base player structure address)
    UINT64 CharAddress = 0;
    // Multi-level pointer variables for HP and death count
    UINT64 finalHP = 0;
    UINT64 finalMaxHP = 0;
    UINT64 finalDeath = 0;
    unsigned int resultHP = 0;
    unsigned int resultDMG = 0;
    unsigned int resultDeath = 0;

    int firstRunFlag = 0;

    // Function to get the PID of requested proccess
    DWORD get_PID(wchar_t PrName[]);
    // Function to get the address of the base module.
    UINT64 GetModuleBase(wchar_t lpModuleName[], DWORD dwProcessId);
    // Function that obtains the address of a multi-level pointer
    UINT64 CalcPtr_Ext(HANDLE hProc, UINT64 base, DWORD offs[], int lvl, int bit);

    UINT64 FindPattern(std::byte *data, UINT64 pBaseAddress, std::byte* pbMask, const char* pszMask, DWORD nLength);

private:
    //byte* FindPattern(HANDLE hProc, byte* pBaseAddress, byte* pbMask, const char* pszMask, size_t nLength);
};

class dsiiReroute : public baseReroute {
public:
    void injectRoutine() override;
    void getStats() override;
    int injAssert(char assertChar, UINT64 Addr) override;
    void setAddresses() override;

    double dmgRecStr;
    double dmgDealtStr;
    double dmgDealThresh = 1;

    signed int lastHP;
    unsigned int lastDeathCnt;

    signed long int currEnHP;
    signed long int newEnHP;
    signed long int maxEnHP;

private:
    unsigned int stats[3] = { 0 };

    const char* iCode1 = "\xA1";
    const char* iCode2 = "\x39\xC6\x74\x1C\x8B\x86\xFC\x00\x00\x00\xA3";
    const char* iCode3 = "\x89\x0D";
    const char* iCode4 = "\x8B\x86\x04\x01\x00\x00\xA3";
    const char* iCode5 = "\x89\x8E\xFC\x00\x00\x00\xC3";
    const char* iCode6 = "\xFF\xE0";

    DWORD HpOffsets[2] = { 0x74, 0xFC };
    DWORD MHpOffsets[2] = { 0x74, 0x104 };
    DWORD DeathOffsets[3] = { 0x74, 0x378, 0x1A0 };

};

class dsiisotfsReroute : public dsiiReroute {
public:
    void setAddresses() override;
    void injectRoutine() override;

    //const char InjAssertChar = 0x49;

private:

    DWORD HpOffsets[2] = { 0xD0, 0x168 };
    DWORD MHpOffsets[2] = { 0xD0, 0x170 };
    DWORD DeathOffsets[3] = { 0xD0, 0x268, 0x364 };
};