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

#include "MemoryLeakDetector.h"

#define WIN32_LEAN_AND_MEAN
#include <crtdbg.h>
#include <windows.h>

namespace Loom {

namespace {
// Redirect CRT report hook to cerr
static int __cdecl crtReportHook(int, char *szMsg, int *) {
  std::cerr << szMsg << std::flush;
  return 1;
}
} // namespace

MemoryLeakDetector::MemoryLeakDetector() {
  _CrtSetReportHook2(_CRT_RPTHOOK_INSTALL, &crtReportHook);
  _CrtMemCheckpoint(&_memState);
}

inline void fail() { FAIL() << "Leaked memory detected"; }

MemoryLeakDetector::~MemoryLeakDetector() {
  _CrtMemState stateNow = {0};
  _CrtMemCheckpoint(&stateNow);
  _CrtMemState memDiff = {0};
  if (_CrtMemDifference(&memDiff, &_memState, &stateNow) &&
      !::testing::Test::HasFailure()) {
    _CrtMemDumpAllObjectsSince(&_memState);
    _CrtMemDumpStatistics(&memDiff);
    fail();
  }

  _CrtSetReportHook2(_CRT_RPTHOOK_REMOVE, &crtReportHook);
}

} // namespace Loom
