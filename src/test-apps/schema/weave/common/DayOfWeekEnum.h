/*
 *
 *    Copyright (c) 2016-2017 Nest Labs, Inc.
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

#ifndef _WEAVE_COMMON__DAY_OF_WEEK_ENUM_H_
#define _WEAVE_COMMON__DAY_OF_WEEK_ENUM_H_

namespace Schema {
namespace Weave {
namespace Common {

    enum DayOfWeek {
        DAY_OF_WEEK_SUNDAY = 1,
        DAY_OF_WEEK_MONDAY = 2,
        DAY_OF_WEEK_TUESDAY = 4,
        DAY_OF_WEEK_WEDNESDAY = 8,
        DAY_OF_WEEK_THURSDAY = 16,
        DAY_OF_WEEK_FRIDAY = 32,
        DAY_OF_WEEK_SATURDAY = 64,
    };

}
}

}
#endif // _WEAVE_COMMON__DAY_OF_WEEK_ENUM_H_
