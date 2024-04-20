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

import java.nio.file.Path;
import java.util.*;
import java.util.stream.Collectors;

public final class LoomNativePlatform {
	private LoomNativePlatform() {
	}

	/**
	 * Gets a list of processes that have a lock on the file, will return empty on unsupported platforms
	 * @param path the path to the file
	 * @return a list of processes that have a lock on the file
	 */
	public static List<ProcessHandle> getProcessesWithLockOn(Path path) throws LoomNativePlatformException {
		if (LoomNativePlatformUtils.isReady()) {
			final long[] pids = LoomNativePlatformImpl.getPidsHoldingFileHandles(path.toString());

			return Arrays.stream(pids)
					.mapToObj(ProcessHandle::of)
					.filter(Optional::isPresent)
					.map(Optional::get)
					.collect(Collectors.toList());
		}

		return Collections.emptyList();
	}

	/**
	 * Get a list of window titles for the given process ID.
	 * @param pid The process ID
	 * @return An array of window titles, may be empty if the process has no windows
	 */
	public static List<String> getWindowTitlesForPid(long pid) throws LoomNativePlatformException {
		if (LoomNativePlatformUtils.isReady()) {
			return Arrays.asList(LoomNativePlatformImpl.getWindowTitlesForPid(pid));
		}

		return Collections.emptyList();
	}

	/**
	 * @return true if the current platform is supported
	 */
	public static boolean isSupported() {
		return LoomNativePlatformUtils.isReady();
	}
}
