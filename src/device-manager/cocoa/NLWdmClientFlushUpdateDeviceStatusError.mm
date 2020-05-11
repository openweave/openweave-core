/*
 *
 *    Copyright (c) 2020 Google, LLC.
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

/**
 *    @file
 *      Objective-C representation of a device status for WdmClient flush update operation
 *
 */

// Note that the choice of namespace alias must be made up front for each and every compile unit
// This is because many include paths could set the default alias to unintended target.

#import "NLProfileStatusError.h"
#import "NLWdmClientFlushUpdateDeviceStatusError.h"

@interface NLWdmClientFlushUpdateDeviceStatusError () {
    NSString * _statusReport;
}

@end

@implementation NLWdmClientFlushUpdateDeviceStatusError

- (id)initWithProfileId:(uint32_t)profileId
             statusCode:(uint16_t)statusCode
              errorCode:(uint32_t)errorCode
           statusReport:(NSString *)statusReport
                   path:(NSString *)path
               dataSink:(NLGenericTraitUpdatableDataSink*) dataSink
{
    self = [super initWithProfileId:profileId statusCode:statusCode errorCode:errorCode statusReport:statusReport];
    if (self)
    {
        _path = path;
        _dataSink = dataSink;
    }
    return self;
}

- (NSString *)description
{
    NSString * emptyStatusReport =
            [NSString stringWithFormat:@"No StatusReport available. profileId = %ld, statusCode = %ld, SysErrorCode = %ld, failed path is %@ ",
                                       (long) self.profileId, (long) self.statusCode, (long) self.errorCode, _path];

    return _statusReport ? ([NSString stringWithFormat:@"%@, SysErrorCode = %ld, failed path is %@", _statusReport, (long) self.errorCode, _path])
                         : emptyStatusReport;
}

@end
