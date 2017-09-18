/*
 *
 *    Copyright (c) 2013-2017 Nest Labs, Inc.
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
 *      Base-64 utility functions.
 *
 */

#import "Base64Encoding.h"
#import "NLLogging.h"

enum {
  kUnknownChar = -1,
  kPaddingChar = -2,
  kIgnoreChar = -3
};

@implementation Base64Encoding
{
    NSData *_charMapData;
    char *_charMap;
    int _reverseCharMap[128];
    int _shift;
    int _mask;
    char _paddingChar;
    int _padLen;
}

+ (id)createBase64StringEncoding {
  Base64Encoding *result = [self stringEncodingWithString:@"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"];
  [result setPaddingChar:'='];
  [result setUsePaddingDuringEncoding:YES];
  return result;
}

NS_INLINE int lcm(int a, int b) {
  for (int aa = a, bb = b; ; ) {
    if (aa == bb)
      return aa;
    else if (aa < bb)
      aa += a;
    else
      bb += b;
  }
}

+ (id)stringEncodingWithString:(NSString *)string {
  return [[self alloc] initWithString:string];
}

- (id)initWithString:(NSString *)string {
  if ((self = [super init])) {
    _charMapData = [string dataUsingEncoding:NSASCIIStringEncoding];
    if (!_charMapData) {
      WDM_LOG_DEBUG(@"Unable to convert string to ASCII");
      return nil;
    }
    _charMap = (char *)[_charMapData bytes];
    NSUInteger length = [_charMapData length];
    if (length < 2 || length > 128 || length & (length - 1)) {
      WDM_LOG_DEBUG(@"Length not a power of 2 between 2 and 128");
      return nil;
    }

    memset(_reverseCharMap, kUnknownChar, sizeof(_reverseCharMap));
    for (unsigned int i = 0; i < length; i++) {
      if (_reverseCharMap[(int)_charMap[i]] != kUnknownChar) {
        WDM_LOG_DEBUG(@"Duplicate character at pos %d", i);
        return nil;
      }
      _reverseCharMap[(int)_charMap[i]] = i;
    }

    for (NSUInteger i = 1; i < length; i <<= 1)
      _shift++;
    _mask = (1 << _shift) - 1;
    _padLen = lcm(8, _shift) / _shift;
  }
  return self;
}

- (void)dealloc {
  _charMapData = nil;
}

- (NSString *)description {
  // TODO(iwade) track synonyms
  return [NSString stringWithFormat:@"<Base%d StringEncoder: %@>",
          1 << _shift, _charMapData];
}

- (NSString *)encode:(NSData *)inData {
  NSUInteger inLen = [inData length];
  if (inLen <= 0) {
    WDM_LOG_DEBUG(@"Empty input");
    return @"";
  }
  unsigned char *inBuf = (unsigned char *)[inData bytes];
  NSUInteger inPos = 0;

  NSUInteger outLen = (inLen * 8 + _shift - 1) / _shift;
  if (_usePaddingDuringEncoding) {
    outLen = ((outLen + _padLen - 1) / _padLen) * _padLen;
  }
  NSMutableData *outData = [NSMutableData dataWithLength:outLen];
  unsigned char *outBuf = (unsigned char *)[outData mutableBytes];
  NSUInteger outPos = 0;

  int buffer = inBuf[inPos++];
  int bitsLeft = 8;
  while (bitsLeft > 0 || inPos < inLen) {
    if (bitsLeft < _shift) {
      if (inPos < inLen) {
        buffer <<= 8;
        buffer |= (inBuf[inPos++] & 0xff);
        bitsLeft += 8;
      } else {
        int pad = _shift - bitsLeft;
        buffer <<= pad;
        bitsLeft += pad;
      }
    }
    int idx = (buffer >> (bitsLeft - _shift)) & _mask;
    bitsLeft -= _shift;
    outBuf[outPos++] = _charMap[idx];
  }

  if (_usePaddingDuringEncoding) {
    while (outPos < outLen)
      outBuf[outPos++] = _paddingChar;
  }

  assert(outPos == outLen); // Underflowed output buffer assertion
  [outData setLength:outPos];

  return [[NSString alloc] initWithData:outData encoding:NSASCIIStringEncoding];
}

- (NSData *)decode:(NSString *)inString {
  char *inBuf = (char *)[inString cStringUsingEncoding:NSASCIIStringEncoding];
  if (!inBuf) {
    WDM_LOG_DEBUG(@"unable to convert buffer to ASCII");
    return nil;
  }
  NSUInteger inLen = strlen(inBuf);

  NSUInteger outLen = inLen * _shift / 8;
  NSMutableData *outData = [NSMutableData dataWithLength:outLen];
  unsigned char *outBuf = (unsigned char *)[outData mutableBytes];
  NSUInteger outPos = 0;

  int buffer = 0;
  int bitsLeft = 0;
  BOOL expectPad = NO;
  for (NSUInteger i = 0; i < inLen; i++) {
    int val = _reverseCharMap[(int)inBuf[i]];
    switch (val) {
      case kIgnoreChar:
        break;
      case kPaddingChar:
        expectPad = YES;
        break;
      case kUnknownChar:
        WDM_LOG_DEBUG(@"Unexpected data in input pos %lu", (unsigned long)i);
        return nil;
      default:
        if (expectPad) {
          WDM_LOG_DEBUG(@"Expected further padding characters");
          return nil;
        }
        buffer <<= _shift;
        buffer |= val & _mask;
        bitsLeft += _shift;
        if (bitsLeft >= 8) {
          outBuf[outPos++] = (unsigned char)(buffer >> (bitsLeft - 8));
          bitsLeft -= 8;
        }
        break;
    }
  }

  if (bitsLeft && buffer & ((1 << bitsLeft) - 1)) {
    WDM_LOG_DEBUG(@"Incomplete trailing data");
    return nil;
  }

  // Shorten buffer if needed due to padding chars
  assert(outPos <= outLen); // Overflowed buffer
  [outData setLength:outPos];

  return outData;
}

- (void)setPaddingChar:(char)c {
  _paddingChar = c;
  _reverseCharMap[(int)c] = kPaddingChar;
}

@end
