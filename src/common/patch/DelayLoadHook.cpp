// Taken from https://github.com/nodejs/node-gyp/pull/653 to work with Electron
// "Official" node-gyp version at https://github.com/nodejs/node-gyp/blob/master/src/win_delay_load_hook.cc

#ifdef _MSC_VER

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Shlwapi.h>
#include <delayimp.h>
#include <string.h>

#pragma comment(lib, "Shlwapi.lib")

static FARPROC WINAPI load_exe_hook(unsigned int event, DelayLoadInfo* info) {

	if (event != dliNotePreLoadLibrary)
		return NULL;

	if (_stricmp(info->szDll, "iojs.exe") != 0 &&
		_stricmp(info->szDll, "node.exe") != 0 &&
		_stricmp(info->szDll, "node.dll") != 0 &&
		_stricmp(info->szDll, "electron.exe") != 0)
		return NULL;

	// Get a handle to the current process executable.
	HMODULE processModule = GetModuleHandle(NULL);

	// Get the path to the executable.
	char processPath[_MAX_PATH];
	GetModuleFileNameA(processModule, processPath, _MAX_PATH);

	// Get the name of the current executable.
	LPSTR processName = PathFindFileNameA(processPath);

	// If the current process is node or iojs, then just return the proccess module.
	if (_stricmp(processName, "node.exe") == 0 ||
		_stricmp(processName, "iojs.exe") == 0 ||
		_stricmp(processName, "electron.exe") == 0) {
		return (FARPROC)processModule;
	}

	// If it is another process, attempt to load 'node.dll' from the same directory.
	PathRemoveFileSpecA(processPath);
	PathAppendA(processPath, "node.dll");

	HMODULE nodeDllModule = GetModuleHandleA(processPath);
	if (nodeDllModule != NULL) {
		// This application has a node.dll in the same directory as the executable, use that.
		return (FARPROC)nodeDllModule;
	}

	// Fallback to the current executable, which must statically link to node.lib.
	return (FARPROC)processModule;
}

// See https://docs.microsoft.com/en-us/cpp/build/reference/notification-hooks
decltype(__pfnDliNotifyHook2) __pfnDliNotifyHook2 = load_exe_hook;

#endif