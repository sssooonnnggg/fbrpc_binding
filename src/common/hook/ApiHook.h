#pragma once

#include <windows.h>
#include <detours.h>

template<class T, auto Api>
struct ApiHook
{
    static decltype(Api) oldApi;
    static bool hook()
    {
        DetourRestoreAfterWith();
        DetourTransactionBegin();
        DetourUpdateThread(::GetCurrentThread());
        auto result = DetourAttach(&(PVOID&)oldApi, &T::hookedApi);
        if (result != NO_ERROR)
        {
            printf("Hook SetCursor failed, error code : %d\n", result);
            return false;
        }
        DetourTransactionCommit();
        return true;
    }
};

template<class T, auto Api>
decltype(Api) ApiHook<T, Api>::oldApi = Api;