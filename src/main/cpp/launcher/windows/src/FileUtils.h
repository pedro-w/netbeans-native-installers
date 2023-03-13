/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef _FileUtils_H
#define _FileUtils_H

#include "Errors.h"
#include "Types.h"
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OUTPUT_LEVEL_DEBUG 0
#define OUTPUT_LEVEL_NORMAL 1

extern LPCTSTR FILE_SEP;
extern const long CRC32_TABLE[256];
void update_crc32(DWORD *crc32, BYTE *buf, DWORD size);
int64t *getFreeSpace(LPCTSTR path);
int64t *getFileSize(LPCTSTR path);
void checkFreeSpace(LauncherProperties *props, LPCTSTR tmpDir, int64t *size);
LPTSTR getParentDirectory(LPCTSTR dir);
void createDirectory(LauncherProperties *props, LPCTSTR directory);
void createTempDirectory(LauncherProperties *props, LPCTSTR argTempDir,
                         DWORD createRndSubDir);
void deleteDirectory(LauncherProperties *props, LPCTSTR dir);
LPTSTR getExePath();
LPTSTR getExeName();
LPTSTR getExeDirectory();

LPTSTR getSystemTemporaryDirectory();
DWORD isDirectory(LPCTSTR path);
LPTSTR getCurrentDirectory();
LPTSTR getCurrentUserHome();

void writeMessage(LauncherProperties *props, DWORD level, DWORD isErr,
                  LPCTSTR message, DWORD needEndOfLine);
void writeError(LauncherProperties *props, DWORD level, DWORD isErr,
                LPCTSTR message, LPCTSTR param, DWORD errorCode);
void writeDWORD(LauncherProperties *props, DWORD level, DWORD isErr,
                const char *message, DWORD value, DWORD needEndOfLine);
void writeint64t(LauncherProperties *props, DWORD level, DWORD isErr,
                 const char *message, int64t *value, DWORD needEndOfLine);

void flushHandle(HANDLE hd);
DWORD fileExists(LPCTSTR path);

#ifdef __cplusplus
}
#endif

#endif /* _FileUtils_H */
