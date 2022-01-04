#pragma once

#include <cassert>
#include <stdio.h>
#include <windows.h>

template<class T>
class GlobalHook
{
public:
    static void begin(int hookType = WH_MOUSE_LL)
    {
        if (m_hookHandle == NULL)
        {
            m_hookHandle = ::SetWindowsHookEx(hookType, globalHookProc, ::GetModuleHandle(NULL), NULL);
            if (m_hookHandle == NULL)
                printf("set global hook failed, error id : %d", ::GetLastError());
        }
    }
    static void end()
    {
        if (m_hookHandle != NULL)
        {
            ::UnhookWindowsHookEx(m_hookHandle);
            m_hookHandle = NULL;
        }
    }

    static LRESULT CALLBACK globalHookProc(int code, WPARAM wParam, LPARAM lParam)
    {
        return T::customGlobalHookProc(code, wParam, lParam);
    }

private:
    inline static HHOOK m_hookHandle = NULL;
};