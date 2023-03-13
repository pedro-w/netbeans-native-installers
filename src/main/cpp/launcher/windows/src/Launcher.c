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

#include "Launcher.h"
#include "ExtractUtils.h"
#include "FileUtils.h"
#include "JavaUtils.h"
#include "Main.h"
#include "ProcessUtils.h"
#include "RegistryUtils.h"
#include "StringUtils.h"
#include <shellapi.h>
#include <shlobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <windows.h>

const DWORD NUMBER_OF_HELP_ARGUMENTS = 11;
const DWORD READ_WRITE_BUFSIZE = 65536;
LPCTSTR outputFileArg = TEXT("--output");
LPCTSTR javaArg = TEXT("--javahome");
LPCTSTR debugArg = TEXT("--verbose");
LPCTSTR tempdirArg = TEXT("--tempdir");
LPCTSTR classPathPrepend = TEXT("--classpath-prepend");
LPCTSTR classPathAppend = TEXT("--classpath-append");
LPCTSTR extractArg = TEXT("--extract");
LPCTSTR helpArg = TEXT("--help");
LPCTSTR helpOtherArg = TEXT("/?");
LPCTSTR silentArg = TEXT("--silent");
LPCTSTR nospaceCheckArg = TEXT("--nospacecheck");
LPCTSTR localeArg = TEXT("--locale");

LPCTSTR javaParameterPrefix = TEXT("-J");

LPCTSTR NEW_LINE = TEXT("\n");

LPCTSTR CLASSPATH_SEPARATOR = TEXT(";");
LPCTSTR CLASS_SUFFIX = TEXT(".class");

DWORD isLauncherArgument(LauncherProperties *props, LPCTSTR value) {
  DWORD i;
  for (i = 0; i < props->launcherCommandArguments->size; i++) {
    if (lstrcmp(props->launcherCommandArguments->items[i], value) == 0) {
      return 1;
    }
  }
  return 0;
}

DWORD getArgumentIndex(LauncherProperties *props, LPCTSTR arg,
                       DWORD removeArgument) {
  TCHARList *cmd = props->commandLine;
  DWORD i = 0;
  for (i = 0; i < cmd->size; i++) {
    if (cmd->items[i] != NULL) { // argument has not been cleaned yet
      if (lstrcmp(arg, cmd->items[i]) ==
          0) { // argument is the same as the desired
        if (removeArgument)
          FREE(cmd->items[i]); // free it .. we don`t need it anymore
        return i;
      }
    }
  }
  return cmd->size;
}

DWORD argumentExists(LauncherProperties *props, LPCTSTR arg,
                     DWORD removeArgument) {
  DWORD index = getArgumentIndex(props, arg, removeArgument);
  return (index < props->commandLine->size);
}
LPTSTR getArgumentValue(LauncherProperties *props, LPCTSTR arg,
                        DWORD removeArgument, DWORD mandatory) {
  TCHARList *cmd = props->commandLine;
  LPTSTR result = NULL;
  DWORD i = getArgumentIndex(props, arg, removeArgument);
  if ((i + 1) < cmd->size) {
    // we have at least one more argument
    if (mandatory || !isLauncherArgument(props, cmd->items[i + 1])) {
      result = appendString(NULL, cmd->items[i + 1]);
      if (removeArgument)
        FREE(cmd->items[i + 1]);
    }
  }
  return result;
}

void setOutputFile(LauncherProperties *props, LPCTSTR path) {
  HANDLE out = INVALID_HANDLE_VALUE;

  out = CreateFile(path, GENERIC_WRITE | GENERIC_READ,
                   FILE_SHARE_READ | FILE_SHARE_WRITE, 0, CREATE_ALWAYS, 0, 0);
  if (out != INVALID_HANDLE_VALUE) {
    SetStdHandle(STD_OUTPUT_HANDLE, out);
    SetStdHandle(STD_ERROR_HANDLE, out);
    props->stdoutHandle = out;
    props->stderrHandle = out;
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 0,
                 TEXT("[CMD Argument] Redirect output to file : "), 0);
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, path, 1);
  } else {
    LPTSTR err;
    DWORD code = GetLastError();
    props->status = ERROR_INPUTOUPUT;
    writeError(props, OUTPUT_LEVEL_DEBUG, 1,
               TEXT("[CMD Argument] Can`t create file: "), path, code);
    err = getErrorDescription(code);
    showMessage(
        props,
        TEXT("Can`t redirect output to file!\n\nRequested file : %s\n%s"), 2,
        path, err);
    FREE(err);
  }
}

void setOutput(LauncherProperties *props) {
  LPTSTR file = props->userDefinedOutput;
  if (file != NULL) {
    DWORD exists = fileExists(file);
    if ((exists && !isDirectory(file)) || !exists) {
      setOutputFile(props, file);
    }
  }

  writeMessage(props, OUTPUT_LEVEL_NORMAL, 0,
               (props->outputLevel == OUTPUT_LEVEL_DEBUG)
                   ? TEXT("[CMD Argument] Using debug output.")
                   : TEXT("Using normal output."),
               1);
}

void loadLocalizationStrings(LauncherProperties *props) {

  // load localized messages
  writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("Loading I18N Strings."), 1);
  loadI18NStrings(props);

  if (!isOK(props)) {
    writeMessage(props, OUTPUT_LEVEL_NORMAL, 1,
                 TEXT("Error! Can`t load i18n strings!!"), 1);
    showError(props, INTEGRITY_ERROR_PROP, 1, props->exeName);
  }
}

void readDefaultRoots(LauncherProperties *props) {
  TCHAR appDataValue[MAX_PATH];
  TCHAR localAppDataValue[MAX_PATH];
  SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, appDataValue);
  SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localAppDataValue);
  writeMessage(props, OUTPUT_LEVEL_DEBUG, 0,
               TEXT("Reading Default Userdir and Cachedir roots..."), 1);

  props->defaultUserDirRoot =
      appendString(appendString(NULL, appDataValue), TEXT("\\NetBeans"));
  writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("defaultUserDirRoot: "), 0);
  writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, props->defaultUserDirRoot, 1);

  props->defaultCacheDirRoot = appendString(
      appendString(NULL, localAppDataValue), TEXT("\\NetBeans\\Cache"));
  writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("defaultCacheDirRoot: "), 0);
  writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, props->defaultCacheDirRoot, 1);

  writeMessage(props, OUTPUT_LEVEL_DEBUG, 0,
               TEXT("End of Reading Default Roots."), 1);
}

void createTMPDir(LauncherProperties *props) {
  LPTSTR argTempDir = NULL;
  DWORD createRndSubDir = 1;

  if ((argTempDir = props->userDefinedExtractDir) != NULL) {
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 1,
                 TEXT("[CMD Argument] Extract data to directory: "), 0);
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 1, argTempDir, 1);
    createRndSubDir = 0;
  } else if ((argTempDir = props->userDefinedTempDir) != NULL) {
    writeMessage(props, OUTPUT_LEVEL_NORMAL, 0,
                 TEXT("[CMD Argument] Using tmp directory: "), 0);
    writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, argTempDir, 1);
  }

  createTempDirectory(props, argTempDir, createRndSubDir);
  if (!isOK(props)) {
    showError(props, CANT_CREATE_TEMP_DIR_PROP, 1, props->tmpDir);
  }
}

void checkExtractionStatus(LauncherProperties *props) {
  if (props->status == ERROR_FREESPACE) {
    DWORD hiMB = props->bundledSize->High;
    DWORD lowMB = props->bundledSize->Low;
    DWORD hiMult = (hiMB != 0) ? ((MAXDWORD / 1024) + 1) / 1024 : 0;
    DWORD dw = hiMult * hiMB + lowMB / (1024 * 1024) + 1;
    WCHAR *size = DWORDtoWCHAR(dw);

    writeMessage(props, OUTPUT_LEVEL_DEBUG, 1,
                 TEXT("Error! Not enought free space !"), 1);
    showError(props, NOT_ENOUGH_FREE_SPACE_PROP, 2, size, tempdirArg);
    FREE(size);
  } else if (props->status == ERROR_INTEGRITY) {
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 1,
                 TEXT("Error! Can`t extract data from bundle. Seems to be "
                      "integrirty error!"),
                 1);
    showError(props, INTEGRITY_ERROR_PROP, 1, props->exeName);
  }
}

void trySetCompatibleJava(LPCTSTR location, LauncherProperties *props) {
  if (isTerminated(props))
    return;
  if (location != NULL) {
    JavaProperties *javaProps = NULL;

    if (inList(props->alreadyCheckedJava, location)) {
      writeMessage(props, OUTPUT_LEVEL_NORMAL, 0,
                   "... already checked location ", 0);
      writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, location, 1);
      // return here and don`t proceed with private jre checking since it`s
      // already checked as well
      return;
    } else {
      props->alreadyCheckedJava =
          addStringToList(props->alreadyCheckedJava, location);
    }

    getJavaProperties(location, props, &javaProps);

    if (isOK(props)) {
      writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, "... some java at ", 0);
      writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, location, 1);
      // some java there, check compatibility
      writeMessage(props, OUTPUT_LEVEL_NORMAL, 0,
                   "... checking compatibility of java : ", 0);
      writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, javaProps->javaHome, 1);
      if (isJavaCompatible(javaProps, props->compatibleJava,
                           props->compatibleJavaNumber)) {
        writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, "... compatible", 1);
        props->java = javaProps;
      } else {
        props->status = ERROR_JVM_UNCOMPATIBLE;
        writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, "... uncompatible", 1);
        freeJavaProperties(&javaProps);
      }
    } else {
      writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, "... no java at ", 0);
      writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, location, 1);
      if (props->status == ERROR_INPUTOUPUT) {
        props->status = ERROR_JVM_NOT_FOUND;
      }
    }

    if (props->status == ERROR_JVM_NOT_FOUND) { // check private JRE
      // DWORD privateJreStatus = props->status;
      LPTSTR privateJre = appendString(NULL, location);
      privateJre = appendString(privateJre, TEXT("\\jre"));
      writeMessage(props, OUTPUT_LEVEL_NORMAL, 0,
                   TEXT("... check private jre at "), 0);
      writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, privateJre, 1);

      if (inList(props->alreadyCheckedJava, privateJre)) {
        writeMessage(props, OUTPUT_LEVEL_NORMAL, 0,
                     TEXT("... already checked location "), 0);
        writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, privateJre, 1);
      } else {
        props->alreadyCheckedJava =
            addStringToList(props->alreadyCheckedJava, privateJre);

        getJavaProperties(privateJre, props, &javaProps);
        if (isOK(props)) {
          writeMessage(props, OUTPUT_LEVEL_NORMAL, 0,
                       TEXT("... checking compatibility of private jre : "), 0);
          writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, javaProps->javaHome, 1);
          if (isJavaCompatible(javaProps, props->compatibleJava,
                               props->compatibleJavaNumber)) {
            props->java = javaProps;
            props->status = ERROR_OK;
          } else {
            freeJavaProperties(&javaProps);
            props->status = ERROR_JVM_UNCOMPATIBLE;
          }
        } else if (props->status == ERROR_INPUTOUPUT) {
          props->status = ERROR_JVM_NOT_FOUND;
        }
      }
      FREE(privateJre);
    }
  } else {
    props->status = ERROR_JVM_NOT_FOUND;
  }
}

void resolveTestJVM(LauncherProperties *props) {
  LPTSTR testJVMFile = NULL;
  LPTSTR testJVMClassPath = NULL;

  writeMessage(props, OUTPUT_LEVEL_DEBUG, 0,
               TEXT("Resolving testJVM classpath..."), 1);
  writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("... first step : "), 0);
  writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, props->testJVMFile->path, 1);
  resolvePath(props, props->testJVMFile);
  testJVMFile = props->testJVMFile->resolved;

  writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("... second     : "), 0);
  writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, props->testJVMFile->resolved, 1);

  if (isDirectory(testJVMFile)) { // the directory of the class file is set
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, "... testJVM is : directory ",
                 1);
    testJVMClassPath = appendString(NULL, testJVMFile);
  } else { // testJVMFile is either .class file or .jar/.zip file with the
           // neccessary class file
    LPTSTR dir = getParentDirectory(testJVMFile);
    LPTSTR ptr = testJVMFile;
    do {
      ptr = search(ptr, CLASS_SUFFIX); // check if ptr contains .class
      if (ptr == NULL) {               // .jar or .zip file
        writeMessage(props, OUTPUT_LEVEL_DEBUG, 0,
                     "... testJVM is : ZIP/JAR file", 1);
        testJVMClassPath = appendString(NULL, testJVMFile);
        break;
      }
      ptr += getLength(CLASS_SUFFIX); // shift to the right after the ".class"

      if (ptr == NULL ||
          getLength(ptr) == 0) { // .class was at the end of the ptr
        writeMessage(props, OUTPUT_LEVEL_DEBUG, 0,
                     TEXT("... testJVM is : .class file "), 1);
        testJVMClassPath = appendString(NULL, dir);
        break;
      }
    } while (1);
    FREE(dir);
  }

  FREE(props->testJVMFile->resolved);
  props->testJVMFile->resolved = testJVMClassPath;
  writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("... resolved   : "), 0);
  writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, props->testJVMFile->resolved, 1);
}

void findSuitableJava(LauncherProperties *props) {
  if (!isOK(props))
    return;

  // resolve testJVM file
  resolveTestJVM(props);

  if (!fileExists(props->testJVMFile->resolved)) {
    writeMessage(props, OUTPUT_LEVEL_NORMAL, 1,
                 TEXT("Can`t find TestJVM classpath : "), 0);
    writeMessage(props, OUTPUT_LEVEL_NORMAL, 1, props->testJVMFile->resolved,
                 1);
    showError(props, JVM_NOT_FOUND_PROP, 1, javaArg);
    props->status = ERROR_JVM_NOT_FOUND;
    return;
  } else if (!isTerminated(props)) {

    // try to get java location from command line arguments
    writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, TEXT(""), 1);
    writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, TEXT("Finding JAVA..."), 1);

    if (props->userDefinedJavaHome !=
        NULL) { // using user-defined JVM via command-line parameter
      writeMessage(props, OUTPUT_LEVEL_NORMAL, 0,
                   TEXT("[CMD Argument] Try to use java from "), 0);
      writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, props->userDefinedJavaHome,
                   1);

      trySetCompatibleJava(props->userDefinedJavaHome, props);
      if (props->status == ERROR_JVM_NOT_FOUND ||
          props->status == ERROR_JVM_UNCOMPATIBLE) {
        LPCTSTR prop = (props->status == ERROR_JVM_NOT_FOUND)
                           ? JVM_USER_DEFINED_ERROR_PROP
                           : JVM_UNSUPPORTED_VERSION_PROP;
        showError(props, prop, 1, props->userDefinedJavaHome);
      }
    } else { // no user-specified java argument
      findSystemJava(props);
      if (props->java == NULL) {
        writeMessage(props, OUTPUT_LEVEL_DEBUG, 1,
                     TEXT("... no java was found"), 1);
        if (props->status == ERROR_BUNDLED_JVM_EXTRACTION) {
          showError(props, BUNDLED_JVM_EXTRACT_ERROR_PROP, 1, javaArg);
        } else if (props->status == ERROR_BUNDLED_JVM_VERIFICATION) {
          showError(props, BUNDLED_JVM_VERIFY_ERROR_PROP, 1, javaArg);
        } else {
          showError(props, JVM_NOT_FOUND_PROP, 1, javaArg);
          props->status = ERROR_JVM_NOT_FOUND;
        }
      }
    }

    if (props->java != NULL) {
      writeMessage(props, OUTPUT_LEVEL_NORMAL, 1,
                   TEXT("Compatible jvm was found on the system"), 1);
      printJavaProperties(props, props->java);
    } else {
      writeMessage(props, OUTPUT_LEVEL_NORMAL, 1,
                   TEXT("No compatible jvm was found on the system"), 1);
    }
  }
  return;
}

void resolveLauncherStringProperty(LauncherProperties *props, LPTSTR *result) {
  if (*result != NULL) {
    LPTSTR propStart = search(*result, TEXT("$P{"));
    if (propStart != NULL) {
      LPTSTR propEnd = search(propStart + 3, TEXT("}"));
      if (propEnd != NULL) {
        LPTSTR propName;
        writeMessage(props, OUTPUT_LEVEL_DEBUG, 0,
                     TEXT("... resolving string property"), 1);
        propName = appendStringN(NULL, 0, propStart + 3,
                                 getLength(propStart + 3) - getLength(propEnd));
        writeMessage(props, OUTPUT_LEVEL_DEBUG, 0,
                     TEXT("... property name is : "), 0);
        writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, propName, 1);

        if (propName != NULL) {
          LPCTSTR propValue = getI18nProperty(props, propName);
          if (propValue != NULL) {
            LPTSTR tmp = appendStringN(
                NULL, 0, *result, getLength(*result) - getLength(propStart));
            tmp = appendString(tmp, propValue);
            tmp = appendString(tmp, propEnd + 1);
            FREE(*result);
            *result = tmp;
          }
          FREE(propName);
        }
      }
    }
  }
}

void resolveLauncherProperties(LauncherProperties *props, LPTSTR *result) {
  if (*result != NULL) {
    LPTSTR propStart = search(*result, TEXT("$L{"));
    if (propStart != NULL) {
      LPTSTR propEnd = search(propStart + 3, TEXT("}"));
      if (propEnd != NULL) {
        LPTSTR propName =
            appendStringN(NULL, 0, propStart + 3,
                          getLength(propStart + 3) - getLength(propEnd));
        if (propName != NULL) {
          LPTSTR propValue = NULL;

          if (lstrcmp(propName, TEXT("nbi.launcher.tmp.dir")) == 0) {
            propValue = appendString(NULL, props->tmpDir); // launcher tmpdir
          } else if (lstrcmp(propName, TEXT("nbi.launcher.java.home")) == 0) {
            if (props->java != NULL) {
              propValue = appendString(
                  NULL, props->java->javaHome); // relative to javahome
            }
          } else if (lstrcmp(propName, TEXT("nbi.launcher.user.home")) == 0) {
            propValue = getCurrentUserHome();
          } else if (lstrcmp(propName, TEXT("nbi.launcher.parent.dir")) == 0) {
            propValue = appendString(NULL, props->exeDir); // launcher parent
          }

          if (propValue != NULL) {
            LPTSTR tmp = appendStringN(
                NULL, 0, *result, getLength(*result) - getLength(propStart));
            tmp = appendString(tmp, propValue);
            tmp = appendString(tmp, propEnd + 1);
            FREE(*result);
            FREE(propValue);
            *result = tmp;
          }
        }
      }
    }
  }
}

void resolveString(LauncherProperties *props, LPTSTR *result) {
  LPTSTR tmp = NULL;

  writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("Resolving string : "), 0);
  writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, *result, 1);
  do {
    FREE(tmp);
    tmp = appendString(NULL, *result);
    // writeMessageA(props, OUTPUT_LEVEL_DEBUG, 0, "... step 1 : ", 0);
    // writeMessageW(props, OUTPUT_LEVEL_DEBUG, 0, *result, 1);
    resolveLauncherProperties(props, result);
    // writeMessageA(props, OUTPUT_LEVEL_DEBUG, 0, "... step 2 : ", 0);
    // writeMessageW(props, OUTPUT_LEVEL_DEBUG, 0, *result, 1);
    resolveLauncherStringProperty(props, result);
    // writeMessageA(props, OUTPUT_LEVEL_DEBUG, 0, "... step 3 : ", 0);
    // writeMessageW(props, OUTPUT_LEVEL_DEBUG, 0, *result, 1);
  } while (lstrcmp(tmp, *result) != 0);

  FREE(tmp);

  writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT(".... resolved : "), 0);
  writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, *result, 1);
}

void resolvePath(LauncherProperties *props, LauncherResource *file) {
  DWORD i;

  if (file == NULL)
    return;
  if (file->resolved != NULL)
    return;

  file->resolved = appendString(NULL, file->path);
  resolveString(props, &(file->resolved));

  for (i = 0; i < getLength(file->resolved); i++) {
    if (file->resolved[i] == TEXT('/')) {
      file->resolved[i] = TEXT('\\');
    }
  }
}

void setClasspathElements(LauncherProperties *props) {
  if (isOK(props)) {
    LPTSTR preCP = NULL;
    LPTSTR appCP = NULL;
    LPTSTR tmp = NULL;
    DWORD i = 0;

    writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("Modifying classpath ..."),
                 1);
    // add some libraries to the beginning of the classpath
    while ((preCP = getArgumentValue(props, classPathPrepend, 1, 1)) != NULL) {
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 0,
                   TEXT("... adding entry to the beginning of classpath : "),
                   0);
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, preCP, 1);
      if (props->classpath != NULL) {
        preCP = appendString(preCP, CLASSPATH_SEPARATOR);
      }
      // WCHAR *last = props->classpath;
      resolveString(props, &preCP);
      tmp = appendString(preCP, props->classpath);
      FREE(props->classpath);
      props->classpath = tmp;
    }

    for (i = 0; i < props->jars->size; i++) {
      LPTSTR resolvedCpEntry;
      resolvePath(props, props->jars->items[i]);
      resolvedCpEntry = props->jars->items[i]->resolved;
      if (!fileExists(resolvedCpEntry)) {
        props->status = EXTERNAL_RESOURCE_MISSING;
        showError(props, EXTERNAL_RESOURE_LACK_PROP, 1, resolvedCpEntry);
        return;
      }
      if (props->classpath != NULL) {
        props->classpath = appendString(props->classpath, CLASSPATH_SEPARATOR);
      }
      props->classpath = appendString(props->classpath, resolvedCpEntry);
    }

    // add some libraries to the end of the classpath
    while ((appCP = getArgumentValue(props, classPathAppend, 1, 1)) != NULL) {
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 0,
                   TEXT("... adding entry to the end of classpath : "), 0);
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, appCP, 1);
      if (props->classpath != NULL) {
        props->classpath = appendString(props->classpath, CLASSPATH_SEPARATOR);
      }
      resolveString(props, &appCP);
      props->classpath = appendString(props->classpath, appCP);
      FREE(appCP);
    }

    writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("... finished"), 1);
  }
}

void setAdditionalArguments(LauncherProperties *props) {
  if (isOK(props)) {
    TCHARList *cmd = props->commandLine;
    LPTSTR *javaArgs;
    LPTSTR *appArgs;
    DWORD i = 0;
    DWORD jArg = 0; // java arguments number
    DWORD aArg = 0; // app arguments number

    writeMessage(props, OUTPUT_LEVEL_DEBUG, 0,
                 TEXT("Parsing rest of command line arguments to add them to "
                      "java or application parameters... "),
                 1);

    // get number for array creation
    for (i = 0; i < cmd->size; i++) {
      if (cmd->items[i] != NULL) {
        if (search(cmd->items[i], javaParameterPrefix) != NULL) {
          jArg++;
        } else {
          aArg++;
        }
      }
    }

    // handle DefaultUserDirRoot, DefaultCacheDirRoot - increasing array size
    jArg = jArg + 2;

    // fill the array
    if (jArg > 0) {
      int size = jArg + props->jvmArguments->size;
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, DWORDtoTCHAR(size), 1);
      javaArgs = newppTCHAR(jArg + props->jvmArguments->size);
      for (i = 0; i < props->jvmArguments->size; i++) {
        javaArgs[i] = props->jvmArguments->items[i];
      }
      FREE(props->jvmArguments->items);

      // cont. handle DefaultUserDirRoot, DefaultCacheDirRoot
      // * add -Dnetbeans.default_userdir_root
      // * add -Dnetbeans.default_cachedir_root
      javaArgs[i - 2] = appendString("-Dnetbeans.default_userdir_root=",
                                     props->defaultUserDirRoot);
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 0,
                   TEXT("Added an JVM argument: "), 0);
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, javaArgs[i - 2], 1);

      javaArgs[i - 1] = appendString("-Dnetbeans.default_cachedir_root=",
                                     props->defaultCacheDirRoot);
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 0,
                   TEXT("Added an JVM argument: "), 0);
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, javaArgs[i - 1], 1);
    } else {
      javaArgs = NULL;
    }

    if (aArg > 0) {
      appArgs = newppTCHAR(aArg + props->appArguments->size);
      for (i = 0; i < props->appArguments->size; i++) {
        appArgs[i] = props->appArguments->items[i];
      }
      FREE(props->appArguments->items);
    } else {
      appArgs = NULL;
    }
    jArg = aArg = 0;

    for (i = 0; i < cmd->size; i++) {
      if (cmd->items[i] != NULL) {
        if (search(cmd->items[i], javaParameterPrefix) != NULL) {
          javaArgs[props->jvmArguments->size + jArg] = appendString(
              NULL, cmd->items[i] + getLength(javaParameterPrefix));
          writeMessage(props, OUTPUT_LEVEL_DEBUG, 0,
                       TEXT("... adding JVM argument : "), 0);
          writeMessage(props, OUTPUT_LEVEL_DEBUG, 0,
                       javaArgs[props->jvmArguments->size + jArg], 1);
          jArg++;
        } else {
          appArgs[props->appArguments->size + aArg] =
              appendString(NULL, cmd->items[i]);
          writeMessage(props, OUTPUT_LEVEL_DEBUG, 0,
                       TEXT("... adding APP argument : "), 0);
          writeMessage(props, OUTPUT_LEVEL_DEBUG, 0,
                       appArgs[props->appArguments->size + aArg], 1);
          aArg++;
        }
        FREE(cmd->items[i]);
      }
    }
    props->appArguments->size = props->appArguments->size + aArg;
    props->jvmArguments->size = props->jvmArguments->size + jArg;
    if (props->jvmArguments->items == NULL)
      props->jvmArguments->items = javaArgs;
    if (props->appArguments->items == NULL)
      props->appArguments->items = appArgs;
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 0,
                 TEXT("... resolving jvm arguments"), 1);
    for (i = 0; i < props->jvmArguments->size; i++) {
      resolveString(props, &props->jvmArguments->items[i]);
    }
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 0,
                 TEXT("... resolving app arguments"), 1);
    for (i = 0; i < props->appArguments->size; i++) {
      resolveString(props, &props->appArguments->items[i]);
    }
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 0,
                 TEXT("... finished parsing parameters"), 1);
  }
}
void appendCommandLineArgument(LPTSTR *command, LPCTSTR arg) {
  LPTSTR escapedString = escapeString(arg);
  *command = appendString(*command, escapedString);
  FREE(escapedString);
  *command = appendString(*command, TEXT(" "));
}

void setLauncherCommand(LauncherProperties *props) {
  if (!isOK(props))
    return;

  if (props->java == NULL) {
    props->status = ERROR_JVM_NOT_FOUND;
    return;
  } else {
    LPTSTR command = NULL;
    LPTSTR javaIOTmpdir = NULL;
    DWORD i = 0;

    appendCommandLineArgument(&command, props->java->javaExe);
    command = appendString(command, TEXT("-Djava.io.tmpdir="));
    javaIOTmpdir = getParentDirectory(props->tmpDir);
    appendCommandLineArgument(&command, javaIOTmpdir);
    FREE(javaIOTmpdir);

    for (i = 0; i < props->jvmArguments->size; i++) {
      appendCommandLineArgument(&command, props->jvmArguments->items[i]);
    }

    appendCommandLineArgument(&command, TEXT("-classpath"));
    appendCommandLineArgument(&command, props->classpath);
    appendCommandLineArgument(&command, props->mainClass);

    for (i = 0; i < props->appArguments->size; i++) {
      appendCommandLineArgument(&command, props->appArguments->items[i]);
    }
    props->command = command;
  }
}

void executeMainClass(LauncherProperties *props) {
  if (isOK(props) && !isTerminated(props)) {
    int64t *minSize = newint64_t(0, 0);
    writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, TEXT("Executing main class"),
                 1);
    checkFreeSpace(props, props->tmpDir, minSize);
    if (isOK(props)) {
      HANDLE hErrorRead;
      HANDLE hErrorWrite;
      LPTSTR error = NULL;

      CreatePipe(&hErrorRead, &hErrorWrite, NULL, 0);
      hideLauncherWindows(props);
      executeCommand(props, props->command, NULL, INFINITE, props->stdoutHandle,
                     hErrorWrite, NORMAL_PRIORITY_CLASS);
      if (!isOK(props)) {
        writeMessage(props, OUTPUT_LEVEL_DEBUG, 0,
                     TEXT("... an error occured during JVM running main class"),
                     1);
        props->exitCode = props->status;
      } else {
        LPTSTR s = DWORDtoTCHAR(props->exitCode);
        writeMessage(
            props, OUTPUT_LEVEL_DEBUG, 0,
            TEXT("... main class has finished its work. Exit code is "), 0);
        writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, s, 1);
        FREE(s);
      }

      error = readHandle(hErrorRead);
      if (getLength(error) > 1) {
        BOOL doShowMessage = FALSE;
        LPTSTR ptr = error;
        while (ptr != NULL) {
          // Bug #105165 and #194242
          if ((search(ptr, TEXT("Picked up ")) == NULL &&
               search(ptr, TEXT("fatal: Not a git repository")) == NULL) &&
              getLength(ptr) > 1) {
            doShowMessage = TRUE;
            break;
          }
          ptr = search(ptr, TEXT("\n"));
          if (ptr != NULL)
            ptr++;
        }

        if (doShowMessage && props->exitCode != 0) {
          showMessage(props, getI18nProperty(props, JAVA_PROCESS_ERROR_PROP), 1,
                      error);
        } else {
          writeMessage(props, OUTPUT_LEVEL_NORMAL, 1, error, 1);
        }
      }
      CloseHandle(hErrorWrite);
      CloseHandle(hErrorRead);
      FREE(error);
      Sleep(1);
    } else {
      props->status = ERROR_FREESPACE;
      props->exitCode = props->status;
      writeMessage(
          props, OUTPUT_LEVEL_DEBUG, 1,
          TEXT("... there is not enough space in tmp dir to execute main jar"),
          1);
    }
    FREE(minSize);
  }
}

DWORD isOnlyHelp(LauncherProperties *props) {
  if (argumentExists(props, helpArg, 1) ||
      argumentExists(props, helpOtherArg, 1)) {

    TCHARList *help = newTCHARList(NUMBER_OF_HELP_ARGUMENTS);
    DWORD counter = 0;
    LPTSTR helpString = NULL;

    help->items[counter++] =
        formatMessage(getI18nProperty(props, ARG_JAVA_PROP), 1, javaArg);
    help->items[counter++] =
        formatMessage(getI18nProperty(props, ARG_TMP_PROP), 1, tempdirArg);
    help->items[counter++] =
        formatMessage(getI18nProperty(props, ARG_EXTRACT_PROP), 1, extractArg);
    help->items[counter++] = formatMessage(
        getI18nProperty(props, ARG_OUTPUT_PROPERTY), 1, outputFileArg);
    help->items[counter++] =
        formatMessage(getI18nProperty(props, ARG_DEBUG_PROP), 1, debugArg);
    help->items[counter++] =
        formatMessage(getI18nProperty(props, ARG_CPA_PROP), 1, classPathAppend);
    help->items[counter++] = formatMessage(getI18nProperty(props, ARG_CPP_PROP),
                                           1, classPathPrepend);
    help->items[counter++] = formatMessage(
        getI18nProperty(props, ARG_DISABLE_SPACE_CHECK), 1, nospaceCheckArg);
    help->items[counter++] =
        formatMessage(getI18nProperty(props, ARG_LOCALE_PROP), 1, localeArg);
    help->items[counter++] =
        formatMessage(getI18nProperty(props, ARG_SILENT_PROP), 1, silentArg);
    help->items[counter++] =
        formatMessage(getI18nProperty(props, ARG_HELP_PROP), 1, helpArg);

    for (counter = 0; counter < NUMBER_OF_HELP_ARGUMENTS; counter++) {
      helpString = appendString(appendString(helpString, help->items[counter]),
                                NEW_LINE);
    }
    freeTCHARList(&help);
    showMessage(props, helpString, 0);
    FREE(helpString);
    return 1;
  }
  return 0;
}

DWORD isSilent(LauncherProperties *props) { return props->silentMode; }

TCHARList *getCommandlineArguments() {
  int argumentsNumber = 0;
  int i = 0;
  // This is always in WCHARS
  LPWSTR *commandLine = CommandLineToArgvW(GetCommandLineW(), &argumentsNumber);
  // the first is always the running program..  we don`t need it
  // it is that same as GetModuleFileNameW says
  TCHARList *commandsList = newTCHARList((DWORD)(argumentsNumber - 1));
  for (i = 1; i < argumentsNumber; i++) {
    commandsList->items[i - 1] = WSTRtoTSTR(commandLine[i]);
  }

  LocalFree(commandLine);
  return commandsList;
}

LauncherProperties *createLauncherProperties() {
  LauncherProperties *props =
      (LauncherProperties *)LocalAlloc(LPTR, sizeof(LauncherProperties));
  DWORD c = 0;
  props->launcherCommandArguments = newTCHARList(11);
  props->launcherCommandArguments->items[c++] =
      appendString(NULL, outputFileArg);
  props->launcherCommandArguments->items[c++] = appendString(NULL, javaArg);
  props->launcherCommandArguments->items[c++] = appendString(NULL, debugArg);
  props->launcherCommandArguments->items[c++] = appendString(NULL, tempdirArg);
  props->launcherCommandArguments->items[c++] =
      appendString(NULL, classPathPrepend);
  props->launcherCommandArguments->items[c++] =
      appendString(NULL, classPathAppend);
  props->launcherCommandArguments->items[c++] = appendString(NULL, extractArg);
  props->launcherCommandArguments->items[c++] = appendString(NULL, helpArg);
  props->launcherCommandArguments->items[c++] =
      appendString(NULL, helpOtherArg);
  props->launcherCommandArguments->items[c++] = appendString(NULL, silentArg);
  props->launcherCommandArguments->items[c++] =
      appendString(NULL, nospaceCheckArg);

  props->jvmArguments = NULL;
  props->appArguments = NULL;
  props->extractOnly = 0;
  props->mainClass = NULL;
  props->testJVMClass = NULL;
  props->classpath = NULL;
  props->jars = NULL;
  props->testJVMFile = NULL;
  props->tmpDir = NULL;
  props->tmpDirCreated = 0;
  props->compatibleJava = NULL;
  props->compatibleJavaNumber = 0;
  props->java = NULL;
  props->command = NULL;
  props->jvms = NULL;
  props->other = NULL;
  props->alreadyCheckedJava = NULL;
  props->exePath = getExePath();
  props->exeName = getExeName();
  props->exeDir = getExeDirectory();
  props->handler = CreateFile(props->exePath, GENERIC_READ, FILE_SHARE_READ,
                              NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  props->bundledSize = newint64_t(0, 0);
  props->bundledNumber = 0;
  props->commandLine = getCommandlineArguments();
  props->status = ERROR_OK;
  props->exitCode = 0;
  props->outputLevel = argumentExists(props, debugArg, 1) ? OUTPUT_LEVEL_DEBUG
                                                          : OUTPUT_LEVEL_NORMAL;
  props->stdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
  props->stderrHandle = GetStdHandle(STD_ERROR_HANDLE);
  props->bufsize = READ_WRITE_BUFSIZE;
  props->restOfBytes = createSizedString();
  props->I18N_PROPERTIES_NUMBER = 0;
  props->i18nMessages = NULL;
  props->userDefinedJavaHome = getArgumentValue(props, javaArg, 1, 1);
  props->userDefinedTempDir = getArgumentValue(props, tempdirArg, 1, 1);
  props->userDefinedLocale = getArgumentValue(props, localeArg, 0, 1);
  props->userDefinedExtractDir = NULL;
  props->extractOnly = 0;

  if (argumentExists(props, extractArg, 0)) {
    props->userDefinedExtractDir = getArgumentValue(props, extractArg, 1, 0);
    if (props->userDefinedExtractDir ==
        NULL) { // next argument is null or another launcher argument
      props->userDefinedExtractDir = getCurrentDirectory();
    }
    props->extractOnly = 1;
  }
  props->userDefinedOutput = getArgumentValue(props, outputFileArg, 1, 1);
  props->checkForFreeSpace = !argumentExists(props, nospaceCheckArg, 0);
  props->silentMode = argumentExists(props, silentArg, 0);
  props->launcherSize = getFileSize(props->exePath);
  props->isOnlyStub = (compare(props->launcherSize, STUB_FILL_SIZE) < 0);
  return props;
}
void freeLauncherResourceList(LauncherResourceList **list) {
  if (*list != NULL) {
    if ((*list)->items != NULL) {
      DWORD i = 0;
      for (i = 0; i < (*list)->size; i++) {
        freeLauncherResource(&((*list)->items[i]));
      }
      FREE((*list)->items);
    }
    FREE((*list));
  }
}

void freeLauncherProperties(LauncherProperties **props) {
  if ((*props) != NULL) {
    DWORD i = 0;
    writeMessage(*props, OUTPUT_LEVEL_DEBUG, 0,
                 TEXT("Closing launcher properties"), 1);
    freeTCHARList(&((*props)->appArguments));
    freeTCHARList(&((*props)->jvmArguments));

    FREE((*props)->mainClass);
    FREE((*props)->testJVMClass);
    FREE((*props)->classpath);
    freeLauncherResourceList(&((*props)->jars));

    freeLauncherResourceList(&((*props)->jvms));

    freeLauncherResourceList(&((*props)->other));

    freeLauncherResource(&((*props)->testJVMFile));

    FREE((*props)->tmpDir);
    for (i = 0; i < (*props)->compatibleJavaNumber; i++) {
      JavaCompatible *jc = (*props)->compatibleJava[i];
      if (jc != NULL) {
        FREE(jc->minVersion);
        FREE(jc->maxVersion);
        FREE(jc->vendor);
        FREE(jc->osName);
        FREE(jc->osArch);
        FREE((*props)->compatibleJava[i]);
      }
    }
    freeStringList(&((*props)->alreadyCheckedJava));
    FREE((*props)->compatibleJava);
    freeJavaProperties(&((*props)->java));
    FREE((*props)->userDefinedJavaHome);
    FREE((*props)->userDefinedTempDir);
    FREE((*props)->userDefinedExtractDir);
    FREE((*props)->userDefinedOutput);
    FREE((*props)->userDefinedLocale);
    FREE((*props)->command);
    FREE((*props)->exePath);
    FREE((*props)->exeDir);
    FREE((*props)->exeName);
    FREE((*props)->bundledSize);
    FREE((*props)->launcherSize);
    freeSizedString(&((*props)->restOfBytes));

    flushHandle((*props)->stdoutHandle);
    flushHandle((*props)->stderrHandle);
    CloseHandle((*props)->stdoutHandle);
    CloseHandle((*props)->stderrHandle);

    freeI18NMessages((*props));
    freeTCHARList(&((*props)->launcherCommandArguments));
    freeTCHARList(&((*props)->commandLine));
    CloseHandle((*props)->handler);

    FREE((*props));
  }
  return;
}
void printStatus(LauncherProperties *props) {
  char *s = DWORDtoCHAR(props->status);
  writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("... EXIT status : "), 0);
  writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, s, 1);
  FREE(s);
  s = DWORDtoCHAR(props->exitCode);
  writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("... EXIT code : "), 0);
  writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, s, 1);
  FREE(s);
}

void processLauncher(LauncherProperties *props) {
  setOutput(props);
  if (!isOK(props) || isTerminated(props))
    return;

  setProgressRange(props, props->launcherSize);
  if (!isOK(props) || isTerminated(props))
    return;

  skipStub(props);
  if (!isOK(props) || isTerminated(props))
    return;

  readDefaultRoots(props);
  if (!isOK(props) || isTerminated(props))
    return;

  loadLocalizationStrings(props);
  if (!isOK(props) || isTerminated(props))
    return;

  if (isOnlyHelp(props))
    return;

  setProgressTitleString(props, getI18nProperty(props, MSG_PROGRESS_TITLE));
  setMainWindowTitle(props, getI18nProperty(props, MAIN_WINDOW_TITLE));
  showLauncherWindows(props);
  if (!isOK(props) || isTerminated(props))
    return;

  readLauncherProperties(props);
  checkExtractionStatus(props);
  if (!isOK(props) || isTerminated(props))
    return;

  if (props->bundledNumber > 0) {
    createTMPDir(props);
    if (isOK(props)) {
      checkFreeSpace(props, props->tmpDir, props->bundledSize);
      checkExtractionStatus(props);
    }
  }

  if (isOK(props)) {
    extractJVMData(props);
    checkExtractionStatus(props);
    if (isOK(props) && !props->extractOnly && !isTerminated(props)) {
      findSuitableJava(props);
    }

    if (isOK(props) && !isTerminated(props)) {
      extractData(props);
      checkExtractionStatus(props);
      if (isOK(props) && (props->java != NULL) && !isTerminated(props)) {
        setClasspathElements(props);
        if (isOK(props) && (props->java != NULL) && !isTerminated(props)) {
          setAdditionalArguments(props);
          setLauncherCommand(props);
          Sleep(500);
          executeMainClass(props);
        }
      }
    }
  }

  if (!props->extractOnly && props->tmpDirCreated) {
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 0,
                 TEXT("... deleting temporary directory "), 1);
    deleteDirectory(props, props->tmpDir);
  }
}
