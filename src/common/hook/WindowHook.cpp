#include <map>
#include <vector>

#include "WindowHook.h"

#define GWL_WNDPROC -4

namespace
{
    std::map<HWND, WNDPROC>  OriginProcs;
    std::vector<WindowHook::HookedMessageHandler> PreParentMessageHandlers;
    std::vector<WindowHook::HookedMessageHandler> PostParentMessageHandlers;
    std::vector<WindowHook::HookedMessageHandler> PreChildMessageHandlers;
    std::vector<WindowHook::HookedMessageHandler> PostChildMessageHandlers;
}

void WindowHook::hook(HWND target)
{
    if (OriginProcs.find(target) == OriginProcs.end())
        hookParentWindow(target); 
}

void WindowHook::registerParentMessageHandler(HookedMessageHandler handler, MessagehandlerType type)
{
    if (type == MessagehandlerType::kPre)
        PreParentMessageHandlers.push_back(handler);
    else
        PostParentMessageHandlers.push_back(handler);
}

void WindowHook::registerChildMessageHandler(HookedMessageHandler handler, MessagehandlerType type)
{
    if (type == MessagehandlerType::kPre)
        PreChildMessageHandlers.push_back(handler);
    else
        PostChildMessageHandlers.push_back(handler);
}

LRESULT WindowHook::hookedParentWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // printf("Parent Window Message : %x\n", message);

    if (message == WM_PARENTNOTIFY)
        hookChildWindowWhenCreated(wParam, lParam);

    for (auto handler : PreParentMessageHandlers)
    {
        auto result = handler(hWnd, message, wParam, lParam, 0);
        if (result.has_value())
            return result.value();
    }
        

    auto originProc = OriginProcs[hWnd];
    auto originResult = originProc(hWnd, message, wParam, lParam);

    for (auto handler : PostParentMessageHandlers)
    {
        auto result = handler(hWnd, message, wParam, lParam, originResult);
        if (result.has_value())
            return result.value();
    }

    return originResult;
}

LRESULT WindowHook::hookedChildWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // printf("Child Window Message : %x\n", message);

    for (auto handler : PreChildMessageHandlers)
    {
        auto result = handler(hWnd, message, wParam, lParam, 0);
        if (result.has_value())
            return result.value();
    }

    auto originProc = OriginProcs[hWnd];
    auto originResult = originProc(hWnd, message, wParam, lParam);

    for (auto handler : PostChildMessageHandlers)
    {
        auto result = handler(hWnd, message, wParam, lParam, originResult);
        if (result.has_value())
            return result.value();
    }

    return originResult;
}

void WindowHook::hookChildWindowWhenCreated(WPARAM wParam, LPARAM lParam)
{
    // when child window is created, hook child window
    auto childMessage = LOWORD(wParam);
    if (childMessage == WM_CREATE)
    {
        auto childHwnd = (HWND)(lParam);
        hookChildWindow(childHwnd);
    }
}

void WindowHook::hookParentWindow(HWND parent)
{
    printf("hook parent window : %p\n", parent);
    hookWindow(parent, hookedParentWndProc);
}

void WindowHook::hookChildWindow(HWND child)
{
    printf("hook child window : %p\n", child);
    hookWindow(child, hookedChildWndProc);
}

void WindowHook::hookWindow(HWND target, WNDPROC proc)
{
    if (OriginProcs.find(target) == OriginProcs.end())
    {
        auto originProc = (WNDPROC)(::GetWindowLongPtrW(target, GWL_WNDPROC));
        printf("origin proc : %p\n", originProc);
        ::SetWindowLongPtr(target, GWL_WNDPROC, (LONG_PTR)proc);
        OriginProcs[target] = originProc;
    }
}