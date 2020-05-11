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
 *      Translation of WdmClientFlushUpdate errors into native Objective C types.
 *
 */

#import "NLWeaveError_Protected.h"
#import "NLWdmClientFlushUpdateError.h"

@interface NLWdmClientFlushUpdateError () {
    NSString * _errorReport;
}
@end

@implementation NLWdmClientFlushUpdateError

- (id)initWithWeaveError:(WEAVE_ERROR)weaveError
                  report:(NSString *)report
                    path:(NSString *)path
                dataSink:(NLGenericTraitUpdatableDataSink*) dataSink
{
    self = [super initWithWeaveError:weaveError report:report];
    if (self)
    {
        _path = path;
        _dataSink = dataSink;
    }
    return self;
}

- (NSString *)description
{
    NSString * emptyErrorReport = [NSString
        stringWithFormat:@"No Error information available. errorCode = %ld (0x%lx), failed path is %@", (long) self.errorCode, (long) self.errorCode, _path];

    return _errorReport ? ([NSString stringWithFormat:@"%@ - errorCode: %ld, failed path is %@", _errorReport, (long) self.errorCode, _path])
                        : emptyErrorReport;
}
@end
