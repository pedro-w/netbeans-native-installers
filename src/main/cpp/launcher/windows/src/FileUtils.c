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

#include "FileUtils.h"
#include "StringUtils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <userenv.h>
#include <wchar.h>
#include <windows.h>

HANDLE stdoutHandle = INVALID_HANDLE_VALUE;
HANDLE stderrHandle = INVALID_HANDLE_VALUE;

DWORD newLine = 1;
LPCTSTR FILE_SEP = TEXT("\\");

const long CRC32_TABLE[256] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
    0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
    0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
    0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
    0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
    0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
    0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
    0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
    0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
    0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
    0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
    0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
    0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d};

void writeTimeStamp(HANDLE hd, DWORD need) {
  DWORD written;
  if (need == 1) {
    SYSTEMTIME t;
    char *yearStr;
    char *monthStr;
    char *dayStr;
    char *hourStr;
    char *minuteStr;
    char *secondStr;
    char *msStr;
    char *result = NULL;
    GetLocalTime(&t);
    yearStr = word2charN(t.wYear, 2);
    monthStr = word2charN(t.wMonth, 2);
    dayStr = word2charN(t.wDay, 2);
    hourStr = word2charN(t.wHour, 2);
    minuteStr = word2charN(t.wMinute, 2);
    secondStr = word2charN(t.wSecond, 2);
    msStr = word2charN(t.wMilliseconds, 3);

    result = appendString(NULL, "[");
    result = appendString(result, yearStr);
    result = appendString(result, "-");
    result = appendString(result, monthStr);
    result = appendString(result, "-");
    result = appendString(result, dayStr);
    result = appendString(result, " ");
    result = appendString(result, hourStr);
    result = appendString(result, ":");
    result = appendString(result, minuteStr);
    result = appendString(result, ":");
    result = appendString(result, secondStr);
    result = appendString(result, ".");
    result = appendString(result, msStr);
    result = appendString(result, "]> ");

    WriteFile(hd, result, sizeof(char) * getLength(result), &written, NULL);
    FREE(result);
    FREE(yearStr);
    FREE(monthStr);
    FREE(dayStr);

    FREE(hourStr);
    FREE(minuteStr);
    FREE(secondStr);
    FREE(msStr);
  }
}

void writeMessage(LauncherProperties *props, DWORD level, DWORD isErr,
                  LPCTSTR message, DWORD needEndOfLine) {
  if (level >= props->outputLevel) {
    HANDLE hd = (isErr) ? props->stderrHandle : props->stdoutHandle;
    DWORD written;
    writeTimeStamp(hd, newLine);
    WriteFile(hd, message, sizeof(TCHAR) * getLength(message), &written, NULL);
    if (needEndOfLine > 0) {
      newLine = 0;
      while ((needEndOfLine--) > 0) {
        writeMessage(props, level, isErr, TEXT("\r\n"), 0);
        newLine = 1;
      }
      flushHandle(hd);
    } else {
      newLine = 0;
    }
  }
}

void writeDWORD(LauncherProperties *props, DWORD level, DWORD isErr,
                LPCTSTR message, DWORD value, DWORD needEndOfLine) {
  LPTSTR dwordStr = DWORDtoTCHAR(value);
  writeMessage(props, level, isErr, message, 0);
  writeMessage(props, level, isErr, dwordStr, needEndOfLine);
  FREE(dwordStr);
}

void writeint64t(LauncherProperties *props, DWORD level, DWORD isErr,
                 LPCTSTR message, int64t *value, DWORD needEndOfLine) {
  LPTSTR str = int64ttoTCHAR(value);
  writeMessage(props, level, isErr, message, 0);
  writeMessage(props, level, isErr, str, needEndOfLine);
  FREE(str);
}
void writeError(LauncherProperties *props, DWORD level, DWORD isErr,
                LPCTSTR message, LPCTSTR param, DWORD errorCode) {
  LPTSTR err = getErrorDescription(errorCode);
  writeMessage(props, level, isErr, message, 0);
  writeMessage(props, level, isErr, param, 1);
  writeMessage(props, level, isErr, err, 1);
  FREE(err);
}

void flushHandle(HANDLE hd) { FlushFileBuffers(hd); }

int64t *getFreeSpace(LPCTSTR path) {
  int64t bytes;
  int64t *result = newint64_t(0, 0);
  LPTSTR dst = appendString(NULL, path);

  while (!fileExists(dst)) {
    LPTSTR parent = getParentDirectory(dst);
    FREE(dst);
    dst = parent;
    if (dst == NULL)
      break;
  }
  if (dst == NULL)
    return result; // no parent ? strange
  if (GetDiskFreeSpaceEx(dst, (PULARGE_INTEGER)&bytes, NULL, NULL)) {
    result->High = bytes.High;
    result->Low = bytes.Low;
  }
  FREE(dst);
  return result;
}

void checkFreeSpace(LauncherProperties *props, LPCTSTR tmpDir, int64t *size) {
  if (props->checkForFreeSpace) {
    int64t *space = getFreeSpace(tmpDir);
    DWORD result = 0;
    result = ((space->High > size->High) ||
              (space->High == size->High && space->Low >= size->Low));

    if (!result) {
      LPTSTR available = NULL;
      LPTSTR str = NULL;
      LPTSTR required = NULL;
      available = int64ttoTCHAR(space);
      required = int64ttoTCHAR(size);
      str = appendString(str, TEXT("Not enough free space in "));
      str = appendString(str, tmpDir);
      str = appendString(str, TEXT(", available="));
      str = appendString(str, available);
      str = appendString(str, TEXT(", required="));
      str = appendString(str, required);

      writeMessage(props, OUTPUT_LEVEL_DEBUG, 1, str, 1);
      FREE(str);
      FREE(available);
      FREE(required);
      props->status = ERROR_FREESPACE;
    }
    FREE(space);
  } else {
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 0,
                 TEXT("... free space checking is disabled"), 1);
  }
}

DWORD fileExists(LPCTSTR path) {
  WIN32_FILE_ATTRIBUTE_DATA attrs;
  return GetFileAttributesEx(path, GetFileExInfoStandard, &attrs);
}

DWORD isDirectory(LPCTSTR path) {
  WIN32_FILE_ATTRIBUTE_DATA attrs;
  if (GetFileAttributesEx(path, GetFileExInfoStandard, &attrs)) {
    return (attrs.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
  } else {
    return 0;
  }
}
LPTSTR getParentDirectory(LPCTSTR dir) {
  LPTSTR ptr = (LPTSTR)dir; // cast away const
  LPTSTR res = NULL;
  while (1) {
    if (search(ptr, FILE_SEP) == NULL) {
      break;
    }
    ptr = search(ptr, FILE_SEP) + 1;
  }
  res = appendStringN(NULL, 0, dir, getLength(dir) - getLength(ptr) - 1);
  return res;
}
LPTSTR normalizePath(LPTSTR dir) {
  LPTSTR directory = NULL;
  LPTSTR ptr1, ptr2, ptr3;
  DWORD i = 0;
  DWORD len;
  ptr1 = search(dir, TEXT(":\\"));
  ptr2 = search(dir, TEXT(":/"));
  ptr3 = search(dir, TEXT("\\\\"));
  if (ptr1 == NULL && ptr2 == NULL && dir != ptr3) { // relative path
    directory = appendString(getCurrentDirectory(), FILE_SEP);
    directory = appendString(directory, dir);
  } else {
    directory = appendString(NULL, dir);
  }
  len = getLength(directory);

  for (i = 0; i < len; i++) {
    if (directory[i] == TEXT('/'))
      directory[i] = TEXT('\\');
  }
  return directory;
}

void createDirectory(LauncherProperties *props, LPCTSTR directory) {

  LPTSTR parent;
  // now directory is the absolute path with normalized slashes
  writeMessage(props, OUTPUT_LEVEL_DEBUG, 0,
               TEXT("Getting parent directory of "), 0);
  writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, directory, 1);

  parent = getParentDirectory(directory);

  if (parent != NULL) {
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("    parent = "), 0);
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, parent, 1);

    if (!fileExists(parent)) {
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 0,
                   TEXT("... doesn`t exist. Create it..."), 1);
      createDirectory(props, parent);
    } else {
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("    ... exist. "), 1);
    }
    if (isOK(props)) {
      DWORD parentAttrs = GetFileAttributes(parent);
      if (parentAttrs == INVALID_FILE_ATTRIBUTES) {
        props->status = ERROR_INPUTOUPUT;
      } else {
        SECURITY_ATTRIBUTES secattr = {0};
        int64t *minSize = newint64_t(0, 0);

        secattr.nLength = sizeof(SECURITY_ATTRIBUTES);
        secattr.lpSecurityDescriptor = 0;
        secattr.bInheritHandle = TRUE;
        writeMessage(props, OUTPUT_LEVEL_DEBUG, 1,
                     TEXT("... creating directory itself... "), 0);
        writeMessage(props, OUTPUT_LEVEL_DEBUG, 1, directory, 1);

        checkFreeSpace(props, parent, minSize);
        FREE(minSize);

        if (isOK(props)) {
          props->status = (CreateDirectoryEx(parent, directory, &secattr))
                              ? ERROR_OK
                              : ERROR_INPUTOUPUT;
          if (!isOK(props)) {
            props->status = (CreateDirectory(directory, &secattr))
                                ? ERROR_OK
                                : ERROR_INPUTOUPUT;
          }
          props->status = (fileExists(directory)) ? ERROR_OK : ERROR_INPUTOUPUT;

          if (isOK(props)) {
            SetFileAttributes(directory, parentAttrs);
          } else {
            writeError(props, OUTPUT_LEVEL_DEBUG, 1,
                       TEXT("Error! Can`t create directory : "), directory,
                       GetLastError());
          }
        }
      }
    }
    FREE(parent);
  } else {
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("    parent is NULL "), 1);
    props->status = ERROR_INPUTOUPUT;
  }
}

#define a 16807       /* multiplier */
#define m 2147483647L /* 2**31 - 1 */
#define q 127773L     /* m div a */
#define r 2836        /* m mod a */

long nextRandDigit(long seed) {
  unsigned long lo, hi;

  lo = a * (long)(seed & 0xFFFF);
  hi = a * (long)((unsigned long)seed >> 16);
  lo += (hi & 0x7FFF) << 16;
  if (lo > m) {
    lo &= m;
    ++lo;
  }
  lo += hi >> 15;
  if (lo > m) {
    lo &= m;
    ++lo;
  }
  return (long)lo;
}

void createTempDirectory(LauncherProperties *props, LPCTSTR argTempDir,
                         DWORD createRndSubDir) {
  LPTSTR t = (argTempDir != NULL) ? appendString(NULL, argTempDir)
                                  : getSystemTemporaryDirectory();

  LPTSTR nbiTmp = normalizePath(t);
  FREE(t);
  if (createRndSubDir) {
    LPTSTR randString = newpTCHAR(6);
    DWORD i = 0;
    DWORD initValue = GetTickCount() & 0x7FFFFFFF;
    long value = (long)initValue;
    nbiTmp = appendString(nbiTmp, TEXT("\\NBI"));

    for (i = 0; i < 5; i++) {
      value = nextRandDigit(value);
      randString[i] = TEXT('0') + (TCHAR)(value % 10);
    }
    nbiTmp = appendString(appendString(nbiTmp, randString), TEXT(".tmp"));
    FREE(randString);
  }

  writeMessage(props, OUTPUT_LEVEL_NORMAL, 0,
               TEXT("Using temp directory for extracting data : "), 0);
  writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, nbiTmp, 1);

  if (fileExists(nbiTmp)) {
    if (!isDirectory(nbiTmp)) {
      props->status = ERROR_INPUTOUPUT;
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 0,
                   TEXT(".. exists but not a directory"), 1);
    } else {
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 0,
                   TEXT("Output directory already exist so don`t create it"),
                   1);
    }
  } else {
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("creating directory..."),
                 1);
    createDirectory(props, nbiTmp);
    if (isOK(props)) {
      props->tmpDirCreated = 1;
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("Directory created : "),
                   0);
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, nbiTmp, 1);
      // set hidden attribute
      if (createRndSubDir) {
        writeMessage(props, OUTPUT_LEVEL_DEBUG, 1,
                     TEXT("Setting hidden attributes to "), 0);
        writeMessage(props, OUTPUT_LEVEL_DEBUG, 1, nbiTmp, 1);

        SetFileAttributes(nbiTmp,
                          GetFileAttributes(nbiTmp) | FILE_ATTRIBUTE_HIDDEN);
      }
    } else {
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 1,
                   TEXT("Error! Can`t create nbi temp directory : "), 0);
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 1, nbiTmp, 1);
    }
  }
  props->tmpDir = nbiTmp;
  return;
}

void deleteDirectory(LauncherProperties *props, LPCTSTR dir) {
  DWORD attrs = GetFileAttributes(dir);
  DWORD dwError;
  DWORD count = 0;
  if (attrs == INVALID_FILE_ATTRIBUTES) {
    writeError(props, OUTPUT_LEVEL_DEBUG, 1,
               TEXT("Error! Can`t get attributes of the dir : "), dir,
               GetLastError());
    return;
  }
  if (!SetFileAttributes(dir, attrs & FILE_ATTRIBUTE_NORMAL)) {
    writeError(props, OUTPUT_LEVEL_DEBUG, 1,
               TEXT("Error! Can`t set attributes of the dir : "), dir,
               GetLastError());
  }

  if (attrs & FILE_ATTRIBUTE_DIRECTORY) {
    WIN32_FIND_DATA FindFileData;
    HANDLE hFind = INVALID_HANDLE_VALUE;

    LPTSTR DirSpec = appendString(appendString(NULL, dir), TEXT("\\*"));

    // Find the first file in the directory.
    hFind = FindFirstFile(DirSpec, &FindFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
      writeError(props, OUTPUT_LEVEL_DEBUG, 1,
                 TEXT("Error! Can`t file with pattern "), DirSpec,
                 GetLastError());
    } else {
      // List all the other files in the directory.
      while (FindNextFile(hFind, &FindFileData) != 0) {
        if (lstrcmp(FindFileData.cFileName, TEXT(".")) != 0 &&
            lstrcmp(FindFileData.cFileName, TEXT("..")) != 0) {
          LPTSTR child =
              appendString(appendString(appendString(NULL, dir), FILE_SEP),
                           FindFileData.cFileName);
          deleteDirectory(props, child);
          FREE(child);
        }
      }

      dwError = GetLastError();
      FindClose(hFind);
      if (dwError != ERROR_NO_MORE_FILES) {
        writeError(props, OUTPUT_LEVEL_DEBUG, 1,
                   TEXT("Error! Can`t find file with pattern : "), DirSpec,
                   dwError);
      }
    }

    // 20 tries in 2 seconds to delete the directory
    while (!RemoveDirectory(dir) && count++ < 20)
      Sleep(100);
    FREE(DirSpec);
  } else {
    // 20 tries in 2 seconds to delete the file
    while (!DeleteFile(dir) && count++ < 20)
      Sleep(100);
  }
}

LPTSTR getSystemTemporaryDirectory() {
  LPTSTR expanded = newpTCHAR(MAX_PATH);
  DWORD result;
  if (GetTempPath(MAX_PATH, expanded) != 0) {
    return expanded;
  }

  result = GetEnvironmentVariable(TEXT("TEMP"), expanded, MAX_PATH);
  if (result <= 0 || result > MAX_PATH) {
    // writeOutputLn("Can`t find variable TEMP");
    result = GetEnvironmentVariable(TEXT("TMP"), expanded, MAX_PATH);
    if (result <= 0 || result > MAX_PATH) {
      // writeOutputLn("Can`t find variable TMP");
      result = GetEnvironmentVariable(TEXT("USERPROFILE"), expanded, MAX_PATH);
      if (result > 0 && result <= MAX_PATH) {
        expanded = appendString(expanded, TEXT("\\Local Settings\\Temp"));
      } else {
        LPTSTR curdir = getCurrentDirectory();
        ZERO(expanded, sizeof(TCHAR) * MAX_PATH);
        lstrcpyn(expanded, curdir, MAX_PATH);
        FREE(curdir);
      }
    }
  }
  return expanded;
}

LPTSTR getExePath() {
  LPTSTR szPath = newpTCHAR(MAX_PATH);

  if (!GetModuleFileName(NULL, szPath, MAX_PATH)) {
    FREE(szPath);
    return NULL;
  } else {
    return szPath;
  }
}

LPTSTR getExeName() {
  LPTSTR szPath = newpTCHAR(MAX_PATH);
  LPTSTR result = NULL;
  if (GetModuleFileName(NULL, szPath, MAX_PATH)) {
    LPTSTR lastSlash = szPath;
    while (search(lastSlash, FILE_SEP) != NULL) {
      lastSlash = search(lastSlash, FILE_SEP) + 1;
    }
    result = appendString(NULL, lastSlash);
  }
  FREE(szPath);
  return result;
}

LPTSTR getExeDirectory() {
  LPTSTR szPath = newpTCHAR(MAX_PATH);
  LPTSTR result = NULL;
  if (GetModuleFileName(NULL, szPath, MAX_PATH)) {
    LPTSTR lastSlash = szPath;
    while (search(lastSlash, FILE_SEP) != NULL) {
      lastSlash = search(lastSlash, FILE_SEP) + 1;
    }
    result = appendStringN(NULL, 0, szPath,
                           getLength(szPath) - getLength(lastSlash) - 1);
  }
  FREE(szPath);
  return result;
}

LPTSTR getCurrentDirectory() {
  DWORD size = GetCurrentDirectory(0, NULL);
  LPTSTR buf = newpTCHAR(size);

  if (GetCurrentDirectory(size, buf) == size) {
    return buf;
  } else {
    FREE(buf);
    return NULL;
  }
}
LPTSTR getCurrentUserHome() {
  HANDLE hUser;
  LPTSTR buf = NULL;
  if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hUser)) {
    DWORD size = 0;
    GetUserProfileDirectory(hUser, NULL, &size);
    buf = newpTCHAR(size);
    if (!GetUserProfileDirectory(hUser, buf, &size)) {
      FREE(buf);
    }
    CloseHandle(hUser);
  }
  if (buf == NULL) {
    DWORD size;
    size = GetEnvironmentVariable(TEXT("USERPROFILE"), NULL, 0);
    if (size > 0) {
      buf = newpTCHAR(size);
      GetEnvironmentVariable(TEXT("USERPROFILE"), buf, size);
    }
  }
  return buf;
}
int64t *getFileSize(LPCTSTR path) {
  WIN32_FILE_ATTRIBUTE_DATA wfad;
  int64t *res = newint64_t(0, 0);
  if (GetFileAttributesEx(path, GetFileExInfoStandard, &wfad)) {
    res->Low = wfad.nFileSizeLow;
    res->High = wfad.nFileSizeHigh;
  }
  return res;
}

void update_crc32(DWORD *crc, char *ptr, DWORD size) {
  while (size--)
    *crc = CRC32_TABLE[(unsigned char)(*crc ^ *ptr++)] ^ (*crc >> 8);
}
