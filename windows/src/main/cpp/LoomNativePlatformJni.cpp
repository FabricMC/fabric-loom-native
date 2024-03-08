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

#include "net_fabricmc_loom_nativeplatform_LoomNativePlatformImpl.h"

#include "LoomNativePlatform.h"

#include <string>

namespace {

// Convert a Java string to a C++ std::wstring
std::wstring asWstring(JNIEnv *env, jstring string) {
  const jchar *raw = env->GetStringChars(string, 0);
  jsize len = env->GetStringLength(string);

  std::wstring_view view(reinterpret_cast<const wchar_t *>(raw), len);
  std::wstring value{view.begin(), view.end()};

  env->ReleaseStringChars(string, raw);

  return value;
}

jlongArray asJlongArray(JNIEnv *env, const std::vector<std::uint64_t> &vec) {
  static_assert(sizeof(std::int64_t) == sizeof(jlong));
  const auto length = static_cast<jsize>(vec.size());
  jlongArray array = env->NewLongArray(length);
  if (array != nullptr && length != 0) {
    env->SetLongArrayRegion(array, 0, length,
                            reinterpret_cast<const jlong *>(vec.data()));
  }
  return array;
}

jobjectArray asJStringArray(JNIEnv *env, const std::vector<std::wstring> &vec) {
  jclass stringClass = env->FindClass("java/lang/String");
  const auto length = static_cast<jsize>(vec.size());
  jobjectArray array = env->NewObjectArray(length, stringClass, nullptr);
  if (array != nullptr) {
    for (jsize i = 0; i < length; i++) {
      auto str = vec[i];
      auto strLen = static_cast<jsize>(str.size());
      jstring string =
          env->NewString(reinterpret_cast<const jchar *>(str.c_str()), strLen);
      env->SetObjectArrayElement(array, i, string);
    }
  }
  return array;
}

jint throwRuntimeException(JNIEnv *env, const std::string &message) {
  jclass exceptionClass = env->FindClass("java/lang/RuntimeException");
  return env->ThrowNew(exceptionClass, message.c_str());
}
} // namespace

JNIEXPORT jlongArray JNICALL
Java_net_fabricmc_loom_nativeplatform_LoomNativePlatformImpl_getPidsHoldingFileHandles(
    JNIEnv *env, jclass, jstring path) {
  try {
    const auto wpath = asWstring(env, path);
    const auto pids = Loom::getPidHoldingFileLock(wpath);
    return asJlongArray(env, pids);
  } catch (const std::exception &e) {
    throwRuntimeException(env, e.what());
  }

  return nullptr;
}

JNIEXPORT jobjectArray JNICALL
Java_net_fabricmc_loom_nativeplatform_LoomNativePlatformImpl_getWindowTitlesForPid(
    JNIEnv *env, jclass, jlong pid) {
  try {
    const auto titles = Loom::getProcessWindowTitles(pid);
    return asJStringArray(env, titles);
  } catch (const std::exception &e) {
    throwRuntimeException(env, e.what());
  }

  return nullptr;
}