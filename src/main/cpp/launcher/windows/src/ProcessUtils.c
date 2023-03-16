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

#include "ProcessUtils.h"
#include "FileUtils.h"
#include "StringUtils.h"

const DWORD DEFAULT_PROCESS_TIMEOUT = 30000; // 30 sec

DWORD readBuf(HANDLE hRead, WCHAR *buf, DWORD *bytesRead, HANDLE hWrite) {
  ReadFile(hRead, buf, STREAM_BUF_LENGTH - 1, bytesRead, NULL);

  if ((*bytesRead) > 0 && hWrite != INVALID_HANDLE_VALUE) {
    WriteFile(hWrite, buf, (*bytesRead), NULL, 0);
  }
  ZERO(buf, STREAM_BUF_LENGTH);
  return 0;
}

DWORD readNextData(HANDLE hRead, WCHAR *buf, HANDLE hWrite) {
  DWORD bytesRead;
  DWORD bytesAvailable;
  ZERO(buf, STREAM_BUF_LENGTH);

  PeekNamedPipe(hRead, buf, STREAM_BUF_LENGTH - 1, &bytesRead, &bytesAvailable,
                NULL);
  if (bytesRead != 0) {
    ZERO(buf, STREAM_BUF_LENGTH);
    if (bytesAvailable >= STREAM_BUF_LENGTH) {
      while (bytesRead >= STREAM_BUF_LENGTH - 1) {
        readBuf(hRead, buf, &bytesRead, hWrite);
      }
    } else {
      readBuf(hRead, buf, &bytesRead, hWrite);
    }
    return bytesRead;
  }
  return 0;
}

// get already running process stdout
DWORD readProcessStream(PROCESS_INFORMATION pi, HANDLE currentProcessStdin,
                        HANDLE currentProcessStdout,
                        HANDLE currentProcessStderr, DWORD timeOut,
                        HANDLE hWriteInput, HANDLE hWriteOutput,
                        HANDLE hWriteError) {
  DWORD started = GetTickCount();
  WCHAR buf[STREAM_BUF_LENGTH];
  DWORD exitCode = 0;
  DWORD outRead = 0;
  DWORD errRead = 0;
  DWORD inRead = 0;
  while (1) {
    outRead = readNextData(currentProcessStdout, buf, hWriteOutput);
    errRead = readNextData(currentProcessStderr, buf, hWriteError);
    inRead = readNextData(hWriteInput, buf, currentProcessStdin);
    GetExitCodeProcess(pi.hProcess, &exitCode);
    if (exitCode != STILL_ACTIVE)
      break;

    if (outRead == 0 && errRead == 0 && inRead == 0 && timeOut != INFINITE) {
      if ((GetTickCount() - started) > timeOut)
        break;
    }
    // avoid extra using of CPU resources
    Sleep(1);
  }
  return exitCode;
}
LPTSTR readHandle(HANDLE hRead) {
  BYTE buf[STREAM_BUF_LENGTH];
  DWORD read;
  DWORD bytesRead;
  DWORD bytesAvailable;
  SizedString *sz = createSizedString();
  while (1) {
    PeekNamedPipe(hRead, buf, STREAM_BUF_LENGTH - 1, &bytesRead,
                  &bytesAvailable, NULL);
    if (bytesAvailable == 0)
      break;
    ReadFile(hRead, buf, STREAM_BUF_LENGTH - 1, &read, NULL);
    if (read == 0)
      break;
    // Always read in CHARs, may need to convert later
    appendToSizedString(sz, buf, read);
  }
  LPTSTR output = createTCHAR(sz);
  freeSizedString(&sz);
  return output;
}

// run process and get its standard output
// command - executing command
// timeLimitMillis - timeout of the process running without any output
// dir - working directory
// return ERROR_ON_EXECUTE_PROCESS for serios error
// return ERROR_PROCESS_TIMEOUT for timeout

void executeCommand(LauncherProperties *props, LPCTSTR command, LPCTSTR dir,
                    DWORD timeLimitMillis, HANDLE hWriteOutput,
                    HANDLE hWriteError, DWORD priority) {
  STARTUPINFO si;
  SECURITY_ATTRIBUTES sa;
  PROCESS_INFORMATION pi;

  HANDLE newProcessInput;
  HANDLE newProcessOutput;
  HANDLE newProcessError;

  HANDLE currentProcessStdout;
  HANDLE currentProcessStdin;
  HANDLE currentProcessStderr;

  LPCTSTR directory;

  sa.lpSecurityDescriptor = NULL;
  sa.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa.bInheritHandle = TRUE;

  if (!CreatePipe(&newProcessInput, &currentProcessStdin, &sa, 0)) {
    writeError(props, OUTPUT_LEVEL_NORMAL, 1,
               TEXT("Can`t create pipe for input. "), NULL, GetLastError());
    props->status = ERROR_ON_EXECUTE_PROCESS;
    return;
  }

  if (!CreatePipe(&currentProcessStdout, &newProcessOutput, &sa, 0)) {
    writeError(props, OUTPUT_LEVEL_NORMAL, 1,
               TEXT("Can`t create pipe for output. "), NULL, GetLastError());
    CloseHandle(newProcessInput);
    CloseHandle(currentProcessStdin);
    props->status = ERROR_ON_EXECUTE_PROCESS;
    return;
  }

  if (!CreatePipe(&currentProcessStderr, &newProcessError, &sa, 0)) {
    writeError(props, OUTPUT_LEVEL_NORMAL, 1,
               TEXT("Can`t create pipe for error. "), NULL, GetLastError());
    CloseHandle(newProcessInput);
    CloseHandle(currentProcessStdin);
    CloseHandle(newProcessOutput);
    CloseHandle(currentProcessStdout);
    props->status = ERROR_ON_EXECUTE_PROCESS;
    return;
  }

  GetStartupInfo(&si);

  si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
  si.wShowWindow = SW_HIDE;
  si.hStdOutput = newProcessOutput;
  si.hStdError = newProcessError;
  si.hStdInput = newProcessInput;
  // TODO this leaks memory if dir == NULL
  directory = (dir != NULL) ? dir : getCurrentDirectory();
  writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, TEXT("Create new process: "), 1);
  writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, TEXT("          command : "), 0);
  writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, command, 1);
  writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, TEXT("        directory : "), 0);
  writeMessage(props, OUTPUT_LEVEL_NORMAL, 0, directory, 1);

  props->exitCode = ERROR_OK;
  // TODO is it OK to cast away const here
  if (CreateProcess(NULL, (LPTSTR)command, NULL, NULL, TRUE,
                    CREATE_NEW_CONSOLE | CREATE_NO_WINDOW |
                        CREATE_DEFAULT_ERROR_MODE | priority,
                    NULL, directory, &si, &pi)) {
    // TODO
    // Check whether folder virtualization can break things and provide method
    // to disable it if necessary I am not sure whether we need it off or on.
    // http://www.netbeans.org/issues/show_bug.cgi?id=122186
    DWORD timeOut =
        timeLimitMillis == 0 ? DEFAULT_PROCESS_TIMEOUT : timeLimitMillis;
    props->status = ERROR_OK;
    writeMessage(props, OUTPUT_LEVEL_DEBUG, 0, TEXT("... process created"), 1);

    props->exitCode = readProcessStream(
        pi, currentProcessStdin, currentProcessStdout, currentProcessStderr,
        timeOut, newProcessInput, hWriteOutput, hWriteError);

    if (props->exitCode == STILL_ACTIVE) {
      // actually we have reached the timeout of the process and need to
      // terminate it
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 1, TEXT("... process timed out"),
                   1);
      GetExitCodeProcess(pi.hProcess, &(props->exitCode));

      if (props->exitCode == STILL_ACTIVE) {
        TerminateProcess(pi.hProcess, 0);
        writeMessage(props, OUTPUT_LEVEL_DEBUG, 1,
                     TEXT("... terminate process"), 1);
        // Terminating process...It worked too much without any
        // stdout/stdin/stderr
        props->status = ERROR_PROCESS_TIMEOUT; // terminated by timeout
      }
    } else {
      // application finished its work... succesfully or not - it doesn`t matter
      writeMessage(props, OUTPUT_LEVEL_DEBUG, 0,
                   TEXT("... process finished his work"), 1);
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
  } else {
    writeError(props, OUTPUT_LEVEL_DEBUG, 1, TEXT("... can`t create process."),
               NULL, GetLastError());
    props->status = ERROR_ON_EXECUTE_PROCESS;
  }

  CloseHandle(newProcessInput);
  CloseHandle(newProcessOutput);
  CloseHandle(newProcessError);
  CloseHandle(currentProcessStdin);
  CloseHandle(currentProcessStdout);
  CloseHandle(currentProcessStderr);
}
