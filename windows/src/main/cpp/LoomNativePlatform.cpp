/*
 * This file is part of fabric-loom, licensed under the MIT License (MIT).
 *
 * Copyright (c) 2024 FabricMC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "LoomNativePlatform.h"
#include "Raii.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <restartmanager.h>

namespace Loom {
namespace {
struct RmSessionRaiiTraits {
  using type = DWORD;
  static constexpr auto invalidValue = 0;
  static void close(type t) noexcept { ::RmEndSession(t); }
};
using RmSession = RaiiWithInvalidValue<RmSessionRaiiTraits>;

struct ProcessRaiiTraits {
  using type = HANDLE;
  static constexpr auto invalidValue = nullptr;
  static void close(type t) noexcept { ::CloseHandle(t); }
};
using Process = RaiiWithInvalidValue<ProcessRaiiTraits>;

[[noreturn]] inline void throwLastError(const std::string &message) {
  auto lastError = GetLastError();
  throw std::system_error(lastError, std::system_category(), message);
}

RmSession createRmSession() {
  DWORD dwSession;
  WCHAR szSessionKey[CCH_RM_SESSION_KEY + 1] = {0};

  if (::RmStartSession(&dwSession, 0, szSessionKey) != ERROR_SUCCESS) {
    throwLastError("RmStartSession failed");
  }

  return RmSession{dwSession};
}

struct EnumWindowsProcData {
  std::uint64_t pid;
  std::vector<std::wstring> &titles;
};

static bool isWindowOfPid(HWND hwnd, std::uint64_t pid) {
  DWORD windowPid;
  ::GetWindowThreadProcessId(hwnd, &windowPid);
  return windowPid == pid;
}

BOOL isMainWindow(HWND handle) {
  return ::GetWindow(handle, GW_OWNER) == (HWND)0 && ::IsWindowVisible(handle);
}

static std::optional<std::wstring> getWindowTitle(HWND hwnd) {
  std::wstring title;
  auto length = ::GetWindowTextLengthW(hwnd);

  if (length == 0) {
    return std::nullopt;
  }

  title.resize(length + 1);
  ::GetWindowTextW(hwnd, title.data(), static_cast<int>(title.size()));

  // Remove the null terminator
  title.pop_back();

  return title;
}

static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
  EnumWindowsProcData *data = reinterpret_cast<EnumWindowsProcData *>(lParam);

  if (!isWindowOfPid(hwnd, data->pid)) {
    return TRUE;
  }

  if (!isMainWindow(hwnd)) {
    return TRUE;
  }

  auto title = getWindowTitle(hwnd);

  if (title) {
    data->titles.push_back(title.value());
  }

  return TRUE;
}
} // namespace

// https://devblogs.microsoft.com/oldnewthing/20120217-00/?p=8283
// TODO maybe look into using:
// https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/nf-shobjidl_core-ifileisinuse-closefile
std::vector<std::uint64_t>
getPidHoldingFileLock(const std::filesystem::path &file) {
  std::vector<std::uint64_t> pids;
  RmSession session = createRmSession();

  PCWSTR path_ptr = file.c_str();
  if (::RmRegisterResources(session.get(), 1, &path_ptr, 0, NULL, 0, NULL) !=
      ERROR_SUCCESS) {
    throwLastError("RmRegisterResources failed");
  }

  DWORD dwError;
  DWORD dwReason;
  UINT nProcInfoNeeded = 64, nProcInfo;
  std::vector<RM_PROCESS_INFO> rgpi;

  do {
    nProcInfo = 2 * nProcInfoNeeded;
    nProcInfoNeeded = 0;
    rgpi.resize(nProcInfo);
    dwError = RmGetList(session.get(), &nProcInfoNeeded, &nProcInfo,
                        rgpi.data(), &dwReason);
  } while (dwError == ERROR_MORE_DATA);

  if (dwError != ERROR_SUCCESS) {
    throwLastError("RmGetList failed");
  }

  for (const auto &info : rgpi) {
    Process process(::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE,
                                  info.Process.dwProcessId));
    if (process) {
      // Ensure that the process start time matches the one from the
      // RM_PROCESS_INFO
      FILETIME ftCreate, ftExit, ftKernel, ftUser;
      if (GetProcessTimes(process.get(), &ftCreate, &ftExit, &ftKernel,
                          &ftUser) &&
          CompareFileTime(&info.Process.ProcessStartTime, &ftCreate) == 0) {
        pids.push_back(info.Process.dwProcessId);
      }
    }
  }

  return pids;
}

std::vector<std::wstring> getProcessWindowTitles(std::uint64_t pid) {
  std::vector<std::wstring> titles;
  EnumWindowsProcData data{pid, titles};

  if (!::EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&data))) {
    throwLastError("EnumWindows failed");
  }

  return titles;
}
} // namespace Loom