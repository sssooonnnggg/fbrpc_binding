#pragma once

#include <functional>
#include <optional>

#include <windows.h>

class WindowHook
{
public:
    static void hook(HWND target);

    using OptionalResult = std::optional<HRESULT>;
    using HookedMessageHandler = std::function<OptionalResult(HWND, UINT, WPARAM, LPARAM, HRESULT)>;

    enum class MessagehandlerType
    {
        kPre,
        kPost
    };
    static void registerParentMessageHandler(HookedMessageHandler handler, MessagehandlerType type = MessagehandlerType::kPre);
    static void registerChildMessageHandler(HookedMessageHandler handler, MessagehandlerType type = MessagehandlerType::kPre);

private:
    static LRESULT CALLBACK hookedParentWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK hookedChildWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    static void hookParentWindow(HWND parent);
    static void hookChildWindowWhenCreated(WPARAM wParam, LPARAM lParam);
    static void hookChildWindow(HWND child);
    static void hookWindow(HWND target, WNDPROC proc);
};