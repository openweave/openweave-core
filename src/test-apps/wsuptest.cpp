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
 *      This file implements test code for the Weave Software Update
 *      profile.
 *
 */

// library includes
#include <iostream>
#include <assert.h>
using namespace std;
#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveMessageLayer.h>
#include <Weave/Profiles/ProfileCommon.h>
#include <Weave/Profiles/software-update/SoftwareUpdateProfile.h>
#if WEAVE_SYSTEM_CONFIG_USE_LWIP
#include <lwip/init.h>
#endif

using namespace ::nl::Inet;
using namespace ::nl::Weave;
using namespace ::nl::Weave::Profiles;
using namespace ::nl::Weave::Profiles::SoftwareUpdate;
// a print method for ImageQuery
void ImageQuery::print(void)
{
  cout<<"<ImageQuery::"<<endl;
  cout<<" product spec: ["<<productSpec.vendorId<<","<<productSpec.productId<<","<<productSpec.productRev<<"]"<<endl;
  cout<<", version: "<<version.printString()<<endl;
  if (localeSpec.theString != NULL)
      cout<<", locale: "<<localeSpec.printString()<<endl;
  cout<<", target node id: "<<targetNodeId<<endl;
  cout<<", integrity types:";
  for (int i=0; i<integrityTypes.theLength; i++) {
    cout<<" "<<integrityTypes.theList[i];
  }
  cout<<", update schemes:";
  for (int i=0; i<updateSchemes.theLength; i++) {
    cout<<" "<<updateSchemes.theList[i];
  }
  cout<<">"<<endl;
}
// and one for ImageQueryResponse
void ImageQueryResponse::print(void)
{
  cout<<"<ImageQueryResponse::";
  cout<<", integrity type: "<<integritySpec.type;
  cout<<", update scheme: "<<updateScheme;
  cout<<", update priority: "<<updatePriority;
  cout<<", update condition: "<<updateCondition;
  cout<<">"<<endl;
}
// static test data
#define TEST_BYTE 0x81
#define SHORT_TEST_NUM 0x8421
#define LONG_TEST_NUM 0x87654321
ProductSpec testSpec(kWeaveVendor_Common, 2, 10);
uint8_t types[3]= { kIntegrityType_SHA160 };
IntegrityTypeList iTList;
uint8_t schemes[4]= { kUpdateScheme_HTTP, kUpdateScheme_BDX };
UpdateSchemeList uSList;
char testPackageString[10]="package!!";
ReferencedString testPackage;
char testVersionString[5]="v1.0";
ReferencedString testVersion;
char testLocaleString[12]="en_AU.UTF-8";
ReferencedString testLocale;
uint8_t fakeTLVDataBytes[10]= { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
ReferencedTLVData fakeTLVData;
uint64_t fakeNodeId=0x12345678;

// and away we go
int main()
{
  // further initialization on test data
  // test limits on inititalizers here
#if WEAVE_SYSTEM_CONFIG_USE_LWIP
  lwip_init();
#endif

  assert(iTList.init(1, types)==WEAVE_NO_ERROR);
  assert(uSList.init(2, schemes)==WEAVE_NO_ERROR);
  assert(testVersion.init((uint8_t)4, testVersionString)==WEAVE_NO_ERROR);
  assert(testPackage.init((uint8_t)9, testPackageString)==WEAVE_NO_ERROR);
  assert(testLocale.init((uint8_t)11, testLocaleString)==WEAVE_NO_ERROR);
  assert(fakeTLVData.init(10, 10, fakeTLVDataBytes)==WEAVE_NO_ERROR);
   // make one of these things (an ImageQuery)
   {
     ImageQuery testQuery;
     assert(testQuery.init(testSpec, testVersion, iTList, uSList)==WEAVE_NO_ERROR);
     cout<<"creating an image query works"<<endl;
     // pack it and reparse
     PacketBuffer *buffer=PacketBuffer::New();
     ImageQuery otherQuery;
     assert(testQuery.pack(buffer)==WEAVE_NO_ERROR);
     assert(ImageQuery::parse(buffer, otherQuery)==WEAVE_NO_ERROR);
     assert(testQuery==otherQuery);
     PacketBuffer::Free(buffer);
   }
   // try the locale option
   {
     ImageQuery testQuery;
     assert(testQuery.init(testSpec, testVersion, iTList, uSList, NULL, &testLocale)==WEAVE_NO_ERROR);
     // pack it and reparse
     PacketBuffer *buffer=PacketBuffer::New();
     ImageQuery otherQuery;
     assert(testQuery.pack(buffer)==WEAVE_NO_ERROR);
     assert(ImageQuery::parse(buffer, otherQuery)==WEAVE_NO_ERROR);
     assert(testQuery==otherQuery);
     PacketBuffer::Free(buffer);
   }
   // try the package option
   {
     ImageQuery testQuery;
     assert(testQuery.init(testSpec, testVersion, iTList, uSList, &testPackage)==WEAVE_NO_ERROR);
     // pack it and reparse
     PacketBuffer *buffer=PacketBuffer::New();
     ImageQuery otherQuery;
     assert(testQuery.pack(buffer)==WEAVE_NO_ERROR);
     assert(ImageQuery::parse(buffer, otherQuery)==WEAVE_NO_ERROR);
     assert(testQuery==otherQuery);
     PacketBuffer::Free(buffer);
   }
   // try 'em both
   {
     ImageQuery testQuery;
     assert(testQuery.init(testSpec, testVersion, iTList, uSList, &testPackage, &testLocale)==WEAVE_NO_ERROR);
     // pack it and reparse
     PacketBuffer *buffer=PacketBuffer::New();
     ImageQuery otherQuery;
     assert(testQuery.pack(buffer)==WEAVE_NO_ERROR);
     assert(ImageQuery::parse(buffer, otherQuery)==WEAVE_NO_ERROR);
     assert(testQuery==otherQuery);
     PacketBuffer::Free(buffer);
   }
  // now try some fake TLV
  {
    ImageQuery testQuery;
    assert(testQuery.init(testSpec, testVersion, iTList, uSList, NULL, NULL, 0, &fakeTLVData)==WEAVE_NO_ERROR);
    // pack it and reparse
    PacketBuffer *buffer=PacketBuffer::New();
    ImageQuery otherQuery;
    assert(testQuery.pack(buffer)==WEAVE_NO_ERROR);
    assert(ImageQuery::parse(buffer, otherQuery)==WEAVE_NO_ERROR);
    assert(testQuery==otherQuery);
    PacketBuffer::Free(buffer);
  }
  // try the target node id
  {
    ImageQuery testQuery;
    assert(testQuery.init(testSpec, testVersion, iTList, uSList, NULL, NULL, fakeNodeId)==WEAVE_NO_ERROR);
    PacketBuffer *buffer=PacketBuffer::New();
    ImageQuery otherQuery;
    assert(testQuery.pack(buffer)==WEAVE_NO_ERROR);
    assert(ImageQuery::parse(buffer, otherQuery)==WEAVE_NO_ERROR);
    assert(testQuery==otherQuery);
    PacketBuffer::Free(buffer);
  }
  cout<<"ImageQuery parse and pack work"<<endl;
  // make an ImageQueryResponse and put it through its paces
  {
    PacketBuffer *buffer=PacketBuffer::New();
    char uriString[26]="http://www.dogbreath.com/";
    ReferencedString testUri;
    assert(testUri.init((uint16_t)25, uriString)==WEAVE_NO_ERROR);
    uint8_t testValue[20] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 1, 15, 16, 17, 18, 19, 20 };
    IntegritySpec testIntegritySpec;
    assert(testIntegritySpec.init(kIntegrityType_SHA160, testValue)==WEAVE_NO_ERROR);
    ImageQueryResponse testResponse;
    assert(testResponse.init(testUri, testVersion, testIntegritySpec, kUpdateScheme_HTTPS, Critical, IfLater, true)==WEAVE_NO_ERROR);
    ImageQueryResponse otherResponse;
    testResponse.pack(buffer);
    ImageQueryResponse::parse(buffer, otherResponse);
    assert(testResponse==otherResponse);
    cout<<"ImageQueryResponse parse and pack work"<<endl;
    PacketBuffer::Free(buffer);
  }
  return 0;
}
