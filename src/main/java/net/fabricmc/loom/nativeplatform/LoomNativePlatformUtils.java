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
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

package net.fabricmc.loom.nativeplatform;

import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.util.Locale;
import java.util.Map;
import java.util.Objects;

// Loads the native dlls if they are supported by the current platform
class LoomNativePlatformUtils {
	private static final String OS_ID = OperatingSystem.CURRENT.name().toLowerCase(Locale.ROOT) + "-" + System.getProperty("os.arch").toLowerCase(Locale.ROOT);
	private static final Map<String, String> NATIVE_LIBS = Map.of(
			"windows-aarch64", "fabric-loom-native/aarch64/fabric-loom-native.dll",
			"windows-win32", "fabric-loom-native/x86/fabric-loom-native.dll",
			"windows-amd64", "fabric-loom-native/x86-64/fabric-loom-native.dll"
	);

	private static boolean initialized = false;
	private static boolean isReady = false;

	static synchronized boolean isReady() {
		if (initialized) {
			return isReady;
		}

		initialized = true;

		if (!NATIVE_LIBS.containsKey(OS_ID)) {
			return false;
		}

		try {
			loadLibrary();
		} catch (Exception e) {
			e.printStackTrace();
		}

		return isReady;
	}

	private static void loadLibrary() throws IOException {
		String nativeName = NATIVE_LIBS.get(OS_ID);
		Path nativePath = Files.createTempFile("fabric-loom-native", ".dll");

		try (InputStream is = LoomNativePlatformUtils.class.getClassLoader().getResourceAsStream(nativeName)) {
			Objects.requireNonNull(is, "Could not load: " + nativeName);
			Files.copy(is, nativePath, StandardCopyOption.REPLACE_EXISTING);
		}

		System.load(nativePath.toString());

		isReady = true;
	}

}
