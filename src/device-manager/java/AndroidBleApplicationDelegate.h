/*
 *
 *    Copyright (c) 2014-2017 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */
#ifndef ANDROID_BLEAPPLICATIONDELEGATE_H_
#define ANDROID_BLEAPPLICATIONDELEGATE_H_

#include <BleLayer/BleApplicationDelegate.h>

typedef void (*NotifyWeaveConnectionClosedCallback)(BLE_CONNECTION_OBJECT connObj);

class AndroidBleApplicationDelegate :
    public nl::Ble::BleApplicationDelegate
{
 public:
    NotifyWeaveConnectionClosedCallback NotifyWeaveConnectionClosedCb;

    // ctor
    AndroidBleApplicationDelegate();

    void NotifyWeaveConnectionClosed(BLE_CONNECTION_OBJECT connObj);

    void SetNotifyWeaveConnectionClosedCallback(NotifyWeaveConnectionClosedCallback cb);
};

#endif /* ANDROID_BLEAPPLICATIONDELEGATE_H_ */
