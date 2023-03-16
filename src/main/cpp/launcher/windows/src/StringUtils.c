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

#include "StringUtils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <windows.h>
#include <winnls.h>

LPCTSTR JVM_NOT_FOUND_PROP = TEXT("nlw.jvm.notfoundmessage");
LPCTSTR JVM_USER_DEFINED_ERROR_PROP = TEXT("nlw.jvm.usererror");

LPCTSTR JVM_UNSUPPORTED_VERSION_PROP = TEXT("nlw.jvm.unsupportedversion");
LPCTSTR NOT_ENOUGH_FREE_SPACE_PROP = TEXT("nlw.freespace");
LPCTSTR CANT_CREATE_TEMP_DIR_PROP = TEXT("nlw.tmpdir");
LPCTSTR INTEGRITY_ERROR_PROP = TEXT("nlw.integrity");
LPCTSTR OUTPUT_ERROR_PROP = TEXT("nlw.output.error");
LPCTSTR JAVA_PROCESS_ERROR_PROP = TEXT("nlw.java.process.error");
LPCTSTR EXTERNAL_RESOURE_LACK_PROP = TEXT("nlw.missing.external.resource");
LPCTSTR BUNDLED_JVM_EXTRACT_ERROR_PROP = TEXT("nlw.bundled.jvm.extract.error");
LPCTSTR BUNDLED_JVM_VERIFY_ERROR_PROP = TEXT("nlw.bundled.jvm.verify.error");

LPCTSTR ARG_OUTPUT_PROPERTY = TEXT("nlw.arg.output");
LPCTSTR ARG_JAVA_PROP = TEXT("nlw.arg.javahome");
LPCTSTR ARG_DEBUG_PROP = TEXT("nlw.arg.verbose");
LPCTSTR ARG_TMP_PROP = TEXT("nlw.arg.tempdir");
LPCTSTR ARG_CPA_PROP = TEXT("nlw.arg.classpatha");
LPCTSTR ARG_CPP_PROP = TEXT("nlw.arg.classpathp");
LPCTSTR ARG_EXTRACT_PROP = TEXT("nlw.arg.extract");
LPCTSTR ARG_DISABLE_SPACE_CHECK = TEXT("nlw.arg.disable.space.check");
LPCTSTR ARG_LOCALE_PROP = TEXT("nlw.arg.locale");
LPCTSTR ARG_SILENT_PROP = TEXT("nlw.arg.silent");
LPCTSTR ARG_HELP_PROP = TEXT("nlw.arg.help");

LPCTSTR MSG_CREATE_TMPDIR = TEXT("nlw.msg.create.tmpdir");
LPCTSTR MSG_EXTRACT_DATA = TEXT("nlw.msg.extract");
LPCTSTR MSG_JVM_SEARCH = TEXT("nlw.msg.jvmsearch");
LPCTSTR MSG_SET_OPTIONS = TEXT("nlw.msg.setoptions");
LPCTSTR MSG_RUNNING = TEXT("nlw.msg.running");
LPCTSTR MSG_TITLE = TEXT("nlw.msg.title");
LPCTSTR MSG_MESSAGEBOX_TITLE = TEXT("nlw.msg.messagebox.title");
LPCTSTR MSG_PROGRESS_TITLE = TEXT("nlw.msg.progress.title");

LPCTSTR EXIT_BUTTON_PROP = TEXT("nlw.msg.button.error");
LPCTSTR MAIN_WINDOW_TITLE = TEXT("nlw.msg.main.title");

// adds string the the initial string and modifies totalWCHARs and capacity
// initial - the beginning of the string
// size - pointer to the value that contains the length of the initial string
//        It is modified to store returning string length
// addString - additional string
//

void freeI18NMessages(LauncherProperties *props) {
  if (props->i18nMessages != NULL) {
    DWORD i = 0;

    for (i = 0; i < props->I18N_PROPERTIES_NUMBER; i++) {
      FREE(props->i18nMessages->properties[i]);
      FREE(props->i18nMessages->strings[i]);
    }
    FREE(props->i18nMessages->properties);
    FREE(props->i18nMessages->strings);
    FREE(props->i18nMessages);
  }
}

TCHAR *search(LPCTSTR wcs1, LPCTSTR wcs2) {
  TCHAR *cp = (TCHAR *)wcs1;
  TCHAR *s1, *s2;

  if (!*wcs2) {
    return (TCHAR *)wcs1;
  }

  while (*cp) {
    s1 = cp;
    s2 = (TCHAR *)wcs2;

    while (*s1 && *s2 && !(*s1 - *s2)) {
      s1++, s2++;
    }
    if (!*s2) {
      return cp;
    }
    cp++;
  }
  return NULL;
}

void getI18nPropertyTitleDetail(LauncherProperties *props, LPCTSTR name,
                                LPTSTR *title, LPTSTR *detail) {
  LPCTSTR prop = getI18nProperty(props, name);
  LPCTSTR detailStringSep = search(prop, TEXT("\n"));

  if (detailStringSep == NULL) {
    *title = appendString(NULL, prop);
    *detail = NULL;
  } else {
    DWORD dif = getLength(prop) - getLength(detailStringSep);
    *title = appendStringN(NULL, 0, prop, dif);
    *detail = appendString(NULL, prop + (dif + 1));
  }
}
LPCTSTR getI18nProperty(LauncherProperties *props, LPCTSTR name) {
  if (name == NULL)
    return NULL;
  if (name != NULL && props->i18nMessages != NULL) {
    DWORD i;
    for (i = 0; i < props->I18N_PROPERTIES_NUMBER; i++) {
      LPCTSTR pr = props->i18nMessages->properties[i];
      if (pr != NULL) { // hope so it`s true
        if (lstrcmp(name, pr) == 0) {
          return props->i18nMessages->strings[i];
        }
      }
    }
  }
  return getDefaultString(name);
}

LPCTSTR getDefaultString(LPCTSTR name) {
  if (lstrcmp(name, JVM_NOT_FOUND_PROP) == 0) {
    return TEXT("Can`t find suitable JVM. Specify it with %s argument");
  } else if (lstrcmp(name, NOT_ENOUGH_FREE_SPACE_PROP) == 0) {
    return TEXT("Not enought free space at %s");
  } else if (lstrcmp(name, CANT_CREATE_TEMP_DIR_PROP) == 0) {
    return TEXT("Can`t create temp directory %s");
  } else if (lstrcmp(name, INTEGRITY_ERROR_PROP) == 0) {
    return TEXT("Integrity error. File %s is corrupted");
  } else if (lstrcmp(name, JVM_USER_DEFINED_ERROR_PROP) == 0) {
    return TEXT("Can`t find JVM at %s");
  } else if (lstrcmp(name, JVM_UNSUPPORTED_VERSION_PROP) == 0) {
    return TEXT("Unsupported JVM at %s");
  } else if (lstrcmp(name, OUTPUT_ERROR_PROP) == 0) {
    return TEXT("Can`t create file %s.\nError: %s");
  } else if (lstrcmp(name, JAVA_PROCESS_ERROR_PROP) == 0) {
    return TEXT("Java error:\n%s");
  } else if (lstrcmp(name, ARG_JAVA_PROP) == 0) {
    return TEXT("%s Using specified JVM");
  } else if (lstrcmp(name, ARG_OUTPUT_PROPERTY) == 0) {
    return TEXT("%s Output all stdout/stderr to the file");
  } else if (lstrcmp(name, ARG_DEBUG_PROP) == 0) {
    return TEXT("%s Use verbose output");
  } else if (lstrcmp(name, ARG_TMP_PROP) == 0) {
    return TEXT("%s Use specified temporary dir for extracting data");
  } else if (lstrcmp(name, ARG_CPA_PROP) == 0) {
    return TEXT("%s Append classpath");
  } else if (lstrcmp(name, ARG_CPP_PROP) == 0) {
    return TEXT("%s Prepend classpath");
  } else if (lstrcmp(name, ARG_EXTRACT_PROP) == 0) {
    return TEXT("%s Extract all data");
  } else if (lstrcmp(name, ARG_HELP_PROP) == 0) {
    return TEXT("%s Using this help");
  } else if (lstrcmp(name, ARG_DISABLE_SPACE_CHECK) == 0) {
    return TEXT("%s Disable free space check");
  } else if (lstrcmp(name, ARG_LOCALE_PROP) == 0) {
    return TEXT("%s Use specified locale for messages");
  } else if (lstrcmp(name, ARG_SILENT_PROP) == 0) {
    return TEXT("%s Run silently");
  } else if (lstrcmp(name, MSG_CREATE_TMPDIR) == 0) {
    return TEXT("Creating tmp directory...");
  } else if (lstrcmp(name, MSG_EXTRACT_DATA) == 0) {
    return TEXT("Extracting data...");
  } else if (lstrcmp(name, MSG_JVM_SEARCH) == 0) {
    return TEXT("Finding JVM...");
  } else if (lstrcmp(name, MSG_RUNNING) == 0) {
    return TEXT("Running JVM...");
  } else if (lstrcmp(name, MSG_SET_OPTIONS) == 0) {
    return TEXT("Setting command options...");
  } else if (lstrcmp(name, MSG_MESSAGEBOX_TITLE) == 0) {
    return TEXT("Message");
  } else if (lstrcmp(name, MSG_PROGRESS_TITLE) == 0) {
    return TEXT("Running");
  } else if (lstrcmp(name, EXIT_BUTTON_PROP) == 0) {
    return TEXT("Exit");
  } else if (lstrcmp(name, MAIN_WINDOW_TITLE) == 0) {
    return TEXT("NBI Launcher");
  } else if (lstrcmp(name, EXTERNAL_RESOURE_LACK_PROP) == 0) {
    return TEXT("Can`t run launcher\nThe following file is missing : %s");
  } else if (lstrcmp(name, BUNDLED_JVM_EXTRACT_ERROR_PROP) == 0) {
    return TEXT("Can`t run prepare bundled JVM");
  } else if (lstrcmp(name, BUNDLED_JVM_VERIFY_ERROR_PROP) == 0) {
    return TEXT("Can`t run verify bundled JVM");
  }
  return NULL;
}

DWORD getLength(LPCTSTR message) {
  return (message != NULL) ? lstrlen(message) : 0;
}

// adds string to the initial string which is freed
LPTSTR appendStringN(LPTSTR initial, DWORD initialLength, LPCTSTR addString,
                     DWORD addStringLength) {
  DWORD length = initialLength + addStringLength;
  if (length > 1) {
    LPTSTR tmp = newpTCHAR(length + 1);
    DWORD i = 0;
    if (initialLength != 0) {
      for (i = 0; i < initialLength; i++) {
        tmp[i] = initial[i];
      }
      FREE(initial);
    }
    for (i = 0; i < addStringLength; i++) {
      tmp[i + initialLength] = addString[i];
    }

    return tmp;
  } else {
    return NULL;
  }
}

LPTSTR appendString(LPTSTR initial, LPCTSTR addString) {
  return appendStringN(initial, getLength(initial), addString,
                       getLength(addString));
}

LPTSTR escapeString(LPCTSTR string) {
  DWORD length = getLength(string);
  LPTSTR result = newpTCHAR(length * 2 + 4);
  DWORD i = 0;
  DWORD r = 0;
  DWORD bsCounter = 0;
  int quoting = search(string, TEXT(" ")) || search(string, TEXT("\t"));
  if (quoting) {
    result[r++] = TEXT('"');
  }
  for (i = 0; i < length; i++) {
    TCHAR c = string[i];
    switch (c) {
    case TEXT('\\'):
      bsCounter++;
      break;
    case TEXT('"'):
      bsCounter++;
      do {
        result[r++] = TEXT('\\');
        bsCounter--;
      } while (bsCounter > 0);
      break;
    default:
      bsCounter = 0;
      break;
    }
    result[r++] = c;
  }
  if (quoting) {
    while (bsCounter > 0) {
      result[r++] = TEXT('\\');
      bsCounter--;
    }
    result[r++] = TEXT('"');
  }
  result[r] = 0;
  return result;
}

char *DWORDtoCHARN(DWORD value, int fillZeros) {
  int digits = 0;
  DWORD tmpValue = value;
  int i = 0;
  char *str;

  do {
    digits++;
    tmpValue = tmpValue / 10;
  } while (tmpValue != 0);
  tmpValue = value;
  if (digits < fillZeros) {
    digits = fillZeros;
  }
  str = (char *)LocalAlloc(LPTR, sizeof(char) * (digits + 1));
  str[digits] = '\0';
  for (i = 0; i < digits; i++) {
    str[digits - i - 1] = '0' + (char)(tmpValue - ((tmpValue / 10) * 10));
    tmpValue = tmpValue / 10;
  }
  return str;
}

WCHAR *DWORDtoWCHARN(DWORD value, int fillZeros) {
  int digits = 0;
  DWORD tmpValue = value;
  int i = 0;
  WCHAR *str;

  do {
    digits++;
    tmpValue = tmpValue / 10;
  } while (tmpValue != 0);
  tmpValue = value;
  if (digits < fillZeros) {
    digits = fillZeros;
  }

  str = (WCHAR *)LocalAlloc(LPTR, sizeof(WCHAR) * (digits + 1));
  str[digits] = L'\0';
  for (i = 0; i < digits; i++) {
    str[digits - i - 1] = L'0' + (WCHAR)(tmpValue - ((tmpValue / 10) * 10));
    tmpValue = tmpValue / 10;
  }
  return str;
}
WCHAR *DWORDtoWCHAR(DWORD value) { return DWORDtoWCHARN(value, 0); }

char *DWORDtoCHAR(DWORD value) { return DWORDtoCHARN(value, 0); }

char *long2charN(long value, int fillZeros) {
  int digits = 0;
  long tmpValue = value;
  int i = 0;
  char *str;

  do {
    digits++;
    tmpValue = tmpValue / 10;
  } while (tmpValue != 0);
  tmpValue = value;
  if (digits < fillZeros) {
    digits = fillZeros;
  }
  str = (char *)LocalAlloc(LPTR, sizeof(char) * (digits + 1));
  str[digits] = '\0';
  for (i = 0; i < digits; i++) {
    str[digits - i - 1] = '0' + (char)(tmpValue - ((tmpValue / 10) * 10));
    tmpValue = tmpValue / 10;
  }
  return str;
}

char *long2char(long value) { return long2charN(value, 0); }

LPTSTR word2charN(WORD value, int fillZeros) {
  int digits = 0;
  WORD tmpValue = value;
  int i = 0;
  LPTSTR str;

  do {
    digits++;
    tmpValue = tmpValue / 10;
  } while (tmpValue != 0);
  tmpValue = value;
  if (digits < fillZeros) {
    digits = fillZeros;
  }
  str = newpTCHAR(digits + 1);
  str[digits] = TEXT('\0');
  for (i = 0; i < digits; i++) {
    str[digits - i - 1] =
        TEXT('0') + (TCHAR)(tmpValue - ((tmpValue / 10) * 10));
    tmpValue = tmpValue / 10;
  }
  return str;
}

LPTSTR word2char(WORD value) { return word2charN(value, 0); }

LPTSTR int64ttoTCHAR(int64t *value) {
  if (value->High == 0) {
    return DWORDtoTCHAR(value->Low);
  } else {
    LPTSTR high = DWORDtoTCHAR(value->High);
    LPTSTR low = DWORDtoTCHAR(value->Low);

    LPTSTR str = appendString(NULL, TEXT("[H][L]="));
    str = appendString(str, high);
    str = appendString(str, TEXT(","));
    str = appendString(str, low);
    FREE(high);
    FREE(low);
    return str;
  }
}

void freeStringList(StringListEntry **ss) {
  while ((*ss) != NULL) {
    StringListEntry *tmp = (*ss)->next;
    FREE((*ss)->string);
    FREE((*ss));
    *ss = tmp;
  }
}

BOOL inList(StringListEntry *top, LPCTSTR str) {
  StringListEntry *tmp = top;
  while (tmp != NULL) {
    if (lstrcmp(tmp->string, str) == 0) {
      return TRUE;
    }
    tmp = tmp->next;
  }
  return FALSE;
}

StringListEntry *addStringToList(StringListEntry *top, LPCTSTR str) {
  StringListEntry *ss =
      (StringListEntry *)LocalAlloc(LPTR, sizeof(StringListEntry));
  ss->string = appendString(NULL, str);
  ss->next = top;
  return ss;
}

StringListEntry *splitStringToList(StringListEntry *top, LPCTSTR strlist,
                                   TCHAR sep) {
  if (strlist != NULL) {
    const TCHAR *start = strlist;
    while (*strlist != 0) {
      if (*strlist == 0 || *strlist == sep) {
        top = addStringToList(top,
                              appendStringN(NULL, 0, start, strlist - start));
        start = ++strlist;
      } else {
        ++strlist;
      }
    }
  }
  return top;
}

DWORD getLineSeparatorNumber(LPCTSTR str) {
  DWORD result = 0;
  TCHAR *ptr = (TCHAR *)str;
  if (ptr != NULL) {
    while ((ptr = search(ptr, TEXT("\n"))) != NULL) {
      ptr++;
      result++;
      if (ptr == NULL)
        break;
    }
  }
  return result;
}

char *toCharN(const WCHAR *string, DWORD n) {
  DWORD len = 0;
  DWORD length = 0;
  char *str = NULL;
  if (string == NULL)
    return NULL;
  // static DWORD excludeCodepages [] = { 50220, 50221, 50222, 50225, 50227,
  // 50229, 52936, 54936, 57002,  57003, 57004, 57005, 57006, 57007, 57008,
  // 57009, 57010, 57011, 65000, 42}; int symbols = 0;
  len = wcslen(string);
  if (n < len)
    len = n;
  length = WideCharToMultiByte(CP_ACP, 0, string, len, NULL, 0, 0, NULL);
  str = newpChar(length + 1);
  WideCharToMultiByte(CP_ACP, 0, string, len, str, length, 0, NULL);
  return str;
}

char *toChar(const WCHAR *string) {
  return string ? toCharN(string, wcslen(string)) : NULL;
}
char *createCHAR(SizedString *sz) {
  char *str = memcpy(newpChar(sz->length + 1), sz->bytes, sz->length);
  str[sz->length] = 0;
  return str;
}
// TODO delete this when not needed
char *createCHARN(SizedString *sz, DWORD maxlen) {
  if (maxlen > sz->length) {
    maxlen = sz->length;
  }

  char *str = newpChar(maxlen + 1);
  DWORD i;
  for (i = 0; i < maxlen; ++i) {
    str[i] = sz->bytes[i] ? sz->bytes[i] : '.';
  }
  str[maxlen] = 0;
  return str;
}
WCHAR *createWCHAR(SizedString *sz) {
  const BYTE *str = sz->bytes;
  DWORD len = sz->length;
  int unicodeFlags;
  DWORD i;
  BYTE *string;
  BYTE *ptr = NULL;
  WCHAR *wstr = NULL;
  if (str == NULL)
    return NULL;

  string = memcpy(newpChar(len + 1), str, len);
  ptr = string;

  unicodeFlags = IS_TEXT_UNICODE_REVERSE_MASK | IS_TEXT_UNICODE_UNICODE_MASK;
  IsTextUnicode(str, len, &unicodeFlags);

  if (unicodeFlags &
      (IS_TEXT_UNICODE_REVERSE_SIGNATURE | IS_TEXT_UNICODE_SIGNATURE)) {
    // Has a BOM
    len -= 2;
    ptr += 2;
  }
  if (unicodeFlags & (IS_TEXT_UNICODE_REVERSE_STATISTICS |
                      IS_TEXT_UNICODE_REVERSE_SIGNATURE)) {
    // we need to change bytes order
    for (i = 0; i < len; i += 2) {
      BYTE c = ptr[i];
      ptr[i] = ptr[i + 1];
      ptr[i + 1] = c;
    }
  }
  wstr = newpWCHAR(len + 1);
  WCHAR *wptr = wstr;
  if (unicodeFlags &
      (IS_TEXT_UNICODE_REVERSE_STATISTICS | IS_TEXT_UNICODE_REVERSE_SIGNATURE |
       IS_TEXT_UNICODE_REVERSE_SIGNATURE | IS_TEXT_UNICODE_SIGNATURE)) {
    // Yes it really is unicode
    for (i = 0; i < len; i += 2) {
      *wptr++ = (ptr[i + 1] & 0xFF) << 8 | (ptr[i] & 0xFF);
    }
    *wptr = L'\0';
  } else {
    // TODO this all needs rethinking.
    // Doesn't seem to be unicode
      return toWCHARN((const char*) str, len);
  }
  FREE(string);
  return wstr;
}
WCHAR *toWCHARN(const char *str, DWORD n) {
  DWORD len = 0;
  DWORD length = 0;
  WCHAR *wstr = NULL;
  if (str == NULL)
    return NULL;
  // static DWORD excludeCodepages [] = { 50220, 50221, 50222, 50225, 50227,
  // 50229, 52936, 54936, 57002,  57003, 57004, 57005, 57006, 57007, 57008,
  // 57009, 57010, 57011, 65000, 42};
  len = strlen(str);
  if (n < len)
    len = n;
  length = MultiByteToWideChar(CP_ACP, 0, str, len, NULL, 0);
  wstr = newpWCHAR(length + 1);
  MultiByteToWideChar(CP_ACP, 0, str, len, wstr, length);
  return wstr;
}

WCHAR *toWCHAR(const char *string) {
  return string ? toWCHARN(string, strlen(string)) : NULL;
}

SizedString *createSizedString() {
  SizedString *s = (SizedString *)LocalAlloc(LPTR, sizeof(SizedString));
  s->bytes = NULL;
  s->length = 0;
  return s;
}

void initSizedStringFrom(SizedString *s, BYTE *p, DWORD size) {
  FREE(s->bytes);
  if (size > 0) {
    s->bytes = (BYTE *)LocalAlloc(LPTR, size * sizeof(BYTE));
    memcpy(s->bytes, p, size);
  }
  s->length = size;
}
void appendToSizedString(SizedString *s, BYTE *p, DWORD size) {
  DWORD newSize = size + s->length;
  BYTE *newBytes = (BYTE *)LocalAlloc(LPTR, newSize * sizeof(BYTE));
  if (s->bytes) {
    memcpy(newBytes, s->bytes, s->length);
    FREE(s->bytes);
  }
  memcpy(newBytes + s->length, p, size);
  s->bytes = newBytes;
  s->length = newSize;
}

void freeSizedString(SizedString **s) {
  SizedString *ss = *s;
  if (ss != NULL) {
    FREE(ss->bytes);
    FREE(ss);
  }
  *s = NULL;
}

LPTSTR getLocaleName() {
  LANGID langID = LANGIDFROMLCID(GetUserDefaultLCID());
  LCID localeID = MAKELCID(langID, SORT_DEFAULT);
  const DWORD MAX_LENGTH = 512;
  LPTSTR lang = newpTCHAR(MAX_LENGTH);
  LPTSTR country = newpTCHAR(MAX_LENGTH);
  LPTSTR locale = NULL;
  GetLocaleInfo(localeID, LOCALE_SISO639LANGNAME, lang, MAX_LENGTH);
  GetLocaleInfo(localeID, LOCALE_SISO3166CTRYNAME, country, MAX_LENGTH);
  locale =
      appendString(appendString(appendString(NULL, lang), TEXT("_")), country);
  FREE(country);
  FREE(lang);
  return locale;
}

WCHAR *newpWCHAR(DWORD length) {
  WCHAR *res = (WCHAR *)LocalAlloc(LPTR, sizeof(WCHAR) * length);
  ZERO(res, length * sizeof(WCHAR));
  return res;
}
WCHAR **newppWCHAR(DWORD length) {
  return (WCHAR **)LocalAlloc(LPTR, sizeof(WCHAR *) * length);
}

char *newpChar(DWORD length) {
  char *res = (char *)LocalAlloc(LPTR, sizeof(char) * length);
  ZERO(res, length * sizeof(char));
  return res;
}

char **newppChar(DWORD length) {
  return (char **)LocalAlloc(LPTR, sizeof(char *) * length);
}

int compare(int64t *size, DWORD value) {
  if (size->High > 0)
    return 1;

  if (size->Low > value)
    return 1;
  else if (size->Low == value)
    return 0;
  else // if(size->Low < value)
    return -1;
}
int compareInt64t(int64t *a1, int64t *a2) {
  if (a1->High > a2->High) {
    return 1;
  } else if (a1->High == a2->High) {
    if (a1->Low > a2->Low) {
      return 1;
    } else if (a1->Low == a2->Low) {
      return 0;
    }
  }
  return -1;
}

void plus(int64t *size, DWORD value) {
  if (value != 0) {
    if ((MAXDWORD - size->Low) >= (value - 1)) {
      size->Low = size->Low + value;
    } else {
      size->High = size->High + 1;
      size->Low = value - (MAXDWORD - size->Low) - 1;
    }
  }
}
void multiply(int64t *size, DWORD value) {
  if (value == 0) {
    size->Low = 0;
    size->High = 0;
  } else {
    DWORD i = 0;
    DWORD low = size->Low;
    DWORD high = size->High;
    size->High = 0;
    for (; i < value - 1; i++) {
      plus(size, low);
    }
    size->High += high * value;
  }
}

void minus(int64t *size, DWORD value) {
  if (value != 0) {
    if (size->Low < value) {
      size->High = size->High - 1;
      size->Low = size->Low + (MAXDWORD - value) + 1;
    } else {
      size->Low = size->Low - value;
    }
  }
}
int64t *newint64_t(DWORD low, DWORD high) {
  int64t *res = (int64t *)LocalAlloc(LPTR, sizeof(int64t));
  res->Low = low;
  res->High = high;
  return res;
}
LPTSTR getErrorDescription(DWORD dw) {
  LPTSTR lpMsgBuf;
  LPTSTR lpDisplayBuf = NULL;
  LPTSTR res = DWORDtoTCHAR(dw);

  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&lpMsgBuf, 0, NULL);

  lpDisplayBuf = appendString(lpDisplayBuf, TEXT("Error code ("));
  lpDisplayBuf = appendString(lpDisplayBuf, res);
  lpDisplayBuf = appendString(lpDisplayBuf, TEXT("): "));
  lpDisplayBuf = appendString(lpDisplayBuf, lpMsgBuf);

  FREE(lpMsgBuf);
  FREE(res);

  return lpDisplayBuf;
}

LPTSTR formatMessage(LPCTSTR message, const DWORD varArgsNumber, ...) {
  DWORD totalLength = getLength(message);
  DWORD counter = 0;
  LPTSTR result = NULL;

  va_list ap;
  va_start(ap, varArgsNumber);

  while ((counter++) < varArgsNumber) {
    LPTSTR arg = va_arg(ap, LPTSTR);
    totalLength += getLength(arg);
  }
  va_end(ap);

  result = newpTCHAR(totalLength + 1);
  va_start(ap, varArgsNumber);
  wvsprintf(result, message, ap);
  va_end(ap);
  return result;
}

DWORD isOK(LauncherProperties *props) { return (props->status == ERROR_OK); }

LPTSTR copyString(LPCTSTR s) { return appendString(NULL, s); }
