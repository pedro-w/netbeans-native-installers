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

#ifndef _RegistryUtils_H
#define _RegistryUtils_H

#include <windows.h>
#ifdef __cplusplus
extern "C" {
#endif

LPTSTR getStringValue(HKEY root, LPCTSTR key, LPCTSTR valueName,
                      BOOL access64key);
LPTSTR getStringValuePC(HKEY root, LPCTSTR parentkey, LPCTSTR childkey,
                        LPCTSTR valueName, BOOL access64key);

#ifndef KEY_WOW64_32KEY
#define KEY_WOW64_32KEY 0x0200
#endif
#ifndef KEY_WOW64_64KEY
#define KEY_WOW64_64KEY 0x0100
#endif

#ifdef __cplusplus
}
#endif

#endif /* _RegistryUtils_H */
