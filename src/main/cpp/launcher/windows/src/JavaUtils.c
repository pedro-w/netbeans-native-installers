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

#ifdef UNICODE
// because we use some of the CRT functions.
#define _UNICODE
#endif

#include <windows.h>

#include "FileUtils.h"
#include "JavaUtils.h"
#include "Launcher.h"
#include "Main.h"
#include "ProcessUtils.h"
#include "RegistryUtils.h"
#include "StringUtils.h"
#include "SystemUtils.h"
#include <stdio.h>
#include <tchar.h>
#include <wchar.h>

const DWORD JAVA_VERIFICATION_PROCESS_TIMEOUT = 10000; // 10sec
const DWORD UNPACK200_EXTRACTION_TIMEOUT = 60000; // 60 seconds on each file
const DWORD JAVA_VERIFICATION_PROCESS_PRIORITY = NORMAL_PRIORITY_CLASS;
const LPCTSTR JAVA_EXE_SUFFIX = TEXT("\\bin\\java.exe");
const LPCTSTR UNPACK200_EXE_SUFFIX = TEXT("\\bin\\unpack200.exe");
const LPCTSTR JAVA_LIB_SUFFIX = TEXT("\\lib");
const LPCTSTR PACK_GZ_SUFFIX = TEXT(".pack.gz");
const LPCTSTR JAR_PACK_GZ_SUFFIX = TEXT(".jar.pack.gz");
const LPCTSTR PATH_ENV = TEXT("PATH");

const DWORD JVM_EXTRACTION_TIMEOUT = 180000; // 180sec

LPCTSTR JAVA_REGISTRY_KEYS[] = {
    TEXT("SOFTWARE\\JavaSoft\\Java Runtime Environment"),
    TEXT("SOFTWARE\\JavaSoft\\Java Development Kit"),
    TEXT("SOFTWARE\\JavaSoft\\JDK"),
    TEXT("SOFTWARE\\JRockit\\Java Runtime Environment"),
    TEXT("SOFTWARE\\JRockit\\Java Development Kit"),
    TEXT("SOFTWARE\\IBM\\Java Runtime Environment"),
    TEXT("SOFTWARE\\IBM\\Java2 Runtime Environment"),
    TEXT("SOFTWARE\\IBM\\Java Development Kit"),
};
LPCTSTR JAVA_HOME = TEXT("JavaHome");
LPTSTR CURRENT_VERSION = TEXT("CurrentVersion");

LPTSTR getJavaHomeValue(LPCTSTR parentkey, LPCTSTR subkey, BOOL access64key) {
  return getStringValuePC(HKEY_LOCAL_MACHINE, parentkey, subkey, JAVA_HOME,
                          access64key);
}

LPTSTR getTestJVMFileName(LPTSTR testJVMFile) {
  LPTSTR filePtr = testJVMFile;
  LPTSTR testJavaClass = NULL;

  if (filePtr != NULL) {
    LPTSTR dotClass = NULL;
    while (search(filePtr, TEXT("\\")) != NULL) {
      filePtr = search(filePtr, TEXT("\\"));
      filePtr++;
    }
    dotClass = search(filePtr, TEXT(".class"));

    if (dotClass != NULL) {
      testJavaClass = appendStringN(NULL, 0, filePtr,
                                    getLength(filePtr) - getLength(dotClass));
    }
  }
  return testJavaClass;
}

// returns 0 if equals, 1 if first > second, -1 if first < second
char compareJavaVersion(JavaVersion *first, JavaVersion *second) {
  if (first == NULL)
    return (second == NULL) ? 0 : -1;
  if (second == NULL)
    return -1;
  if (first->major == second->major) {
    if (first->minor == second->minor) {
      if (first->micro == second->micro) {
        if (first->update == second->update)
          return 0;
        return (first->update > second->update) ? 1 : -1;
      }
      return (first->micro > second->micro) ? 1 : -1;
    }
    return (first->minor > second->minor) ? 1 : -1;
  } else {
    return (first->major > second->major) ? 1 : -1;
  }
}
DWORD isJavaCompatible(JavaProperties *currentJava,
                       JavaCompatible **compatibleJava, DWORD number) {
  JavaVersion *current = currentJava->version;
  DWORD i = 0;

  for (i = 0; i < number; i++) {
    DWORD check = 1;

    check = (compareJavaVersion(current, compatibleJava[i]->minVersion) >= 0 &&
             compareJavaVersion(current, compatibleJava[i]->maxVersion) <= 0)
                ? check
                : 0;

    if (check) {
      if (compatibleJava[i]->vendor != NULL) {
        check = (search(currentJava->vendor, compatibleJava[i]->vendor) != NULL)
                    ? check
                    : 0;
      }
      if (compatibleJava[i]->osName != NULL) {
        check = (search(currentJava->osName, compatibleJava[i]->osName) != NULL)
                    ? check
                    : 0;
      }

      if (compatibleJava[i]->osArch != NULL) {
        check = (search(currentJava->osArch, compatibleJava[i]->osArch) != NULL)
                    ? check
                    : 0;
      }
      if (check) {
        return 1;
      }
    }
  }
  return 0;
}

JavaVersion *getJavaVersionFromString(LPCTSTR string, DWORD *result) {
  JavaVersion *vers = NULL;
  if (getLength(string) < 3) {
    return vers;
  }

  LPCTSTR p = string;

  // get major
  long major = 0;
  while (*p != 0) {
    TCHAR c = *p++;
    if (c >= TEXT('0') && c <= TEXT('9')) {
      major = (major)*10 + c - TEXT('0');
      if (major > 999)
        return vers;
      continue;
    } else if (c == TEXT('.') || c == TEXT('+')) {
      break;
    } else {
      return vers;
    }
  }

  // get minor
  long minor = 0;
  while (*p != 0) {
    TCHAR c = *p;
    if (c >= TEXT('0') && c <= TEXT('9')) {
      minor = (minor)*10 + c - TEXT('0');
      p++;
      continue;
    }
    break;
  }

  *result = ERROR_OK;
  vers = (JavaVersion *)LocalAlloc(LPTR, sizeof(JavaVersion));
  vers->major = major;
  vers->minor = minor;
  vers->micro = 0;
  vers->update = 0;
  ZERO(vers->build, 128);

  if (p[0] == TEXT('.')) { // micro...
    p++;
    while (*p != 0) {
      TCHAR c = *p;
      if (c >= TEXT('0') && c <= TEXT('9')) {
        vers->micro = (vers->micro) * 10 + c - TEXT('0');
        p++;
        continue;
      } else if (c == '_') { // update
        p++;
        while ((c = *p) != 0) {
          p++;
          if (c >= TEXT('0') && c <= TEXT('9')) {
            vers->update = (vers->update) * 10 + c - TEXT('0');
            continue;
          } else {
            break;
          }
        }
      } else {
        if (*p != 0)
          p++;
      }
      if (c == TEXT('-') && *p != 0) { // build number
        lstrcpyn(vers->build, p, min(127, getLength(p) + 1));
      }
      break;
    }
  }
  return vers;
}

DWORD getJavaPropertiesFromOutput(LauncherProperties *props, LPTSTR str,
                                  JavaProperties **javaProps) {
  DWORD separators = getLineSeparatorNumber(str);
  DWORD result = ERROR_INPUTOUPUT;
  *javaProps = NULL;
  if (separators == TEST_JAVA_PARAMETERS) {
    LPTSTR start;
    LPTSTR end;
    LPTSTR javaVersion;
    LPTSTR javaVmVersion;
    LPTSTR javaVendor;
    LPTSTR osName;
    LPTSTR osArch;
    LPTSTR string;
    JavaVersion *vers;

    start = str;
    end = search(start, TEXT("\n"));

    javaVersion =
        appendStringN(NULL, 0, start, getLength(start) - getLength(end) - 1);
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("    java.version =  "), 0);
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, javaVersion, 1);
    start = end + 1;
    end = search(start, TEXT("\n"));

    javaVmVersion =
        appendStringN(NULL, 0, start, getLength(start) - getLength(end) - 1);
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("    java.vm.version = "),
                 0);
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, javaVmVersion, 1);
    start = end + 1;
    end = search(start, TEXT("\n"));

    javaVendor =
        appendStringN(NULL, 0, start, getLength(start) - getLength(end) - 1);
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("    java.vendor = "), 0);
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, javaVendor, 1);
    start = end + 1;
    end = search(start, TEXT("\n"));

    osName =
        appendStringN(NULL, 0, start, getLength(start) - getLength(end) - 1);
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("    os.name = "), 0);
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, osName, 1);
    start = end + 1;
    end = search(start, TEXT("\n"));

    osArch =
        appendStringN(NULL, 0, start, getLength(start) - getLength(end) - 1);
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("    os.arch = "), 0);
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, osArch, 2);

    string = javaVersion;

    if (javaVmVersion != NULL) {
      string = search(javaVmVersion, javaVersion);
      if (string == NULL) {
        string = javaVersion;
      }
    }
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 0,
                 TEXT("... getting java version from string : "), 0);
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, string, 1);

    vers = getJavaVersionFromString(string, &result);
    if (javaProps != NULL) {
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("... some java there"),
                   1);
      *javaProps = (JavaProperties *)LocalAlloc(LPTR, sizeof(JavaProperties));
      (*javaProps)->version = vers;
      (*javaProps)->vendor = javaVendor;
      (*javaProps)->osName = osName;
      (*javaProps)->osArch = osArch;
      (*javaProps)->javaHome = NULL;
      (*javaProps)->javaExe = NULL;
    } else {
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("... no java  there"), 1);
      FREE(javaVendor);
      FREE(osName);
      FREE(osArch);
    }
    FREE(javaVmVersion);
    FREE(javaVersion);
  }
  return result;
}

void getJavaProperties(LPCTSTR location, LauncherProperties *props,
                       JavaProperties **javaProps) {
  LPTSTR testJavaClass = props->testJVMClass;
  LPTSTR javaExecutable = getJavaResource(location, JAVA_EXE_SUFFIX);
  LPTSTR libDirectory = getJavaResource(location, JAVA_LIB_SUFFIX);

  if (fileExists(javaExecutable) && testJavaClass != NULL &&
      isDirectory(libDirectory)) {
    LPTSTR command = NULL;
    HANDLE hRead;
    HANDLE hWrite;

    writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("... java hierarchy there"),
                 1);
    // <location>\bin\java.exe exists

    appendCommandLineArgument(&command, javaExecutable);
    appendCommandLineArgument(&command, TEXT("-classpath"));
    appendCommandLineArgument(&command, props->testJVMFile->resolved);
    appendCommandLineArgument(&command, testJavaClass);

    CreatePipe(&hRead, &hWrite, NULL, 0);
    // Start the child process.
    executeCommand(props, command, NULL, JAVA_VERIFICATION_PROCESS_TIMEOUT,
                   hWrite, INVALID_HANDLE_VALUE,
                   JAVA_VERIFICATION_PROCESS_PRIORITY);
    if (props->status != ERROR_ON_EXECUTE_PROCESS &&
        props->status != ERROR_PROCESS_TIMEOUT) {
      LPTSTR output = readHandle(hRead);
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("           output :\n"),
                   0);
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, output, 1);

      props->status = getJavaPropertiesFromOutput(props, output, javaProps);
      if (props->status == ERROR_OK) {
        (*javaProps)->javaHome = appendString(NULL, location);
        (*javaProps)->javaExe = appendString(NULL, javaExecutable);
      }
      FREE(output);
    } else if (props->status == ERROR_PROCESS_TIMEOUT) {
      // java verification process finished by time out
      props->status = ERROR_INPUTOUPUT;
    }
    FREE(command);
    CloseHandle(hWrite);
    CloseHandle(hRead);
  } else {
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("... not a java hierarchy"),
                 1);
    props->status = ERROR_INPUTOUPUT;
  }
  FREE(libDirectory);
  FREE(javaExecutable);
}

LPTSTR getJavaVersionFormatted(const JavaProperties *javaProps) {
  LPTSTR result = NULL;
  if (javaProps != NULL) {
    JavaVersion *version = javaProps->version;
    if (version != NULL) {
      TCHAR tmp[128];
      wsprintf(tmp, TEXT("%ld.%ld.%ld"), version->major, version->minor,
               version->micro);
      result = appendString(NULL, tmp);
      if (version->update != 0) {
        wsprintf(tmp, TEXT("_%02ld"), version->update);
        result = appendString(result, tmp);
      }
      if (getLength(version->build) > 0) {
        result = appendString(result, TEXT("-"));
        result = appendString(result, version->build);
      }
    }
  }
  return result;
}

JavaCompatible *newJavaCompatible() {
  JavaCompatible *props =
      (JavaCompatible *)LocalAlloc(LPTR, sizeof(JavaCompatible));
  props->minVersion = NULL;
  props->maxVersion = NULL;
  props->vendor = NULL;
  props->osName = NULL;
  return props;
}

void freeJavaProperties(JavaProperties **props) {
  if (*props != NULL) {
    FREE((*props)->version);
    FREE((*props)->javaHome);
    FREE((*props)->javaExe);
    FREE((*props)->vendor);
    FREE(*props);
  }
}

LPTSTR getJavaResource(LPCTSTR location, LPCTSTR suffix) {
  return appendString(appendString(NULL, location), suffix);
}

void searchCurrentJavaRegistry(LauncherProperties *props, BOOL access64key) {
  DWORD i = 0;
  LPCTSTR *keys = JAVA_REGISTRY_KEYS;
  DWORD k = 0;
  LPTSTR buffer = newpTCHAR(MAX_LEN_VALUE_NAME);
  HKEY rootKeys[2] = {HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER};
  DWORD rootKeysNumber = sizeof(rootKeys) / sizeof(HKEY);
  DWORD keysNumber = sizeof(JAVA_REGISTRY_KEYS) / sizeof(JAVA_REGISTRY_KEYS[0]);

  writeMessage(props, OUTPUT_LEVEL_NORMAL, 0,
               TEXT("Search java in CurrentVersion values..."), 1);

  for (k = 0; k < rootKeysNumber; k++) {
    for (i = 0; i < keysNumber; i++) {
      if (isTerminated(props)) {
        return;
      } else {

        LPTSTR value =
            getStringValue(rootKeys[k], keys[i], CURRENT_VERSION, access64key);
        if (value != NULL) {
          LPTSTR javaHome = getStringValuePC(rootKeys[k], keys[i], value,
                                             JAVA_HOME, access64key);
          writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, TEXT("... "), 0);
          writeMessage(props, OUTPUT_LEVEL_NORMAL, 0,
                       (rootKeys[k] == HKEY_LOCAL_MACHINE)
                           ? TEXT("HKEY_LOCAL_MACHINE")
                           : TEXT("HKEY_CURRENT_USER"),
                       0);
          writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, TEXT("\\"), 0);
          writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, keys[i], 0);
          writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, TEXT("\\"), 0);
          writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, CURRENT_VERSION, 0);
          writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, TEXT("->"), 0);
          writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, value, 0);
          writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, TEXT("["), 0);
          writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, JAVA_HOME, 0);
          writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, TEXT("] = "), 0);
          writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, javaHome, 1);

          FREE(value);
          trySetCompatibleJava(javaHome, props);
          FREE(javaHome);
          if (props->java != NULL) {
            FREE(buffer);
            return;
          }
        }
      }
    }
  }

  // we found no CurrentVersion java... just search for other possible keys
  writeMessage(props, OUTPUT_LEVEL_NORMAL, 0,
               TEXT("Search java in other values..."), 1);

  for (k = 0; k < rootKeysNumber; k++) {
    for (i = 0; i < keysNumber; i++) {
      HKEY hkey = 0;
      DWORD index = 0;
      if (RegOpenKeyEx(rootKeys[k], keys[i], 0,
                       KEY_READ |
                           ((access64key && IsWow64) ? KEY_WOW64_64KEY : 0),
                       &hkey) == ERROR_SUCCESS) {
        DWORD number = 0;
        if (RegQueryInfoKey(hkey, NULL, NULL, NULL, &number, NULL, NULL, NULL,
                            NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
          DWORD err = 0;
          do {

            DWORD size = MAX_LEN_VALUE_NAME;
            buffer[0] = 0;
            err = RegEnumKeyEx(hkey, index, buffer, &size, NULL, NULL, NULL,
                               NULL);
            if (err == ERROR_SUCCESS) {
              LPTSTR javaHome = getJavaHomeValue(keys[i], buffer, access64key);

              writeMessage(props, OUTPUT_LEVEL_NORMAL, 0,
                           (rootKeys[k] == HKEY_LOCAL_MACHINE)
                               ? TEXT("HKEY_LOCAL_MACHINE")
                               : TEXT("HKEY_CURRENT_USER"),
                           0);
              writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, TEXT("\\"), 0);
              writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, keys[i], 0);
              writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, TEXT("\\"), 0);
              writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, buffer, 0);
              writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, TEXT("["), 0);
              writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, JAVA_HOME, 0);
              writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, TEXT("] = "), 0);
              writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, javaHome, 1);

              trySetCompatibleJava(javaHome, props);
              FREE(javaHome);
              if (props->java != NULL) {
                i = keysNumber; // to the end of cycles
                k = rootKeysNumber;
                break;
              }
            }
            index++;
          } while (err == ERROR_SUCCESS);
        }
      }
      if (hkey != 0) {
        RegCloseKey(hkey);
      }
    }
  }
  FREE(buffer);
  return;
}

void searchJavaFromEnvVariables(LauncherProperties *props) {
  static LPCTSTR ENVS[] = {
      TEXT("JAVA_HOME"), TEXT("JAVAHOME"), TEXT("JAVA_PATH"), TEXT("JDK_HOME"),
      TEXT("JDKHOME"),   TEXT("ANT_JAVA"), TEXT("JAVA"),      TEXT("JDK")};

  TCHAR buffer[MAX_PATH];

  int size = sizeof(ENVS) / sizeof(ENVS[0]);
  int i = 0;
  int ret;

  for (i = 0; i < size; i++) {
    if (isTerminated(props))
      return;
    buffer[0] = '\0';
    ret = GetEnvironmentVariable(ENVS[i], buffer, MAX_PATH);
    if (ret > 0 && ret <= MAX_PATH) {
      writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, TEXT("    <"), 0);
      writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, ENVS[i], 0);
      writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, TEXT("> = "), 0);
      writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, buffer, 1);
      trySetCompatibleJava(buffer, props);
      if (props->java != NULL) {
        break;
      }
    }
  }
}

void unpackJars(LauncherProperties *props, LPCTSTR jvmDir, LPCTSTR startDir,
                LPCTSTR unpack200exe) {
  DWORD attrs;
  DWORD dwError;

  if (!isOK(props))
    return;
  attrs = GetFileAttributes(startDir);
  if (attrs == INVALID_FILE_ATTRIBUTES) {
    writeError(props, OUTPUT_LEVEL_DEBUG, 1,
               TEXT("Error! Can`t get attributes of the file : "), startDir,
               GetLastError());
    return;
  }
  if (attrs & FILE_ATTRIBUTE_DIRECTORY) { // is directory
    WIN32_FIND_DATA FindFileData;
    HANDLE hFind = INVALID_HANDLE_VALUE;

    LPTSTR DirSpec = appendString(appendString(NULL, startDir), TEXT("\\*"));

    // Find the first file in the directory.
    hFind = FindFirstFile(DirSpec, &FindFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
      writeError(props, OUTPUT_LEVEL_DEBUG, 1,
                 TEXT("Error! Can`t file with pattern "), DirSpec,
                 GetLastError());
    } else {
      // List all the other files in the directory.
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("... listing directory "),
                   0);
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, startDir, 1);

      while (FindNextFile(hFind, &FindFileData) != 0 && isOK(props)) {
        if (lstrcmp(FindFileData.cFileName, TEXT(".")) != 0 &&
            lstrcmp(FindFileData.cFileName, TEXT("..")) != 0) {
          LPTSTR child =
              appendString(appendString(appendString(NULL, startDir), FILE_SEP),
                           FindFileData.cFileName);
          if (isDirectory(child)) {
            writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("... directory : "),
                         0);
            writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, child, 1);
            unpackJars(props, jvmDir, child, unpack200exe);
          } else if (search(FindFileData.cFileName, JAR_PACK_GZ_SUFFIX) !=
                     NULL) {
            LPTSTR jarName = appendString(
                appendString(appendString(NULL, startDir), FILE_SEP),
                appendStringN(NULL, 0, FindFileData.cFileName,
                              getLength(FindFileData.cFileName) -
                                  getLength(PACK_GZ_SUFFIX)));
            LPTSTR unpackCommand = NULL;
            writeMessage(props, OUTPUT_LEVEL_DEBUG, 0,
                         TEXT("... packed jar : "), 0);
            writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, child, 1);
            writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("... jar name : "),
                         0);
            writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, jarName, 1);

            appendCommandLineArgument(&unpackCommand, unpack200exe);
            appendCommandLineArgument(&unpackCommand,
                                      TEXT("-r")); // remove input file
            appendCommandLineArgument(&unpackCommand, child);
            appendCommandLineArgument(&unpackCommand, jarName);

            executeCommand(props, unpackCommand, NULL,
                           UNPACK200_EXTRACTION_TIMEOUT, props->stdoutHandle,
                           props->stderrHandle, NORMAL_PRIORITY_CLASS);
            FREE(unpackCommand);
            if (!isOK(props)) {
              if (props->status == ERROR_PROCESS_TIMEOUT) {
                writeMessage(props, OUTPUT_LEVEL_DEBUG, 1,
                             TEXT("... could not unpack file : timeout"), 1);
              } else {
                writeMessage(props, OUTPUT_LEVEL_DEBUG, 1,
                             TEXT("... an error occured unpacking the file"),
                             1);
              }
              props->exitCode = props->status;
            }
            FREE(jarName);
          }
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
    FREE(DirSpec);
  }
}
void installJVM(LauncherProperties *props, LauncherResource *jvm) {
  LPTSTR command = NULL;
  LPTSTR jvmDir = getParentDirectory(jvm->resolved);

  jvmDir = appendString(jvmDir, TEXT("\\_jvm"));
  createDirectory(props, jvmDir);
  if (!isOK(props)) {
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 1,
                 TEXT("... cannot create dir for JVM extraction :"), 0);
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 1, jvmDir, 1);
    FREE(jvmDir);
    return;
  }

  appendCommandLineArgument(&command, jvm->resolved);
  appendCommandLineArgument(&command, TEXT("-d"));
  appendCommandLineArgument(&command, jvmDir);

  executeCommand(props, command, jvmDir, JVM_EXTRACTION_TIMEOUT,
                 props->stdoutHandle, props->stderrHandle,
                 NORMAL_PRIORITY_CLASS);
  FREE(command);
  if (!isOK(props)) {
    if (props->status == ERROR_PROCESS_TIMEOUT) {
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 1,
                   TEXT("... could not extract JVM : timeout"), 1);
    } else {
      writeMessage(
          props, OUTPUT_LEVEL_DEBUG, 1,
          TEXT("... an error occured during running JVM extraction file"), 1);
    }
    props->exitCode = props->status;
  } else {
    LPTSTR unpack200exe =
        appendString(appendString(NULL, jvmDir), UNPACK200_EXE_SUFFIX);
    if (fileExists(unpack200exe)) {
      unpackJars(props, jvmDir, jvmDir, unpack200exe);
    } else {
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 1,
                   TEXT("... no unpack200 command"), 1);
      props->status = ERROR_BUNDLED_JVM_EXTRACTION;
    }
    if (!isOK(props)) {
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 1,
                   TEXT("Could not unpack200 the JVM jars"), 1);
    }
    FREE(unpack200exe);
  }
  FREE(jvm->resolved);
  jvm->resolved = jvmDir;
}

void installBundledJVMs(LauncherProperties *props) {
  if (props->jvms->size > 0) {
    DWORD i = 0;
    writeMessage(props, OUTPUT_LEVEL_NORMAL, 0,
                 TEXT("... search for bundled JVMs"), 1);
    for (i = 0; i < props->jvms->size; i++) {
      if (props->jvms->items[i]->type == 0 && !isTerminated(props)) {
        resolvePath(props, props->jvms->items[i]);
        writeMessage(props, OUTPUT_LEVEL_NORMAL, 0,
                     TEXT("... install bundled JVM "), 0);
        writeMessage(props, OUTPUT_LEVEL_NORMAL, 0,
                     props->jvms->items[i]->resolved, 1);
        installJVM(props, props->jvms->items[i]);
        if (isTerminated(props))
          return;
        if (isOK(props)) {
          trySetCompatibleJava(props->jvms->items[i]->resolved, props);
          if (props->java != NULL) {
            break;
          } else {
            props->status = ERROR_BUNDLED_JVM_VERIFICATION;
            return;
          }
        } else {
          writeMessage(props, OUTPUT_LEVEL_NORMAL, 0,
                       TEXT("... error occured during JVM extraction"), 1);
          props->status = ERROR_BUNDLED_JVM_EXTRACTION;
          return;
        }
      }
    }
  }
}
void searchJavaInstallationFolder(LauncherProperties *props) {
  TCHAR executablePath[MAX_PATH];
  GetModuleFileName(0, executablePath, MAX_PATH);
  TCHAR *pch = _tcsrchr(executablePath, TEXT('\\'));
  TCHAR installationFolder[MAX_PATH] = {0};
  int i;
  int end = (int)(pch - executablePath);

  for (i = 0; i < end; i++) {
    installationFolder[i] = executablePath[i];
  }
  LPTSTR nestedJreFolder = _tcscat(installationFolder, TEXT("\\bin\\jre"));

  // check if JRE is in installation folder
  if (!fileExists(installationFolder)) {
    // if not exists - return
    return;
  }

  // if exists - copy to temp folder to run uninstaller on that jvm
  // to be able to delete jvm in installation folder
  LPTSTR tempJreFolder = NULL;
  tempJreFolder = appendString(tempJreFolder, props->testJVMFile->resolved);
  tempJreFolder = appendString(tempJreFolder, TEXT("\\_jvm\\"));

  LPTSTR command = NULL;
  appendCommandLineArgument(&command, TEXT("xcopy"));
  appendCommandLineArgument(&command, nestedJreFolder);
  appendCommandLineArgument(&command, tempJreFolder);
  appendCommandLineArgument(&command, TEXT("/e"));
  appendCommandLineArgument(&command, TEXT("/y"));

  writeMessage(props, OUTPUT_LEVEL_DEBUG, 1,
               TEXT("Copying nested JRE to temp folder"), 0);

  executeCommand(props, command, NULL, JVM_EXTRACTION_TIMEOUT,
                 props->stdoutHandle, props->stderrHandle,
                 NORMAL_PRIORITY_CLASS);

  if (fileExists(tempJreFolder)) {
    trySetCompatibleJava(tempJreFolder, props);
  }
}
void searchJavaSystemLocations(LauncherProperties *props) {
  if (props->jvms->size > 0) {
    DWORD i = 0;
    writeMessage(props, OUTPUT_LEVEL_NORMAL, 0,
                 TEXT("Search jvm using some predefined locations"), 1);
    for (i = 0; i < props->jvms->size && !isTerminated(props); i++) {
      resolvePath(props, props->jvms->items[i]);
      if (props->jvms->items[i]->type !=
          0) { // bundled JVMs are already checked
        trySetCompatibleJava(props->jvms->items[i]->resolved, props);
        if (props->java != NULL) {
          break;
        }
      }
    }
  }
}
static void searchJavaOnPath(LauncherProperties *props) {
  // Find the correct size first, then allocate
  DWORD size = GetEnvironmentVariable(PATH_ENV, NULL, 0);
  LPTSTR str = newpTCHAR(size);
  GetEnvironmentVariable(PATH_ENV, str, size);
  StringListEntry *list = splitStringToList(NULL, str, TEXT(';'));
  StringListEntry *iter = list;
  while (iter) {
    // Quickly check for java.exe ...
    LPTSTR javaExecutable = appendString(NULL, iter->string);
    javaExecutable = appendString(javaExecutable, TEXT("\\java.exe"));
    if (fileExists(javaExecutable)) {
      // ... then check properly with trySetCompatibleJava
      LPTSTR msg = appendString(NULL, TEXT("A potential path is "));
      msg = appendString(msg, iter->string);
      writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, msg, 1);
      LPTSTR javaHome = getParentDirectory(iter->string);
      trySetCompatibleJava(javaHome, props);
      FREE(javaHome);
      FREE(msg);
    }
    FREE(javaExecutable);
    iter = iter->next;
  }
  freeStringList(&list);
  FREE(str);
}

void findSystemJava(LauncherProperties *props) {
  // install bundled JVMs if any
  if (isTerminated(props))
    return;
  installBundledJVMs(props);

  if (!isOK(props) || isTerminated(props))
    return;
  // search in <installation folder>/bin/jre
  if (props->java == NULL) {
    writeMessage(props, OUTPUT_LEVEL_NORMAL, 0,
                 TEXT("Search java in installation folder"), 1);
    searchJavaInstallationFolder(props);
  }
  // search JVM in the system paths
  if (props->java == NULL) {
    writeMessage(props, OUTPUT_LEVEL_NORMAL, 0,
                 TEXT("Search java in the system paths"), 1);
    searchJavaSystemLocations(props);
  }

  if (isTerminated(props))
    return;
  if (props->java == NULL) {
    writeMessage(props, OUTPUT_LEVEL_NORMAL, 0,
                 TEXT("Search java in environment variables"), 1);
    searchJavaFromEnvVariables(props);
  }

  if (isTerminated(props))
    return;
  if (props->java == NULL) {
    writeMessage(props, OUTPUT_LEVEL_NORMAL, 0,
                 TEXT("Search java in executable paths"), 1);
    searchJavaOnPath(props);
  }
  // search JVM in the registry
  if (isTerminated(props))
    return;
  if (props->java == NULL) {
    if (IsWow64) {
      writeMessage(props, OUTPUT_LEVEL_NORMAL, 0,
                   TEXT("Search java in 64-bit registry"), 1);
      searchCurrentJavaRegistry(props, 1);
    }
    if (props->java == NULL) {
      writeMessage(props, OUTPUT_LEVEL_NORMAL, 0,
                   TEXT("Search java in 32-bit registry"), 1);
      searchCurrentJavaRegistry(props, 0);
    }
  }
}

void printJavaProperties(LauncherProperties *props, JavaProperties *javaProps) {
  if (javaProps != NULL) {
    LPTSTR jv = getJavaVersionFormatted(javaProps);
    writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, TEXT("Current Java:"), 1);
    writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, TEXT("       javaHome: "), 0);
    writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, javaProps->javaHome, 1);
    writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, TEXT("        javaExe: "), 0);
    writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, javaProps->javaExe, 1);
    writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, TEXT("        version: "), 0);
    writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, jv, 1);
    writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, TEXT("         vendor: "), 0);
    writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, javaProps->vendor, 1);
    writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, TEXT("        os.name: "), 0);
    writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, javaProps->osName, 1);
    writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, TEXT("        os.arch: "), 0);
    writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, javaProps->osArch, 1);
    FREE(jv);
  }
}
