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

#ifndef _Launcher_H
#define _Launcher_H

#include "Errors.h"
#include "JavaUtils.h"
#include "Types.h"
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

extern LPCTSTR NEW_LINE;

LauncherProperties *createLauncherProperties();
void freeLauncherProperties(LauncherProperties **props);

void printStatus(LauncherProperties *props);
void trySetCompatibleJava(LPCTSTR location, LauncherProperties *props);
DWORD isSilent(LauncherProperties *props);
DWORD isLauncherArgument(LauncherProperties *props, LPCTSTR value);
void processLauncher(LauncherProperties *props);

void resolvePath(LauncherProperties *props, LauncherResource *file);
void resolveString(LauncherProperties *props, LPTSTR *result);
void resolveLauncherProperties(LauncherProperties *props, LPTSTR *result);
void appendCommandLineArgument(LPTSTR *command, LPCTSTR arg);

#ifdef __cplusplus
}
#endif

#endif /* _Launcher_H */
