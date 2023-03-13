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

#ifndef _Types_H
#define _Types_H

#include <tchar.h>
#include <windows.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct int64s {
  DWORD Low;
  DWORD High;
} int64t;

typedef struct {
  long major;
  long minor;
  long micro;
  long update;
  char build[128];
} JavaVersion;

typedef struct _javaProps {
  TCHAR *javaHome;
  TCHAR *javaExe;
  TCHAR *vendor;
  TCHAR *osName;
  TCHAR *osArch;
  JavaVersion *version;
} JavaProperties;

typedef struct _javaCompatible {
  JavaVersion *minVersion;
  JavaVersion *maxVersion;
  TCHAR *vendor;
  TCHAR *osName;
  TCHAR *osArch;
} JavaCompatible;

typedef struct _launcherResource {
  TCHAR *path;
  TCHAR *resolved;
  DWORD type;
} LauncherResource;

typedef struct _launcherResourceList {
  LauncherResource **items;
  DWORD size;
} LauncherResourceList;

typedef struct _WCHARList {
  WCHAR **items;
  DWORD size;
} WCHARList;
typedef struct _charList {
  char **items;
  DWORD size;
} charList;
#ifdef UNICODE
typedef struct _WCHARList TCHARList;
#else
typedef struct _charList TCHARList;
#endif
typedef struct _string {
  TCHAR *bytes;
  DWORD length;
} SizedString;

typedef struct _i18nstrings {
  TCHAR **properties; // property name
  TCHAR **strings;    // value
} I18NStrings;

typedef struct _stringListEntry {
  LPTSTR string;
  struct _stringListEntry *next;
} StringListEntry;

typedef struct _launchProps {

  LauncherResourceList *jars;
  LauncherResourceList *jvms;
  LauncherResourceList *other;

  LauncherResource *testJVMFile;

  TCHAR *testJVMClass;
  TCHAR *tmpDir;
  DWORD tmpDirCreated;
  int64t *bundledSize;
  DWORD bundledNumber;
  JavaCompatible **compatibleJava;
  DWORD compatibleJavaNumber;

  DWORD checkForFreeSpace;
  DWORD silent;
  TCHARList *jvmArguments;
  TCHARList *appArguments;

  DWORD extractOnly;
  TCHAR *classpath;
  TCHAR *mainClass;

  JavaProperties *java;
  TCHAR *command;
  TCHAR *exePath;
  TCHAR *exeDir;
  TCHAR *exeName;
  DWORD status;
  DWORD exitCode;
  DWORD silentMode;
  HANDLE handler;
  DWORD outputLevel;
  TCHARList *commandLine;
  HANDLE stdoutHandle;
  HANDLE stderrHandle;
  DWORD bufsize;
  int64t *launcherSize;
  DWORD isOnlyStub;
  TCHAR *userDefinedJavaHome;
  TCHAR *userDefinedTempDir;
  TCHAR *userDefinedExtractDir;
  TCHAR *userDefinedOutput;
  TCHAR *userDefinedLocale;
  SizedString *restOfBytes;
  I18NStrings *i18nMessages;
  DWORD I18N_PROPERTIES_NUMBER;
  StringListEntry *alreadyCheckedJava;
  TCHARList *launcherCommandArguments;
  TCHAR *defaultUserDirRoot;
  TCHAR *defaultCacheDirRoot;

} LauncherProperties;

#ifdef __cplusplus
}
#endif

#endif /* _Types_H */
