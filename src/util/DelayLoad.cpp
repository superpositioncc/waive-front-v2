/*
WAIVE-FRONT
Copyright (C) 2024  Bram Bogaerts, Superposition

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/**
 * @file DelayLoad.cpp
 * @brief Loads the FFmpeg DLLs from the folder of the plugin itself (Windows, shared FFmpeg only)
 *
 * A DAW loads a plugin by its full path, but Windows looks for the DLLs that plugin needs next to
 * the DAW's exe, not next to the plugin. The FFmpeg DLLs are delay-loaded (see CMakeLists.txt), and
 * this hook loads them from the plugin's own folder instead.
 */

#ifdef _WIN32

#include <windows.h>
#include <delayimp.h>
#include <string>

static FARPROC WINAPI loadFromPluginFolder(unsigned notification, PDelayLoadInfo info)
{
	if (notification != dliNotePreLoadLibrary)
		return nullptr;

	HMODULE self = nullptr;
	if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
							reinterpret_cast<LPCWSTR>(&loadFromPluginFolder), &self))
		return nullptr;

	wchar_t buffer[MAX_PATH];
	DWORD length = GetModuleFileNameW(self, buffer, MAX_PATH);
	if (length == 0 || length == MAX_PATH)
		return nullptr;

	std::wstring path(buffer, length);
	path = path.substr(0, path.find_last_of(L"\\/") + 1);

	for (const char *c = info->szDll; *c; c++)
		path += static_cast<wchar_t>(*c);

	// The altered search path lets avformat find avcodec and friends in the same folder.
	return reinterpret_cast<FARPROC>(LoadLibraryExW(path.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH));
}

extern "C" const PfnDliHook __pfnDliNotifyHook2 = loadFromPluginFolder;

#endif
