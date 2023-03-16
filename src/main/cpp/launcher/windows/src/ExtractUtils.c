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

#include "ExtractUtils.h"
#include "FileUtils.h"
#include "JavaUtils.h"
#include "Launcher.h"
#include "Main.h"
#include "RegistryUtils.h"
#include "StringUtils.h"
#include "SystemUtils.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

const DWORD STUB_FILL_SIZE = 450000;

void skipLauncherStub(LauncherProperties *props, DWORD stubSize) {
  HANDLE hFileRead = props->handler;

  if (hFileRead != INVALID_HANDLE_VALUE) {
    DWORD status = SetFilePointer(hFileRead, (LONG)stubSize, NULL, FILE_BEGIN);
    if (status == INVALID_SET_FILE_POINTER) {
      props->status = ERROR_INTEGRITY;
    } else {
      TCHAR tmp[128];
      wsprintf(tmp, TEXT("Stub, skipped to %ld"), status);
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, tmp, 1);
    }
  }
}

void skipStub(LauncherProperties *props) {
  if (props->isOnlyStub) {
    LPTSTR os = NULL;
    props->status = EXIT_CODE_STUB;
    os = appendString(os, TEXT("It`s only the launcher stub.\nOS: "));
    if (is9x())
      os = appendString(os, TEXT("Windows 9x"));
    if (isNT())
      os = appendString(os, TEXT("Windows NT"));
    if (is2k())
      os = appendString(os, TEXT("Windows 2000"));
    if (isXP())
      os = appendString(os, TEXT("Windows XP"));
    if (is2003())
      os = appendString(os, TEXT("Windows 2003"));
    if (isVista())
      os = appendString(os, TEXT("Windows Vista"));
    if (is2008())
      os = appendString(os, TEXT("Windows 2008"));
    if (is7())
      os = appendString(os, TEXT("Windows 7"));
    if (IsWow64)
      os = appendString(os, TEXT(" x64"));
    showMessage(props, os, 0);
    FREE(os);
  } else {
    skipLauncherStub(props, STUB_FILL_SIZE);
    if (!isOK(props)) {
      writeMessage(props, OUTPUT_LEVEL_NORMAL, 1,
                   TEXT("Error! Can`t process launcher stub"), 1);
      showError(props, INTEGRITY_ERROR_PROP, 1, props->exeName);
    }
  }
}

void modifyRestBytes(SizedString *rest, DWORD start) {

  rest->length -= start;
  memmove(rest->bytes, rest->bytes + start, rest->length);
}
// Move from 'rest' into 'result' up to a '\0', either in unicode (i.e. UTF-16)
// mode or not-unicode mode
DWORD readStringFromBuf(SizedString *rest, SizedString *result,
                        BOOL isUnicode) {
  if (rest->length > 0) {
    // we have smth in the restBytes that we have read but haven`t yet proceeded
    DWORD i;
    if (isUnicode) {
      for (i = 0; i < rest->length; i += sizeof(WCHAR)) {
        WCHAR *ptr = (WCHAR *)(rest->bytes + i);
        if (*ptr == L'\0') {
          initSizedStringFrom(result, rest->bytes, i);
          modifyRestBytes(rest, i + sizeof(WCHAR));
          return ERROR_OK;
        }
      }
    } else {
      for (i = 0; i < rest->length; ++i) {
        char *ptr = (char *)(rest->bytes + i);
        if (*ptr == '\0') {
          initSizedStringFrom(result, rest->bytes, i);
          modifyRestBytes(rest, i + 1);
          return ERROR_OK;
        }
      }
    }
    // here we have found no \0 character in the rest of bytes...
  }
  return ERROR_INPUTOUPUT;
}

void readString(LauncherProperties *props, SizedString *result,
                BOOL isUnicode) {
  DWORD *status = &props->status;
  HANDLE hFileRead = props->handler;
  SizedString *rest = props->restOfBytes;
  DWORD bufferSize = props->bufsize;
  DWORD read = 0;
  BYTE *buf = NULL;

  if (*status != ERROR_OK)
    return;

  if (readStringFromBuf(rest, result, isUnicode) == ERROR_OK) {
    return;
  }

  // we need to read file for more data to find \0 character...
  buf = (BYTE *)newpChar(bufferSize);
  while (ReadFile(hFileRead, buf, bufferSize, &read, 0)) {
    addProgressPosition(props, read);
    appendToSizedString(rest, buf, read);
    if (readStringFromBuf(rest, result, isUnicode) == ERROR_OK) {
      // we have found \0 character
      break;
    }

    ZERO(buf, sizeof(char) * bufferSize);
    if (read == 0) { // we have nothing to read.. smth wrong
      *status = ERROR_INTEGRITY;
      break;
    }
  }
  FREE(buf);
  return;
}

void readNumber(LauncherProperties *props, DWORD *result) {
  if (isOK(props)) {

    SizedString *numberString = createSizedString();
    DWORD i;
    DWORD number = 0;

    readString(props, numberString, FALSE);

    if (!isOK(props)) {
      freeSizedString(&numberString);
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 1,
                   TEXT("Error!! Can`t read number string. Most probably "
                        "integrity error."),
                   1);
      return;
    }

    if (numberString->bytes == NULL) {
      freeSizedString(&numberString);
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 1,
                   TEXT("Error!! Can`t read number string (it can`t be NULL). "
                        "Most probably integrity error."),
                   1);
      props->status = ERROR_INTEGRITY;
      return;
    }

    for (i = 0; i < numberString->length; i++) {
      char c = numberString->bytes[i];
      if (c >= '0' && c <= '9') {
        number = number * 10 + (c - '0');
      } else if (c == 0) {
        // we have reached the end of number section
        writeMessage(
            props, OUTPUT_LEVEL_DEBUG, 1,
            TEXT("Can`t read number from string (it contains zero character):"),
            1);
        // TODO this cast is almost certainly wrong
        writeMessage(props, OUTPUT_LEVEL_DEBUG, 1, (LPTSTR)numberString->bytes,
                     1);
        props->status = ERROR_INTEGRITY;
        break;
      } else {
        // unexpected...
        writeMessage(props, OUTPUT_LEVEL_DEBUG, 1,
                     TEXT("Can`t read number from string (unexpected error):"),
                     1);
        LPTSTR what = createTCHAR(numberString);
        writeMessage(props, OUTPUT_LEVEL_DEBUG, 1, what, 1);
        FREE(what);
        props->status = ERROR_INTEGRITY;
        break;
      }
    }
    freeSizedString(&numberString);
    *result = number;
  } else {
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 1,
                 TEXT("Error!! Reading number and not ok."), 1);
  }
}

void readStringWithDebug(LauncherProperties *props, LPTSTR *dest,
                         LPCTSTR paramName, BOOL isUnicode) {
  SizedString *sizedStr = createSizedString();
  if (paramName != NULL) {
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("Reading "), 0);
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, paramName, 0);
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT(" : "), 0);
  }
  readString(props, sizedStr, isUnicode);
  if (!isOK(props)) {
    freeSizedString(&sizedStr);
    writeMessage(
        props, OUTPUT_LEVEL_DEBUG, 0,
        TEXT("[ERROR] Can`t read string !! Seems to be integrity error"), 1);
    return;
  }
  /*
  int fl= IS_TEXT_UNICODE_UNICODE_MASK|IS_TEXT_UNICODE_REVERSE_MASK;
  BOOL itu = IsTextUnicode(sizedStr->bytes, sizedStr->length, &fl);
  if (fl & IS_TEXT_UNICODE_STATISTICS) {
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("unicode stats"), 1);
  }
  if (fl & IS_TEXT_UNICODE_REVERSE_STATISTICS) {
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("reverse unicode stats"),
  1);
  }
  if (fl & IS_TEXT_UNICODE_SIGNATURE) {
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("unicode sig"), 1);
  }
  if (fl & IS_TEXT_UNICODE_REVERSE_SIGNATURE) {
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("reverse unicode sig"),
  1);
  }
  */
  if (isUnicode) {
#ifdef UNICODE
    *dest = createWCHAR(sizedStr);
#else
    LPWSTR tmp = createWCHAR(sizedStr);
    *dest = toChar(tmp);
    FREE(tmp);
#endif
  } else {
#ifdef UNICODE
    LPSTR tmp = createCHAR(sizedStr);
    *dest = toWCHAR(tmp);
    FREE(tmp);
#else
    *dest = createCHAR(sizedStr);
#endif
  }
  freeSizedString(&sizedStr);
  if (paramName != NULL) {
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, *dest ? *dest : TEXT("NULL"), 1);
  }
  return;
}
/*
void readStringWithDebugA(LauncherProperties *props, char **dest,
                          char *paramName) {
  SizedString *sizedStr = createSizedString();

  if (paramName != NULL) {
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("Reading "), 0);
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, paramName, 0);
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT(" : "), 0);
  }
  readString(props, sizedStr, 0);
  if (!isOK(props)) {
    freeSizedString(&sizedStr);
    writeMessage(
        props, OUTPUT_LEVEL_DEBUG, 0,
        TEXT("[ERROR] Can`t read string!!! Seems to be integrity error"), 1);
    return;
  }
  *dest = appendString(NULL, sizedStr->bytes);
  if (paramName != NULL) {
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, *dest ? *dest : TEXT("NULL"), 1);
  }
  freeSizedString(&sizedStr);
  return;
}
*/
void readNumberWithDebug(LauncherProperties *props, DWORD *dest,
                         LPCTSTR paramName) {
  writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("Reading "), 0);
  writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, paramName, 0);
  writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT(" : "), 0);
  readNumber(props, dest);

  if (!isOK(props)) {
    writeMessage(
        props, OUTPUT_LEVEL_DEBUG, 0,
        TEXT("[ERROR] Can`t read number !!! Seems to be integrity error"), 1);
    return;
  }
  writeDWORD(props, OUTPUT_LEVEL_DEBUG, 0, NULL, *dest, 1);
  return;
}

void readBigNumberWithDebug(LauncherProperties *props, int64t *dest,
                            LPCTSTR paramName) {
  DWORD low = 0;
  DWORD high = 0;
  writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("Reading "), 0);
  writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, paramName, 0);
  writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT(" : "), 0);

  readNumber(props, &low);
  if (isOK(props)) {
    readNumber(props, &high);
  }
  if (!isOK(props)) {
    writeMessage(
        props, OUTPUT_LEVEL_DEBUG, 0,
        TEXT("[ERROR] Can`t read number !!! Seems to be integrity error"), 1);
    return;
  }
  dest->High = high;
  dest->Low = low;
  writeint64t(props, OUTPUT_LEVEL_DEBUG, 0, TEXT(""), dest, 1);
}

// returns: ERROR_OK, ERROR_INPUTOUPUT, ERROR_INTEGRITY
void extractDataToFile(LauncherProperties *props, LPTSTR output,
                       int64t *fileSize, DWORD expectedCRC) {
  if (isOK(props)) {
    DWORD *status = &props->status;
    HANDLE hFileRead = props->handler;
    int64t *size = fileSize;
    DWORD counter = 0;
    DWORD crc32 = -1L;
    HANDLE hFileWrite = CreateFile(output, GENERIC_READ | GENERIC_WRITE, 0, 0,
                                   CREATE_ALWAYS, 0, hFileRead);

    if (hFileWrite == INVALID_HANDLE_VALUE) {
      LPTSTR err;
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 0,
                   TEXT("[ERROR] Can`t create file "), 0);
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, output, 1);

      err = getErrorDescription(GetLastError());
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("Error description : "),
                   0);
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, err, 1);

      showError(props, OUTPUT_ERROR_PROP, 2, output, err);
      FREE(err);
      *status = ERROR_INPUTOUPUT;
      return;
    }
    if (props->restOfBytes->length != 0 && props->restOfBytes->bytes != NULL) {
      // check if the data stored in restBytes is more than we need
      // rest bytes contains much less than int64t so we operate here only with
      // low bits of size
      DWORD restBytesToWrite = (compare(size, props->restOfBytes->length) > 0)
                                   ? props->restOfBytes->length
                                   : size->Low;
      DWORD usedBytes = restBytesToWrite;

      BYTE *ptr = props->restOfBytes->bytes;

      DWORD write = 0;
      while (restBytesToWrite > 0) {
        WriteFile(hFileWrite, ptr, restBytesToWrite, &write, 0);
        update_crc32(&crc32, ptr, write);
        restBytesToWrite -= write;
        ptr += write;
      }
      modifyRestBytes(props->restOfBytes, usedBytes);
      minus(size, usedBytes);
    }

    if (compare(size, 0) > 0) {
      DWORD bufferSize = props->bufsize;
      BYTE *buf = (BYTE *)newpChar(bufferSize);
      DWORD bufsize = (compare(size, bufferSize) > 0) ? bufferSize : size->Low;
      DWORD read = 0;
      //  printf("Using buffer size: %u/%u\n", bufsize, bufferSize);
      while (ReadFile(hFileRead, buf, bufsize, &read, 0) && read &&
             compare(size, 0) > 0) {
        addProgressPosition(props, read);
        WriteFile(hFileWrite, buf, read, &read, 0);
        update_crc32(&crc32, buf, read);

        minus(size, read);

        if ((compare(size, bufsize) < 0) && (compare(size, 0) > 0)) {
          bufsize = size->Low;
        }
        ZERO(buf, sizeof(char) * bufferSize);
        if (compare(size, 0) == 0) {
          break;
        }
        if ((counter++) % 20 == 0) {
          if (isTerminated(props))
            break;
        }
      }
      if ((compare(size, 0) > 0 || read == 0) && !isTerminated(props)) {
        // we could not read requested size
        *status = ERROR_INTEGRITY;
        writeMessage(props, OUTPUT_LEVEL_DEBUG, 1,
                     TEXT("Can`t read data from file : not enough data"), 1);
      }
      FREE(buf);
    }
    CloseHandle(hFileWrite);
    crc32 = ~crc32;
    if (isOK(props) && crc32 != expectedCRC) {
      writeDWORD(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("expected CRC : "),
                 expectedCRC, 1);
      writeDWORD(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("real     CRC : "), crc32,
                 1);
      *status = ERROR_INTEGRITY;
    }
  }
}

// returns : ERROR_OK, ERROR_INTEGRITY, ERROR_FREE_SPACE
void extractFileToDir(LauncherProperties *props, LPTSTR *resultFile) {
  LPTSTR fileName = NULL;
  int64t *fileLength = NULL;
  DWORD crc = 0;
  writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("Extracting file ..."), 1);
  readStringWithDebug(props, &fileName, TEXT("file name"), TRUE);

  fileLength = newint64_t(0, 0);
  readBigNumberWithDebug(props, fileLength, TEXT("file length "));

  readNumberWithDebug(props, &crc, TEXT("CRC32"));

  if (!isOK(props))
    return;

  if (fileName != NULL) {
    DWORD i = 0;
    LPTSTR dir;
    resolveString(props, &fileName);

    for (i = 0; i < getLength(fileName); i++) {
      if (fileName[i] == TEXT('/')) {
        fileName[i] = TEXT('\\');
      }
    }

    dir = getParentDirectory(fileName);
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 0,
                 TEXT("   ... extract to directory = "), 0);
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, dir, 1);

    checkFreeSpace(props, dir, fileLength);
    FREE(dir);
    if (isOK(props)) {
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 0,
                   TEXT("   ... starting data extraction"), 1);
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("   ... output file is "),
                   0);
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, fileName, 1);
      extractDataToFile(props, fileName, fileLength, crc);
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 0,
                   TEXT("   ... extraction finished"), 1);
      *resultFile = fileName;
    } else {
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 0,
                   TEXT("   ... data extraction canceled"), 1);
    }
  } else {
    writeMessage(
        props, OUTPUT_LEVEL_DEBUG, 0,
        TEXT("Error! File name can`t be null. Seems to be integrity error!"),
        1);
    *resultFile = NULL;
    props->status = ERROR_INTEGRITY;
  }
  FREE(fileLength);
  return;
}

void loadI18NStrings(LauncherProperties *props) {
  DWORD i = 0;
  DWORD j = 0;
  // read number of locales

  DWORD numberOfLocales = 0;
  DWORD numberOfProperties = 0;

  readNumberWithDebug(props, &numberOfLocales, TEXT("number of locales"));
  if (!isOK(props))
    return;
  if (numberOfLocales == 0) {
    props->status = ERROR_INTEGRITY;
    return;
  }

  readNumberWithDebug(props, &numberOfProperties, TEXT("i18n properties"));
  if (!isOK(props))
    return;
  if (numberOfProperties == 0) {
    props->status = ERROR_INTEGRITY;
    return;
  }

  props->i18nMessages =
      (I18NStrings *)LocalAlloc(LPTR, sizeof(I18NStrings) * numberOfProperties);

  props->I18N_PROPERTIES_NUMBER = numberOfProperties;
  props->i18nMessages->properties = newppTCHAR(props->I18N_PROPERTIES_NUMBER);
  props->i18nMessages->strings = newppTCHAR(props->I18N_PROPERTIES_NUMBER);

  for (i = 0; isOK(props) && i < numberOfProperties; i++) {
    // read property name as ASCII
    LPTSTR propName = NULL;
    LPTSTR number = DWORDtoTCHARN(i, 2);
    props->i18nMessages->properties[i] = NULL;
    props->i18nMessages->strings[i] = NULL;
    propName = appendString(NULL, TEXT("property name "));

    propName = appendString(propName, number);
    FREE(number);
    readStringWithDebug(props, &(props->i18nMessages->properties[i]), propName,
                        FALSE);
    FREE(propName);
  }
  if (isOK(props)) {

    BOOL isLocaleMatches;
    LPTSTR localeName;
    LPTSTR currentLocale = getLocaleName();

    writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("Current System Locale : "),
                 0);
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, currentLocale, 1);

    if (props->userDefinedLocale !=
        NULL) { // using user-defined locale via command-line parameter
      writeMessage(props, OUTPUT_LEVEL_NORMAL, 0,
                   TEXT("[CMD Argument] Try to use locale "), 0);
      writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, props->userDefinedLocale, 1);
      FREE(currentLocale);
      currentLocale = appendString(NULL, props->userDefinedLocale);
    }

    for (j = 0; j < numberOfLocales; j++) { //  for all locales in file...
      // read locale name as UNICODE ..
      // it should be like en_US or smth like that
      localeName = NULL;
      readStringWithDebug(props, &localeName, TEXT("locale name"), TRUE);
      if (!isOK(props))
        break;

      isLocaleMatches =
          (localeName == NULL) || (search(currentLocale, localeName) != NULL);

      // read properties names and value
      for (i = 0; i < numberOfProperties; i++) {
        // read property value as UNICODE

        LPTSTR value = NULL;
        LPTSTR s1 = DWORDtoTCHAR(i + 1);
        LPTSTR s2 = DWORDtoTCHAR(numberOfProperties);
        LPTSTR s3 = appendString(NULL, TEXT("value "));
        s3 = appendString(s3, s1);
        s3 = appendString(s3, TEXT("/"));
        s3 = appendString(s3, s2);
        FREE(s1);
        FREE(s2);
        readStringWithDebug(props, &value, s3, TRUE);

        FREE(s3);
        if (!isOK(props))
          break;
        if (isLocaleMatches) {
          // it is a know property
          FREE(props->i18nMessages->strings[i]);
          props->i18nMessages->strings[i] = appendString(NULL, value);
        }
        FREE(value);
      }
      FREE(localeName);
    }
    FREE(currentLocale);
  }
}

LauncherResource *newLauncherResource() {
  LauncherResource *file =
      (LauncherResource *)LocalAlloc(LPTR, sizeof(LauncherResource));
  file->path = NULL;
  file->resolved = NULL;
  file->type = 0;
  return file;
}
TCHARList *newTCHARList(DWORD number) {
  TCHARList *list = (TCHARList *)LocalAlloc(LPTR, sizeof(TCHARList));
  list->size = number;
  if (number > 0) {
    DWORD i = 0;
    list->items = newppTCHAR(number);
    for (i = 0; i < number; i++) {
      list->items[i] = NULL;
    }
  } else {
    list->items = NULL;
  }
  return list;
}

void freeTCHARList(TCHARList **plist) {
  TCHARList *list;
  list = *plist;
  if (list != NULL) {
    DWORD i = 0;
    if (list->items != NULL) {
      for (i = 0; i < list->size; i++) {
        FREE(list->items[i]);
      }
      FREE(list->items);
    }
    FREE(*plist);
  }
}

LauncherResourceList *newLauncherResourceList(DWORD number) {
  LauncherResourceList *list =
      (LauncherResourceList *)LocalAlloc(LPTR, sizeof(LauncherResourceList));
  list->size = number;
  if (number > 0) {
    DWORD i = 0;

    list->items = (LauncherResource **)LocalAlloc(
        LPTR, sizeof(LauncherResource *) * number);
    for (i = 0; i < number; i++) {
      list->items[i] = NULL;
    }
  } else {
    list->items = NULL;
  }
  return list;
}

void freeLauncherResource(LauncherResource **file) {
  if (*file != NULL) {
    FREE((*file)->path);
    FREE((*file)->resolved);
    FREE(*file);
  }
}

void extractLauncherResource(LauncherProperties *props, LauncherResource **file,
                             LPCTSTR name) {
  LPTSTR typeStr = appendString(appendString(NULL, name), TEXT(" type"));
  *file = newLauncherResource();

  readNumberWithDebug(props, &((*file)->type), typeStr);

  if (isOK(props)) {
    FREE(typeStr);

    if ((*file)->type == 0) { // bundled
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 1, TEXT("... file is bundled"),
                   1);
      extractFileToDir(props, &((*file)->path));
      if (!isOK(props)) {
        writeMessage(props, OUTPUT_LEVEL_DEBUG, 1,
                     TEXT("Error extracting file!"), 1);
        return;
      } else {
        (*file)->resolved = appendString(NULL, (*file)->path);
        writeMessage(props, OUTPUT_LEVEL_DEBUG, 0,
                     TEXT("file was succesfully extracted to "), 0);
        writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, (*file)->path, 1);
      }
    } else {
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 1, TEXT("... file is external"),
                   1);
      readStringWithDebug(props, &((*file)->path), name, FALSE);
      if (!isOK(props)) {
        writeMessage(props, OUTPUT_LEVEL_DEBUG, 1, TEXT("Error reading "), 1);
        writeMessage(props, OUTPUT_LEVEL_DEBUG, 1, name, 1);
      }
    }
  } else {
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 1, TEXT("Error reading "), 0);
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 1, typeStr, 0);
    FREE(typeStr);
  }
}

void readTCHARList(LauncherProperties *props, TCHARList **list, LPCTSTR name) {
  DWORD number = 0;
  DWORD i = 0;
  LPTSTR numberStr = appendString(appendString(NULL, TEXT("number of ")), name);

  *list = NULL;
  readNumberWithDebug(props, &number, numberStr);
  FREE(numberStr);

  if (!isOK(props))
    return;

  *list = newTCHARList(number);
  for (i = 0; i < (*list)->size; i++) {
    LPTSTR nextStr =
        appendString(appendString(NULL, TEXT("next item in ")), name);
    readStringWithDebug(props, &((*list)->items[i]), nextStr, TRUE);
    FREE(nextStr);
    if (!isOK(props))
      return;
  }
}
void readLauncherResourceList(LauncherProperties *props,
                              LauncherResourceList **list, LPCTSTR name) {
  DWORD num = 0;
  DWORD i = 0;
  LPTSTR numberStr = appendString(appendString(NULL, TEXT("number of ")), name);
  readNumberWithDebug(props, &num, numberStr);
  FREE(numberStr);
  if (!isOK(props))
    return;

  *list = newLauncherResourceList(num);
  for (i = 0; i < (*list)->size; i++) {
    extractLauncherResource(props, &((*list)->items[i]),
                            TEXT("launcher resource"));
    if (!isOK(props)) {
      LPTSTR str =
          appendString(appendString(NULL, TEXT("Error processing ")), name);
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 1, str, 1);
      FREE(str);
      break;
    }
  }
}

void readLauncherProperties(LauncherProperties *props) {
  DWORD i = 0;
  LPTSTR str = NULL;

  readTCHARList(props, &(props->jvmArguments), TEXT("jvm arguments"));
  if (!isOK(props))
    return;

  readTCHARList(props, &(props->appArguments), TEXT("app arguments"));
  if (!isOK(props))
    return;

  readStringWithDebug(props, &(props->mainClass), TEXT("Main Class"), TRUE);
  if (!isOK(props))
    return;

  readStringWithDebug(props, &(props->testJVMClass), TEXT("TestJVM Class"),
                      TRUE);
  if (!isOK(props))
    return;

  readNumberWithDebug(props, &(props->compatibleJavaNumber),
                      TEXT("compatible java"));
  if (!isOK(props))
    return;

  if (props->compatibleJavaNumber > 0) {
    props->compatibleJava = (JavaCompatible **)LocalAlloc(
        LPTR, sizeof(JavaCompatible *) * props->compatibleJavaNumber);
    for (i = 0; i < props->compatibleJavaNumber; i++) {

      props->compatibleJava[i] = newJavaCompatible();

      readStringWithDebug(props, &str, TEXT("min java version"), FALSE);
      if (!isOK(props))
        return;
      props->compatibleJava[i]->minVersion =
          getJavaVersionFromString(str, &props->status);
      FREE(str);
      if (!isOK(props))
        return;

      str = NULL;
      readStringWithDebug(props, &str, TEXT("max java version"), FALSE);
      if (!isOK(props))
        return;
      props->compatibleJava[i]->maxVersion =
          getJavaVersionFromString(str, &props->status);
      FREE(str);
      if (!isOK(props))
        return;

      readStringWithDebug(props, &(props->compatibleJava[i]->vendor),
                          TEXT("vendor"), FALSE);
      if (!isOK(props))
        return;

      readStringWithDebug(props, &(props->compatibleJava[i]->osName),
                          TEXT("os name"), FALSE);
      if (!isOK(props))
        return;

      readStringWithDebug(props, &(props->compatibleJava[i]->osArch),
                          TEXT("os arch"), FALSE);
      if (!isOK(props))
        return;
    }
  }
  readNumberWithDebug(props, &props->bundledNumber, TEXT("bundled files"));
  readBigNumberWithDebug(props, props->bundledSize, TEXT("bundled size"));
}

void extractJVMData(LauncherProperties *props) {
  if (isOK(props)) {

    writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, TEXT("Extracting JVM data... "),
                 1);
    extractLauncherResource(props, &(props->testJVMFile), TEXT("testJVM file"));
    if (!isOK(props)) {
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 1,
                   TEXT("Error extracting testJVM file!"), 1);
      return;
    }

    readLauncherResourceList(props, &(props->jvms), TEXT("JVMs"));
  }
}

void extractData(LauncherProperties *props) {
  if (isOK(props)) {
    writeMessage(props, OUTPUT_LEVEL_NORMAL, 0,
                 TEXT("Extracting Bundled data... "), 1);
    readLauncherResourceList(props, &(props->jars),
                             TEXT("bundled and external files"));
    if (isOK(props)) {
      readLauncherResourceList(props, &(props->other), TEXT("other data"));
    }
  }
}
