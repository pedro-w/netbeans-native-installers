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

#ifndef _StringUtils_H
#define _StringUtils_H
#include <windows.h>

#include "Errors.h"
#include "Types.h"
#include <stdarg.h>
#include <tchar.h>
#ifdef __cplusplus
extern "C" {
#endif

extern LPCTSTR JVM_NOT_FOUND_PROP;
extern LPCTSTR JVM_USER_DEFINED_ERROR_PROP;
extern LPCTSTR JVM_UNSUPPORTED_VERSION_PROP;
extern LPCTSTR NOT_ENOUGH_FREE_SPACE_PROP;
extern LPCTSTR CANT_CREATE_TEMP_DIR_PROP;
extern LPCTSTR INTEGRITY_ERROR_PROP;
extern LPCTSTR OUTPUT_ERROR_PROP;
extern LPCTSTR JAVA_PROCESS_ERROR_PROP;
extern LPCTSTR EXTERNAL_RESOURE_LACK_PROP;
extern LPCTSTR BUNDLED_JVM_EXTRACT_ERROR_PROP;
extern LPCTSTR BUNDLED_JVM_VERIFY_ERROR_PROP;
extern LPCTSTR ARG_OUTPUT_PROPERTY;
extern LPCTSTR ARG_JAVA_PROP;
extern LPCTSTR ARG_DEBUG_PROP;
extern LPCTSTR ARG_TMP_PROP;
extern LPCTSTR ARG_CPA_PROP;
extern LPCTSTR ARG_CPP_PROP;
extern LPCTSTR ARG_EXTRACT_PROP;
extern LPCTSTR ARG_DISABLE_SPACE_CHECK;
extern LPCTSTR ARG_LOCALE_PROP;
extern LPCTSTR ARG_SILENT_PROP;
extern LPCTSTR ARG_HELP_PROP;
extern LPCTSTR MSG_CREATE_TMPDIR;
extern LPCTSTR MSG_EXTRACT_DATA;
extern LPCTSTR MSG_JVM_SEARCH;
extern LPCTSTR MSG_SET_OPTIONS;
extern LPCTSTR MSG_RUNNING;
extern LPCTSTR MSG_TITLE;
extern LPCTSTR MSG_MESSAGEBOX_TITLE;
extern LPCTSTR MSG_PROGRESS_TITLE;
extern LPCTSTR EXIT_BUTTON_PROP;
extern LPCTSTR MAIN_WINDOW_TITLE;

#define FREE(x)                                                                \
  {                                                                            \
    if ((x) != NULL) {                                                         \
      LocalFree(x);                                                            \
      (x) = NULL;                                                              \
    }                                                                          \
  }

#ifdef _MSC_VER
#define ZERO(x, y) SecureZeroMemory((x), (y));
#else
#define ZERO(x, y) ZeroMemory((x), (y));
#endif

void freeI18NMessages(LauncherProperties *props);

void getI18nPropertyTitleDetail(LauncherProperties *props, LPCTSTR name,
                                LPTSTR *title, LPTSTR *detail);
LPCTSTR getI18nProperty(LauncherProperties *props, LPCTSTR name);
LPCTSTR getDefaultString(LPCTSTR name);

LPTSTR appendStringN(LPTSTR initial, DWORD initialLength, LPCTSTR addString,
                     DWORD addStringLength);
LPTSTR appendString(LPTSTR initial, LPCTSTR addString);
LPTSTR escapeString(LPCTSTR string);
LPTSTR copyString(LPCTSTR s);

void freeStringList(StringListEntry **s);
StringListEntry *addStringToList(StringListEntry *top, LPCTSTR str);
StringListEntry *splitStringToList(StringListEntry *top, LPCTSTR str,
                                   TCHAR sep);
BOOL inList(StringListEntry *top, LPCTSTR str);

char *toChar(const WCHAR *string);
char *toCharN(const WCHAR *string, DWORD length);
WCHAR *toWCHAR(const char *string);
WCHAR *toWCHARN(const char *string, DWORD length);

WCHAR *createWCHAR(SizedString *sz);
char *createCHAR(SizedString *sz);
WCHAR *createWCHARN(SizedString *sz);
char *createCHARN(SizedString *sz, DWORD maxsize);

SizedString *createSizedString();
void initSizedStringFrom(SizedString *s, BYTE *ptr, DWORD size);
void appendToSizedString(SizedString *s, BYTE *ptr, DWORD size);
char *int64ttoCHAR(int64t *);
WCHAR *int64ttoWCHAR(int64t *);
char *DWORDtoCHAR(DWORD);
char *DWORDtoCHARN(DWORD, int);

WCHAR *DWORDtoWCHAR(DWORD);
WCHAR *DWORDtoWCHARN(DWORD, int);

char *long2char(long value);
char *long2charN(long value, int fillZeros);

LPTSTR word2char(WORD value);
LPTSTR word2charN(WORD value, int fillZeros);

void freeSizedString(SizedString **s);

LPTSTR getLocaleName();

WCHAR *newpWCHAR(DWORD length);
char *newpChar(DWORD length);
TCHAR *search(LPCTSTR wcs1, LPCTSTR wcs2);
WCHAR **newppWCHAR(DWORD length);
char **newppChar(DWORD length);
int64t *newint64_t(DWORD low, DWORD high);
int compare(int64t *size, DWORD value);
int compareInt64t(int64t *a1, int64t *a2);
void plus(int64t *size, DWORD value);
void multiply(int64t *size, DWORD value);
void minus(int64t *size, DWORD value);
DWORD getLineSeparatorNumber(LPCTSTR str);
DWORD getLength(LPCTSTR message);

LPTSTR getErrorDescription(DWORD dw);
LPTSTR formatMessage(LPCTSTR message, const DWORD varArgsNumber, ...);
DWORD isOK(LauncherProperties *props);
#ifdef UNICODE
#define newpTCHAR newpWCHAR
#define newppTCHAR newppWCHAR
#define DWORDtoTCHAR DWORDtoWCHAR
#define DWORDtoTCHARN DWORDtoWCHARN
#define int64ttoTCHAR int64ttoWCHAR
#define WSTRtoTSTR copyString
#define createTCHAR createWCHAR
#else
#define newpTCHAR newpChar
#define newppTCHAR newppChar
#define DWORDtoTCHAR DWORDtoCHAR
#define DWORDtoTCHARN DWORDtoCHARN
#define int64ttoTCHAR int64ttoCHAR
#define WSTRtoTSTR toChar
#define createTCHAR createCHAR
#endif

#ifdef __cplusplus
}
#endif

#endif /* _StringUtils_H */
