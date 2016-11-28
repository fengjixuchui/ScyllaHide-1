#include "ReadNtConfig.h"
#include <Scylla/OsInfo.h>
#include <Scylla/Util.h>
#include "..\HookLibrary\HookMain.h"
#include "Logger.h"

extern HOOK_DLL_EXCHANGE DllExchangeLoader;
extern WCHAR NtApiIniPath[MAX_PATH];

DWORD ReadApiFromIni(const WCHAR * name, const WCHAR * section) //rva
{
    WCHAR buf[100] = { 0 };
    if (GetPrivateProfileStringW(section, name, L"0", buf, _countof(buf), NtApiIniPath) > 0)
    {
        return wcstoul(buf, 0, 16);
    }

    return 0;
}

WCHAR text[500];

void ReadNtApiInformation()
{
    WCHAR OsId[300] = { 0 };
    WCHAR temp[50] = { 0 };

    const auto osVerInfo = Scylla::GetVersionExW();
    const auto osSysInfo = Scylla::GetNativeSystemInfo();

#ifdef _WIN64
    const wchar_t wszArch[] = L"x64";
#else
    const wchar_t wszArch[] = L"x86";
#endif

    auto wstrOsId = Scylla::format_wstring(L"%02X%02X%02X%02X%02X%02X_%s",
        osVerInfo->dwMajorVersion, osVerInfo->dwMinorVersion,
        osVerInfo->wServicePackMajor, osVerInfo->wServicePackMinor,
        osVerInfo->wProductType, osSysInfo->wProcessorArchitecture, wszArch);

    HMODULE hUser = GetModuleHandleW(L"user32.dll");
    PIMAGE_DOS_HEADER pDosUser = (PIMAGE_DOS_HEADER)hUser;
    PIMAGE_NT_HEADERS pNtUser = (PIMAGE_NT_HEADERS)((DWORD_PTR)pDosUser + pDosUser->e_lfanew);

    if (pNtUser->Signature != IMAGE_NT_SIGNATURE)
    {
        MessageBoxA(0,"Wrong user32.dll IMAGE_NT_SIGNATURE", "ERROR", MB_ICONERROR);
        return;
    }
    wsprintfW(temp, L"%08X", pNtUser->OptionalHeader.AddressOfEntryPoint);
    wcscat(OsId, L"_");
    wcscat(OsId, temp);

	LogDebug("ReadNtApiInformation -> Requesting OS-ID %S", OsId);

    DllExchangeLoader.NtUserBuildHwndListRVA = ReadApiFromIni(L"NtUserBuildHwndList", OsId);
    DllExchangeLoader.NtUserFindWindowExRVA = ReadApiFromIni(L"NtUserFindWindowEx", OsId);
    DllExchangeLoader.NtUserQueryWindowRVA = ReadApiFromIni(L"NtUserQueryWindow", OsId);

    if (!DllExchangeLoader.NtUserBuildHwndListRVA || !DllExchangeLoader.NtUserFindWindowExRVA || !DllExchangeLoader.NtUserQueryWindowRVA)
    {
        wsprintfW(text, L"NtUser* API Addresses missing!\r\nSection: %s\r\nFile: %s\r\n\r\nPlease read the documentation to fix this problem! https://bitbucket.org/NtQuery/scyllahide/downloads/ScyllaHide.pdf", OsId, NtApiIniPath);
        MessageBoxW(0, text, L"ERROR", MB_ICONERROR);
    }
}
