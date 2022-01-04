
#include <vector>
#include <windows.h>

class WindowHelper
{
public:
    static std::vector<HWND> getChildren(HWND parent)
    {
        std::vector<HWND> result;
        ::EnumChildWindows(
            parent,
            [](HWND hWnd, LPARAM lParam) -> BOOL {
                auto result = reinterpret_cast<std::vector<HWND>*>(lParam);
                result->push_back(hWnd);
                return true;
            },
            (LONG_PTR)(&result)
        );
        return result;
    }
};