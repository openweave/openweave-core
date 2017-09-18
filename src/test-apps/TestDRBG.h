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

/**
 *    @file
 *      NIST DRBG Test Vectors taken from:
 *
 *        http://csrc.nist.gov/groups/STM/cavp/documents/drbg/drbgtestvectors.zip
 *
 */

#include <stdlib.h>
#include <stdint.h>

// ============================================================
// [AES-128 no df]
// [PredictionResistance = False]
// [EntropyInputLen = 256]
// [NonceLen = 0]
// [PersonalizationStringLen = 0]
// [AdditionalInputLen = 0]
// [ReturnedBitsLen = 512]
// ============================================================
//
// COUNT = 0
// EntropyInput = ed1e7f21ef66ea5d8e2a85b9337245445b71d6393a4eecb0e63c193d0f72f9a9
// Nonce =
// PersonalizationString =
// EntropyInputReseed = 303fb519f0a4e17d6df0b6426aa0ecb2a36079bd48be47ad2a8dbfe48da3efad
// AdditionalInputReseed =
// AdditionalInput =
// AdditionalInput =
// ReturnedBits = f80111d08e874672f32f42997133a5210f7a9375e22cea70587f9cfafebe0f6a6aa2eb68e7dd9164536d53fa020fcab20f54caddfab7d6d91e5ffec1dfd8deaa
//
// COUNT = 1
// EntropyInput = eab5a9f23ceac9e4195e185c8cea549d6d97d03276225a7452763c396a7f70bf
// Nonce =
// PersonalizationString =
// EntropyInputReseed = 4258765c65a03af92fc5816f966f1a6644a6134633aad2d5d19bd192e4c1196a
// AdditionalInputReseed =
// AdditionalInput =
// AdditionalInput =
// ReturnedBits = 2915c9fabfbf7c62d68d83b4e65a239885e809ceac97eb8ef4b64df59881c277d3a15e0e15b01d167c49038fad2f54785ea714366d17bb2f8239fd217d7e1cba
//
// COUNT = 2
// EntropyInput = 4465bf169297819160b8ef406ce768f70d094588322e8a214a8d67d55704931c
// Nonce =
// PersonalizationString =
// EntropyInputReseed = a461f049fca9349c29f4aa4909a4d15d11e4ce72747ad5b0a7b1ca6d83f88ff1
// AdditionalInputReseed =
// AdditionalInput =
// AdditionalInput =
// ReturnedBits = 1ed1079763fbe2dcfc65532d2f1db0e1ccd271a9c73b3479f16b0d3d993bc0516f4caf6f0185ecba912ebb8e42437e2016a6121459e64e82b414ba7f994a53bd
//
// COUNT = 3
// EntropyInput = 67566494c6a170e65d5277b827264da11de1bdef345e593f7a420580be8e3f7b
// Nonce =
// PersonalizationString =
// EntropyInputReseed = 737652b3a4cea2e68f28fed839994170b701aaa0fdc015a945e8ee00577a7f6e
// AdditionalInputReseed =
// AdditionalInput =
// AdditionalInput =
// ReturnedBits = e0ee86950de55281d861dc656f80bc4bbeaf8b5303e07df353f67aa63183333a437aabc400643e648f21e63809d688632e4fc8a25aa740637d812abe9eb17b5a
//
// COUNT = 4
// EntropyInput = 9ba928f88bc924a1e19ea804d7096dd6c55d9497d889fb87eafb179380f7d7a5
// Nonce =
// PersonalizationString =
// EntropyInputReseed = 76337f55d07c33c21129aa694912703e4fef8e5401185c7e7d47784e963c87a4
// AdditionalInputReseed =
// AdditionalInput =
// AdditionalInput =
// ReturnedBits = 510b18ec20120da8798ca944dfc97c63ae62266d122c70ce5cf472d5ba717dfc80a1cce0c29a8cf3d221583c7223b331727b41a0cd56d4ca425e7678441784fc
//
// COUNT = 5
// EntropyInput = eb20968b85cdabe87c6400d8b01d93c0240ace20a40bbb4996a0de6ed3c49326
// Nonce =
// PersonalizationString =
// EntropyInputReseed = c46e67b8027db6b5bac40906ad0be62759524a2f3d90a5025b188e7a850c73be
// AdditionalInputReseed =
// AdditionalInput =
// AdditionalInput =
// ReturnedBits = bd158d21c0172d5058f74d69865c98b61025683807df930bf5fc3c500c8c10c71d8804fa67db413a4a5c53d57a52aaac469698b4a42fda0eedf7b45d36078639
//
// COUNT = 6
// EntropyInput = 7b3292fed22226315b52c12e0a493eb4eda9a79498cc71985a3bd07d29e5ae04
// Nonce =
// PersonalizationString =
// EntropyInputReseed = 21317bba5c805b6e05a1137c90b6559bf1027c2a80b95d176e31a87f6ddd48b9
// AdditionalInputReseed =
// AdditionalInput =
// AdditionalInput =
// ReturnedBits = eb68b9985db9fc8654e7219c8599f42ec0164b42b5e95a087c4ee8bd8898fa69548b8c5da1af2a785f5a0149dd30c88922123d449e324c399df4b524a33e5a9d
//
// COUNT = 7
// EntropyInput = 477baac730e534f2e2525e83719802764b954acf9732e8724d856dcd124aeac7
// Nonce =
// PersonalizationString =
// EntropyInputReseed = 4461fa9e6fb6d4829c8b16cbccb14dedee9f0d6f5883748d7a90f14fef54d8cc
// AdditionalInputReseed =
// AdditionalInput =
// AdditionalInput =
// ReturnedBits = 61e5d9056d27691f4258e8844a516e979aeb49c5d9482682f914cb9b310172ed1ae1b01b241b317a59adcc9444cdd8204e49b8d917892d23725866cd31eff534
//
// COUNT = 9
// EntropyInput = 0e5585e10cedd896792e2b918b2cb0a37844b64c862c283d76c97055c88d702b
// Nonce =
// PersonalizationString =
// EntropyInputReseed = 87445a1ade002d1f0f49d64bda4c8ca427225ff56f371a20bd8a5a3bd35fc568
// AdditionalInputReseed =
// AdditionalInput =
// AdditionalInput =
// ReturnedBits = 83067b4a57a5f6ba418a98996eb102329d6bdc4e1dfb125468f1f8ab36d00732597de568c17cae3412c9ebfae08377ca19406b1abc5e10be5dbeacf3839bcf43
//
// COUNT = 10
// EntropyInput = 01d9f6246936ee6682e5cb840a394628c79d0d74c898c73cac2515ed9e05303b
// Nonce =
// PersonalizationString =
// EntropyInputReseed = e2ae7e8d2e3a182936891c066751d40dd6c92ebe146dd13d4e076591d7d63f8d
// AdditionalInputReseed =
// AdditionalInput =
// AdditionalInput =
// ReturnedBits = 1ce04a78ac2d53db46a1bb9240d47f37134ca7a2826c09ceb48d533d645bb087bfb77b18f9aafd1cf1727ad48aede207f490bf53e1e19f9f06615dd937073c11
//
// COUNT = 11
// EntropyInput = c91189669aab973c92c9a71fd68db253d2adee1cbf25bd6a4a1fa669f7d06e35
// Nonce =
// PersonalizationString =
// EntropyInputReseed = b76f3931100b658fc064a1cd21cb751d57708f71e903bf7908a80616fa7e5bcf
// AdditionalInputReseed =
// AdditionalInput =
// AdditionalInput =
// ReturnedBits = 86a59707f43f09df04d060e9ad080f2d9584dc33c8f2de9733751de4ae17da5ac93ad9f7e304390137325216f37c77a712b6756e6ffa382b63495eeb80332456
//
// COUNT = 12
// EntropyInput = b0c35bba01043398443d68dfe2c8898933ce58b98a598064b76d095c30074bf6
// Nonce =
// PersonalizationString =
// EntropyInputReseed = 02fdeb64d0973996a8a8a0629026f56cbbb91fca34b8f50ec059e746d4b20b1a
// AdditionalInputReseed =
// AdditionalInput =
// AdditionalInput =
// ReturnedBits = a3dcfd3547814b5439dd5cc6178c6632cccd81fcc34b8f9c9ceb52c23efdd18bb4873b97ade53c54824c8768df0e9987ecfa9635e1ba3944d8694f7ca8c51fac
//
// COUNT = 13
// EntropyInput = 569f3b21f1b80c6c517030f51cb81866cda24ac168a99b4e5e9634d0b16ac0a1
// Nonce =
// PersonalizationString =
// EntropyInputReseed = 0d6625c6e5102216d4e0e5e6171d8ee260cacde6bdb5b082cb9bcfe96b67986e
// AdditionalInputReseed =
// AdditionalInput =
// AdditionalInput =
// ReturnedBits = 006be6cbd866e275d97cc499813f462587f938054d733ff209d3035fde3e2d6915cf6ca3342d9064df7ac8075b3f54f87b35cd9b4ebc56835a9ea2557d8e154b
//
// COUNT = 14
// EntropyInput = 8ecdbf1cba26eae45f70ccfec0e42d6139be57f131ff60898a3b63968acf28ac
// Nonce =
// PersonalizationString =
// EntropyInputReseed = 8d860dcf67fbee47f33ed5273ff81956335d9152085f184f8427ad4234f95661
// AdditionalInputReseed =
// AdditionalInput =
// AdditionalInput =
// ReturnedBits = 8049f3fe2e62883f71cc43873b9775bf60a97c070370f9757c51488b050c00959d085ddd8f8e3702aa4cd6ff19b6c62685afb7792eb003c07bbcc9f4a026d138
//
// ============================================================

// COUNT = 0
#define NIST_CTR_DRBG_AES128_NoPR_NoDF_EntropyInput_0 0xed, 0x1e, 0x7f, 0x21, 0xef, 0x66, 0xea, 0x5d, 0x8e, 0x2a, 0x85, 0xb9, 0x33, 0x72, 0x45, 0x44, 0x5b, 0x71, 0xd6, 0x39, 0x3a, 0x4e, 0xec, 0xb0, 0xe6, 0x3c, 0x19, 0x3d, 0x0f, 0x72, 0xf9, 0xa9
#define NIST_CTR_DRBG_AES128_NoPR_NoDF_EntropyInputReseed_0 0x30, 0x3f, 0xb5, 0x19, 0xf0, 0xa4, 0xe1, 0x7d, 0x6d, 0xf0, 0xb6, 0x42, 0x6a, 0xa0, 0xec, 0xb2, 0xa3, 0x60, 0x79, 0xbd, 0x48, 0xbe, 0x47, 0xad, 0x2a, 0x8d, 0xbf, 0xe4, 0x8d, 0xa3, 0xef, 0xad
#define NIST_CTR_DRBG_AES128_NoPR_NoDF_ReturnedBytes_0 0xf8, 0x01, 0x11, 0xd0, 0x8e, 0x87, 0x46, 0x72, 0xf3, 0x2f, 0x42, 0x99, 0x71, 0x33, 0xa5, 0x21, 0x0f, 0x7a, 0x93, 0x75, 0xe2, 0x2c, 0xea, 0x70, 0x58, 0x7f, 0x9c, 0xfa, 0xfe, 0xbe, 0x0f, 0x6a, 0x6a, 0xa2, 0xeb, 0x68, 0xe7, 0xdd, 0x91, 0x64, 0x53, 0x6d, 0x53, 0xfa, 0x02, 0x0f, 0xca, 0xb2, 0x0f, 0x54, 0xca, 0xdd, 0xfa, 0xb7, 0xd6, 0xd9, 0x1e, 0x5f, 0xfe, 0xc1, 0xdf, 0xd8, 0xde, 0xaa

// COUNT = 1
#define NIST_CTR_DRBG_AES128_NoPR_NoDF_EntropyInput_1 0xea, 0xb5, 0xa9, 0xf2, 0x3c, 0xea, 0xc9, 0xe4, 0x19, 0x5e, 0x18, 0x5c, 0x8c, 0xea, 0x54, 0x9d, 0x6d, 0x97, 0xd0, 0x32, 0x76, 0x22, 0x5a, 0x74, 0x52, 0x76, 0x3c, 0x39, 0x6a, 0x7f, 0x70, 0xbf
#define NIST_CTR_DRBG_AES128_NoPR_NoDF_EntropyInputReseed_1 0x42, 0x58, 0x76, 0x5c, 0x65, 0xa0, 0x3a, 0xf9, 0x2f, 0xc5, 0x81, 0x6f, 0x96, 0x6f, 0x1a, 0x66, 0x44, 0xa6, 0x13, 0x46, 0x33, 0xaa, 0xd2, 0xd5, 0xd1, 0x9b, 0xd1, 0x92, 0xe4, 0xc1, 0x19, 0x6a
#define NIST_CTR_DRBG_AES128_NoPR_NoDF_ReturnedBytes_1 0x29, 0x15, 0xc9, 0xfa, 0xbf, 0xbf, 0x7c, 0x62, 0xd6, 0x8d, 0x83, 0xb4, 0xe6, 0x5a, 0x23, 0x98, 0x85, 0xe8, 0x09, 0xce, 0xac, 0x97, 0xeb, 0x8e, 0xf4, 0xb6, 0x4d, 0xf5, 0x98, 0x81, 0xc2, 0x77, 0xd3, 0xa1, 0x5e, 0x0e, 0x15, 0xb0, 0x1d, 0x16, 0x7c, 0x49, 0x03, 0x8f, 0xad, 0x2f, 0x54, 0x78, 0x5e, 0xa7, 0x14, 0x36, 0x6d, 0x17, 0xbb, 0x2f, 0x82, 0x39, 0xfd, 0x21, 0x7d, 0x7e, 0x1c, 0xba

// COUNT = 2
#define NIST_CTR_DRBG_AES128_NoPR_NoDF_EntropyInput_2 0x44, 0x65, 0xbf, 0x16, 0x92, 0x97, 0x81, 0x91, 0x60, 0xb8, 0xef, 0x40, 0x6c, 0xe7, 0x68, 0xf7, 0x0d, 0x09, 0x45, 0x88, 0x32, 0x2e, 0x8a, 0x21, 0x4a, 0x8d, 0x67, 0xd5, 0x57, 0x04, 0x93, 0x1c
#define NIST_CTR_DRBG_AES128_NoPR_NoDF_EntropyInputReseed_2 0xa4, 0x61, 0xf0, 0x49, 0xfc, 0xa9, 0x34, 0x9c, 0x29, 0xf4, 0xaa, 0x49, 0x09, 0xa4, 0xd1, 0x5d, 0x11, 0xe4, 0xce, 0x72, 0x74, 0x7a, 0xd5, 0xb0, 0xa7, 0xb1, 0xca, 0x6d, 0x83, 0xf8, 0x8f, 0xf1
#define NIST_CTR_DRBG_AES128_NoPR_NoDF_ReturnedBytes_2 0x1e, 0xd1, 0x07, 0x97, 0x63, 0xfb, 0xe2, 0xdc, 0xfc, 0x65, 0x53, 0x2d, 0x2f, 0x1d, 0xb0, 0xe1, 0xcc, 0xd2, 0x71, 0xa9, 0xc7, 0x3b, 0x34, 0x79, 0xf1, 0x6b, 0x0d, 0x3d, 0x99, 0x3b, 0xc0, 0x51, 0x6f, 0x4c, 0xaf, 0x6f, 0x01, 0x85, 0xec, 0xba, 0x91, 0x2e, 0xbb, 0x8e, 0x42, 0x43, 0x7e, 0x20, 0x16, 0xa6, 0x12, 0x14, 0x59, 0xe6, 0x4e, 0x82, 0xb4, 0x14, 0xba, 0x7f, 0x99, 0x4a, 0x53, 0xbd

uint8_t NIST_CTR_DRBG_AES128_NoPR_NoDF_Count = 3;
bool NIST_CTR_DRBG_AES128_NoPR_NoDF_PredictionResistance = false;
bool NIST_CTR_DRBG_AES128_NoPR_NoDF_Reseed = true;
bool NIST_CTR_DRBG_AES128_NoPR_NoDF_DerivationFunction = false;

size_t NIST_CTR_DRBG_AES128_NoPR_NoDF_EntropyInputLen = 32;
size_t NIST_CTR_DRBG_AES128_NoPR_NoDF_PersonalizationStringLen = 0;
size_t NIST_CTR_DRBG_AES128_NoPR_NoDF_AdditionalInputLen = 0;
size_t NIST_CTR_DRBG_AES128_NoPR_NoDF_ReturnedBytesLen = 64;

uint8_t NIST_CTR_DRBG_AES128_NoPR_NoDF_EntropyInput[] = {
          NIST_CTR_DRBG_AES128_NoPR_NoDF_EntropyInput_0,
	  NIST_CTR_DRBG_AES128_NoPR_NoDF_EntropyInputReseed_0,
	  NIST_CTR_DRBG_AES128_NoPR_NoDF_EntropyInput_1,
	  NIST_CTR_DRBG_AES128_NoPR_NoDF_EntropyInputReseed_1,
	  NIST_CTR_DRBG_AES128_NoPR_NoDF_EntropyInput_2,
	  NIST_CTR_DRBG_AES128_NoPR_NoDF_EntropyInputReseed_2 };
uint8_t *NIST_CTR_DRBG_AES128_NoPR_NoDF_PersonalizationString = NULL;
uint8_t *NIST_CTR_DRBG_AES128_NoPR_NoDF_AdditionalInput = NULL;
uint8_t NIST_CTR_DRBG_AES128_NoPR_NoDF_ReturnedBytes[] = {
          NIST_CTR_DRBG_AES128_NoPR_NoDF_ReturnedBytes_0,
	  NIST_CTR_DRBG_AES128_NoPR_NoDF_ReturnedBytes_1,
	  NIST_CTR_DRBG_AES128_NoPR_NoDF_ReturnedBytes_2 };

// ============================================================
// [AES-128 use df]
// [PredictionResistance = False]
// [EntropyInputLen = 128]
// [NonceLen = 64]
// [PersonalizationStringLen = 128]
// [AdditionalInputLen = 128]
// [ReturnedBitsLen = 512]
// ============================================================
//
// COUNT = 0
// EntropyInput = e796b728ec69cf79f97eaa2c06e7187f
// Nonce = 3568f011c282c01d
// PersonalizationString = b5ae693192ff057e682a629b84b8feec
// EntropyInputReseed = 31c4db5713e08e4e8cfbf777b9621a04
// AdditionalInputReseed = b6997617e4e2c94d8a3bf3c61439a55e
// AdditionalInput = c3998f9edd938286d7fad2cc75963fdd
// AdditionalInput = 648fc7360ae27002e1aa77d85895b89e
// ReturnedBits = 6ce1eb64fdca9fd3b3ef61913cc1c214f93bca0e515d0514fa488d8af529f49892bb7cd7fbf584eb020fd8cb2af9e6dbfce8a8a3439be85d5cc4de7640b4ef7d
//
// COUNT = 1
// EntropyInput = 94a799a2c352bbc824921a75db0b1590
// Nonce = f5fcd4cf35327dbe
// PersonalizationString = defffafd85c84735beaee87b3d226684
// EntropyInputReseed = e6950d5bd31c482b6d83c646d7bfab07
// AdditionalInputReseed = 903b9c0791794cf5c88248825422ab79
// AdditionalInput = 1d087bde28b66353e0261db4f99ac5f8
// AdditionalInput = 909a9f61a87a681d08441c261e33edae
// ReturnedBits = 551f8b6071e4a1bde59b606f8e3df033501c1e45a0f718b6bdf06a64fcef9cdaec65ba6089125ad0a25e617a03acc26a262fe4ef4b6460524cf6bf921108d5ff
//
// COUNT = 2
// EntropyInput = ed3f6346ca316cea24558d0f1aaa4bac
// Nonce = cffbd0ed0336fd69
// PersonalizationString = 07d6fa6941fe2a4af35b4f939c2ca89d
// EntropyInputReseed = 1dadbff4d917ba5236752de8e01e42a8
// AdditionalInputReseed = 031f2b1f3130d0db79805d9c787c1899
// AdditionalInput = b2b2eea5fa8a1881e2615b4679ec4d9c
// AdditionalInput = 00f61c3a374536f89bfccf7e43a3b04b
// ReturnedBits = 8cc6b6dbb095c87e0ede01f5e87b8aaf0eaddf43fd2dffc08439d106ebbe5173d5b3eb61f38963c19b5db1833be1442e8a5099251cc66d75773bc6fa4936732a
//
// COUNT = 3
// EntropyInput = 6b476b201b0caa270eef30b0940682a3
// Nonce = a1d0954c17167b17
// PersonalizationString = 654279a6972d0f18ca990fb9e87f4089
// EntropyInputReseed = f4367480086b4822a3b54dd5b1f4d310
// AdditionalInputReseed = de6e784068b051f60427a3f49dde446a
// AdditionalInput = a6a5d61d9697d93364e7e550a93cf7d7
// AdditionalInput = a4e948811c4555002062f5e76e892ffc
// ReturnedBits = 332a3868362afee036e8073f1c8391ffca33c724325aa3f66c797152b9978eabe8319229f8ae3e523f03b147a87bee278fa33551d5f30428b56bb01e3b3c1cf3
//
// COUNT = 4
// EntropyInput = 70cad557defaa0c8b37c167c9723dbc7
// Nonce = dbe89a106f15d112
// PersonalizationString = 846cdb83872be7900104c14c1f88dd6b
// EntropyInputReseed = f04c519d94a0ab7eaf3544ecd5f85b95
// AdditionalInputReseed = 9fbd72ce332abff5f74666d7e64f5acd
// AdditionalInput = b07687adbaef6b1141aebd9e78358474
// AdditionalInput = 5398f31a34af720d2b2e30f65b8b0567
// ReturnedBits = 0f29f6b207c382d3524403de0edde3bdca56ab081013742b40c923a3e3c95b9c3b361c1cff27bf2d2e60d81f3b4506f88ee558f77f7248ae3a68ac291f0021e8
//
// COUNT = 5
// EntropyInput = 33ba494b5491845103b57aaacef0a65d
// Nonce = 55385b3ea7f3b4ac
// PersonalizationString = 9247dda03f2c310793f3e5ec5d07d397
// EntropyInputReseed = 87895ed43af734518991197748198c0d
// AdditionalInputReseed = de0eccbfdabbaeb7f9c52580490ceb87
// AdditionalInput = 6dea3b1bee14666ed053f0b512565316
// AdditionalInput = 1e3d49df6e079f9c8e79260c63127445
// ReturnedBits = 139d256f1ce3e4f2b6bf086d809faf19820e799f744f7c36580cf82419caef4de76e5110e11ca475604fc04a55fe3b0713cf191a6dfb9d33ded0d5c61a6ba160
//
// COUNT = 6
// EntropyInput = 26e3a50f73a60698b94ad39315a66ada
// Nonce = 6e53c2baa71745ca
// PersonalizationString = c47beb1a590e06707722e3f194fa5cee
// EntropyInputReseed = 618151421345f7400ace77eb7fa4b0c8
// AdditionalInputReseed = fd7ebb1ea4e76d8e5228cdff7786a52a
// AdditionalInput = 39e8c578b7c924c56afcdffea37fed4d
// AdditionalInput = 2483b96f2190ed57b4a68c7cd04a42cf
// ReturnedBits = 948c0ae18f989d2de0700b5ff1cfb539b914676ecb4e516f4fa037699b181404a300513bbb9af3f55553a6be6e894196485736e753432bfa679d07bb6c5bcda8
//
// COUNT = 7
// EntropyInput = b7516865da6b6b494e6b33b278fba588
// Nonce = c66f5d699f36981d
// PersonalizationString = bc6431500495b2cf8f910537b29e86c1
// EntropyInputReseed = 4ef07edec95e662ad8ec1ae30c7739f2
// AdditionalInputReseed = 5db27541e3b6dc4830e7afb16b267e42
// AdditionalInput = 5a4ba4f6a1a76d21a4161204d582cd25
// AdditionalInput = d35def58fb9c13f26f13db4f44f843c8
// ReturnedBits = bc5a174b9af4c1b9b377b320a93bce848da3106a147ca6b8842e020ce3ad5b79d26f7adf721005183a1b53ce0764a7d4738ddb97cdcd0d6a76a32dbfa496d2cb
//
// COUNT = 8
// EntropyInput = bc0807f6cdae0e32b1bb0ac0b4028119
// Nonce = 76f5694b0c852d4b
// PersonalizationString = 32494822002c74dc406b88a213e26d46
// EntropyInputReseed = 16d460237a5f38a649e68fc111c1ef56
// AdditionalInputReseed = ee95f184344bfb0455fa7f4db2326d45
// AdditionalInput = be4b332e3cf20a868ba990eef3f8212f
// AdditionalInput = 4345acd433eec510afde2faf2c51dc9a
// ReturnedBits = c7b57b7e6deeb13288b48fc1775c0eb122811ce27e336dabdce3fa23db316882ede6938a441ffe848d82c0a1a37a1e8bbf12ca3bb8b2265cb5b3b335d4c79f14
//
// COUNT = 9
// EntropyInput = b54314023cb882d759e74fd822d61495
// Nonce = b33ad7169c3264c0
// PersonalizationString = 89bf26190eafec317dae9949ff79aa20
// EntropyInputReseed = 490088225d2a81641bc0147d108a7925
// AdditionalInputReseed = 470b3f0aa0f280599fe938cd5aa1b9bd
// AdditionalInput = 32f3b0f8610b2fe6549409a742ae1638
// AdditionalInput = c837682d0a19db10efc34a44478512e2
// ReturnedBits = 99ab5fa618bc7bd1521bf3d05db3cd082724836d2913619d18d07ac47d57f05f4df2ebe2bd22de0487ac6cb9bab758f9f49ebf1afdf7cf83e8ae3b5fb69d629f
//
// COUNT = 10
// EntropyInput = b3cb56ce6d01eac4583866fdf2a0cce6
// Nonce = 130d06c35a72a870
// PersonalizationString = 5de4b0c6c71f9ad91df2ee4ca4cade6e
// EntropyInputReseed = 7a916fbc9f003948551b1c7e61c5d126
// AdditionalInputReseed = 637d0c6ef199c8e3e309387a55ea3986
// AdditionalInput = 84b3aa93f84447e2d3792d2f5fe0ea61
// AdditionalInput = a045f50aa17c94d1f55c55d3dd705932
// ReturnedBits = 227e06771d658094c824cc4bbc1a985058c5afa37106f3f0fed370df64bb22651151e3332c602d0a36486c70ed7fd1f5af6b52c3fbd28b8a31032bb10f79df5d
//
// COUNT = 11
// EntropyInput = 7f249f2b0af7f6d2accc534486ff25d4
// Nonce = b4a5eb9f06227580
// PersonalizationString = 260f24cbf2c66c31c878ba867a4704c0
// EntropyInputReseed = 9ba292c29226af77342ca46ca80d0320
// AdditionalInputReseed = 91a77c7f132f210351bb51c6c7327f6f
// AdditionalInput = 3361b46fc051204302264b97b8547707
// AdditionalInput = 06478fa687465e759420eaac2c7d0add
// ReturnedBits = 2605bbd7d0ef8e2b3d8c579eb9f0602cb035cfd5e466e4d5a1e40f9288bb06dc52896ce341ab39e216ce7b8b70d62e1ce07bbc0a31172fd191f400fd2bfafd47
//
// COUNT = 12
// EntropyInput = 7c5d54816513a318b33ac67ed7144d4c
// Nonce = 52bec78faea5e1f6
// PersonalizationString = 8a2f62b83307bc52e7cfa4cf81736efa
// EntropyInputReseed = 192f6531617ec4baef6a302e18fe8a16
// AdditionalInputReseed = f2aca1e5c0017708fc7814cb748b0979
// AdditionalInput = ff68c19402e0cbae1a67f98ffd62f25e
// AdditionalInput = 2e89134c16b5da0f572a72e8f72bad03
// ReturnedBits = 959d505f742d5cfd4e5767ab75f41a6a97865b6a48918a68c9155d9ee56143963c5fe0200eb3f73e234ddb0df4bfe9a96b59c8d3dbc324d49a01e0113bfc0eae
//
// COUNT = 13
// EntropyInput = 59870c273d6d94a6a8a38a2c63ba2882
// Nonce = 16767b2036076301
// PersonalizationString = 1e4c01af969528849ffafb17044bbcd9
// EntropyInputReseed = 4d8353750de15bbc510ee89c56d9b2cf
// AdditionalInputReseed = 58d49d610473cd1a0c022e338e45c9c7
// AdditionalInput = 2bc01029d8195d54f8ace2352c4bf156
// AdditionalInput = dfdcb78bcda77cf0a636de21849babc9
// ReturnedBits = dd24c835595ff642fc2f2379e35f5cdd7495783848613564d9b2bef962504f2c7607505e09122d63a2aa0678b95f4d3c3dd3c5f85f1a1a0f559eef23db002fd7
//
// COUNT = 14
// EntropyInput = 03af6c44c7101544ca7888831f3f9f95
// Nonce = 58d80fc378569346
// PersonalizationString = 7ec2b597c845ae8bbb63609a80c2ab4f
// EntropyInputReseed = 410737c3c6112b5113f0ef66b0fa6a07
// AdditionalInputReseed = ae2ce6d1dba56775db17b8c6d9379f14
// AdditionalInput = 57866375238ea56e97dd6af8c5010618
// AdditionalInput = 34a449f860c0c18fdf854b32de572917
// ReturnedBits = 564bd3b77e4c3bdcb6c85db111f294293a4738d20043da15649b8c94f14ecdc73fb75d761971f3a8dc4f33f45df97c0d2bba4e4be74f52887325b300ff1e6d81
//
// ============================================================

// COUNT = 0
#define NIST_CTR_DRBG_AES128_NoPR_UseDF_EntropyInput_0 0xe7, 0x96, 0xb7, 0x28, 0xec, 0x69, 0xcf, 0x79, 0xf9, 0x7e, 0xaa, 0x2c, 0x06, 0xe7, 0x18, 0x7f
#define NIST_CTR_DRBG_AES128_NoPR_UseDF_Nonce_0 0x35, 0x68, 0xf0, 0x11, 0xc2, 0x82, 0xc0, 0x1d
#define NIST_CTR_DRBG_AES128_NoPR_UseDF_PersonalizationString_0 0xb5, 0xae, 0x69, 0x31, 0x92, 0xff, 0x05, 0x7e, 0x68, 0x2a, 0x62, 0x9b, 0x84, 0xb8, 0xfe, 0xec
#define NIST_CTR_DRBG_AES128_NoPR_UseDF_EntropyInputReseed_0 0x31, 0xc4, 0xdb, 0x57, 0x13, 0xe0, 0x8e, 0x4e, 0x8c, 0xfb, 0xf7, 0x77, 0xb9, 0x62, 0x1a, 0x04
#define NIST_CTR_DRBG_AES128_NoPR_UseDF_AdditionalInputReseed_0 0xb6, 0x99, 0x76, 0x17, 0xe4, 0xe2, 0xc9, 0x4d, 0x8a, 0x3b, 0xf3, 0xc6, 0x14, 0x39, 0xa5, 0x5e
#define NIST_CTR_DRBG_AES128_NoPR_UseDF_AdditionalInput1_0 0xc3, 0x99, 0x8f, 0x9e, 0xdd, 0x93, 0x82, 0x86, 0xd7, 0xfa, 0xd2, 0xcc, 0x75, 0x96, 0x3f, 0xdd
#define NIST_CTR_DRBG_AES128_NoPR_UseDF_AdditionalInput2_0 0x64, 0x8f, 0xc7, 0x36, 0x0a, 0xe2, 0x70, 0x02, 0xe1, 0xaa, 0x77, 0xd8, 0x58, 0x95, 0xb8, 0x9e
#define NIST_CTR_DRBG_AES128_NoPR_UseDF_ReturnedBytes_0 0x6c, 0xe1, 0xeb, 0x64, 0xfd, 0xca, 0x9f, 0xd3, 0xb3, 0xef, 0x61, 0x91, 0x3c, 0xc1, 0xc2, 0x14, 0xf9, 0x3b, 0xca, 0x0e, 0x51, 0x5d, 0x05, 0x14, 0xfa, 0x48, 0x8d, 0x8a, 0xf5, 0x29, 0xf4, 0x98, 0x92, 0xbb, 0x7c, 0xd7, 0xfb, 0xf5, 0x84, 0xeb, 0x02, 0x0f, 0xd8, 0xcb, 0x2a, 0xf9, 0xe6, 0xdb, 0xfc, 0xe8, 0xa8, 0xa3, 0x43, 0x9b, 0xe8, 0x5d, 0x5c, 0xc4, 0xde, 0x76, 0x40, 0xb4, 0xef, 0x7d

// COUNT = 1
#define NIST_CTR_DRBG_AES128_NoPR_UseDF_EntropyInput_1 0x94, 0xa7, 0x99, 0xa2, 0xc3, 0x52, 0xbb, 0xc8, 0x24, 0x92, 0x1a, 0x75, 0xdb, 0x0b, 0x15, 0x90
#define NIST_CTR_DRBG_AES128_NoPR_UseDF_Nonce_1 0xf5, 0xfc, 0xd4, 0xcf, 0x35, 0x32, 0x7d, 0xbe
#define NIST_CTR_DRBG_AES128_NoPR_UseDF_PersonalizationString_1 0xde, 0xff, 0xfa, 0xfd, 0x85, 0xc8, 0x47, 0x35, 0xbe, 0xae, 0xe8, 0x7b, 0x3d, 0x22, 0x66, 0x84
#define NIST_CTR_DRBG_AES128_NoPR_UseDF_EntropyInputReseed_1 0xe6, 0x95, 0x0d, 0x5b, 0xd3, 0x1c, 0x48, 0x2b, 0x6d, 0x83, 0xc6, 0x46, 0xd7, 0xbf, 0xab, 0x07
#define NIST_CTR_DRBG_AES128_NoPR_UseDF_AdditionalInputReseed_1 0x90, 0x3b, 0x9c, 0x07, 0x91, 0x79, 0x4c, 0xf5, 0xc8, 0x82, 0x48, 0x82, 0x54, 0x22, 0xab, 0x79
#define NIST_CTR_DRBG_AES128_NoPR_UseDF_AdditionalInput1_1 0x1d, 0x08, 0x7b, 0xde, 0x28, 0xb6, 0x63, 0x53, 0xe0, 0x26, 0x1d, 0xb4, 0xf9, 0x9a, 0xc5, 0xf8
#define NIST_CTR_DRBG_AES128_NoPR_UseDF_AdditionalInput2_1 0x90, 0x9a, 0x9f, 0x61, 0xa8, 0x7a, 0x68, 0x1d, 0x08, 0x44, 0x1c, 0x26, 0x1e, 0x33, 0xed, 0xae
#define NIST_CTR_DRBG_AES128_NoPR_UseDF_ReturnedBytes_1 0x55, 0x1f, 0x8b, 0x60, 0x71, 0xe4, 0xa1, 0xbd, 0xe5, 0x9b, 0x60, 0x6f, 0x8e, 0x3d, 0xf0, 0x33, 0x50, 0x1c, 0x1e, 0x45, 0xa0, 0xf7, 0x18, 0xb6, 0xbd, 0xf0, 0x6a, 0x64, 0xfc, 0xef, 0x9c, 0xda, 0xec, 0x65, 0xba, 0x60, 0x89, 0x12, 0x5a, 0xd0, 0xa2, 0x5e, 0x61, 0x7a, 0x03, 0xac, 0xc2, 0x6a, 0x26, 0x2f, 0xe4, 0xef, 0x4b, 0x64, 0x60, 0x52, 0x4c, 0xf6, 0xbf, 0x92, 0x11, 0x08, 0xd5, 0xff

// COUNT = 2
#define NIST_CTR_DRBG_AES128_NoPR_UseDF_EntropyInput_2 0xed, 0x3f, 0x63, 0x46, 0xca, 0x31, 0x6c, 0xea, 0x24, 0x55, 0x8d, 0x0f, 0x1a, 0xaa, 0x4b, 0xac
#define NIST_CTR_DRBG_AES128_NoPR_UseDF_Nonce_2 0xcf, 0xfb, 0xd0, 0xed, 0x03, 0x36, 0xfd, 0x69
#define NIST_CTR_DRBG_AES128_NoPR_UseDF_PersonalizationString_2 0x07, 0xd6, 0xfa, 0x69, 0x41, 0xfe, 0x2a, 0x4a, 0xf3, 0x5b, 0x4f, 0x93, 0x9c, 0x2c, 0xa8, 0x9d
#define NIST_CTR_DRBG_AES128_NoPR_UseDF_EntropyInputReseed_2 0x1d, 0xad, 0xbf, 0xf4, 0xd9, 0x17, 0xba, 0x52, 0x36, 0x75, 0x2d, 0xe8, 0xe0, 0x1e, 0x42, 0xa8
#define NIST_CTR_DRBG_AES128_NoPR_UseDF_AdditionalInputReseed_2 0x03, 0x1f, 0x2b, 0x1f, 0x31, 0x30, 0xd0, 0xdb, 0x79, 0x80, 0x5d, 0x9c, 0x78, 0x7c, 0x18, 0x99
#define NIST_CTR_DRBG_AES128_NoPR_UseDF_AdditionalInput1_2 0xb2, 0xb2, 0xee, 0xa5, 0xfa, 0x8a, 0x18, 0x81, 0xe2, 0x61, 0x5b, 0x46, 0x79, 0xec, 0x4d, 0x9c
#define NIST_CTR_DRBG_AES128_NoPR_UseDF_AdditionalInput2_2 0x00, 0xf6, 0x1c, 0x3a, 0x37, 0x45, 0x36, 0xf8, 0x9b, 0xfc, 0xcf, 0x7e, 0x43, 0xa3, 0xb0, 0x4b
#define NIST_CTR_DRBG_AES128_NoPR_UseDF_ReturnedBytes_2 0x8c, 0xc6, 0xb6, 0xdb, 0xb0, 0x95, 0xc8, 0x7e, 0x0e, 0xde, 0x01, 0xf5, 0xe8, 0x7b, 0x8a, 0xaf, 0x0e, 0xad, 0xdf, 0x43, 0xfd, 0x2d, 0xff, 0xc0, 0x84, 0x39, 0xd1, 0x06, 0xeb, 0xbe, 0x51, 0x73, 0xd5, 0xb3, 0xeb, 0x61, 0xf3, 0x89, 0x63, 0xc1, 0x9b, 0x5d, 0xb1, 0x83, 0x3b, 0xe1, 0x44, 0x2e, 0x8a, 0x50, 0x99, 0x25, 0x1c, 0xc6, 0x6d, 0x75, 0x77, 0x3b, 0xc6, 0xfa, 0x49, 0x36, 0x73, 0x2a

uint8_t NIST_CTR_DRBG_AES128_NoPR_UseDF_Count = 3;
bool NIST_CTR_DRBG_AES128_NoPR_UseDF_PredictionResistance = false;
bool NIST_CTR_DRBG_AES128_NoPR_UseDF_Reseed = true;
bool NIST_CTR_DRBG_AES128_NoPR_UseDF_DerivationFunction = true;

size_t NIST_CTR_DRBG_AES128_NoPR_UseDF_EntropyInputLen = 16;
size_t NIST_CTR_DRBG_AES128_NoPR_UseDF_PersonalizationStringLen = 16 + 8;
size_t NIST_CTR_DRBG_AES128_NoPR_UseDF_AdditionalInputLen = 16;
size_t NIST_CTR_DRBG_AES128_NoPR_UseDF_ReturnedBytesLen = 64;

uint8_t NIST_CTR_DRBG_AES128_NoPR_UseDF_EntropyInput[] = {
          NIST_CTR_DRBG_AES128_NoPR_UseDF_EntropyInput_0,
	  NIST_CTR_DRBG_AES128_NoPR_UseDF_EntropyInputReseed_0,
	  NIST_CTR_DRBG_AES128_NoPR_UseDF_EntropyInput_1,
	  NIST_CTR_DRBG_AES128_NoPR_UseDF_EntropyInputReseed_1,
	  NIST_CTR_DRBG_AES128_NoPR_UseDF_EntropyInput_2,
	  NIST_CTR_DRBG_AES128_NoPR_UseDF_EntropyInputReseed_2 };
uint8_t NIST_CTR_DRBG_AES128_NoPR_UseDF_PersonalizationString[] = {
          NIST_CTR_DRBG_AES128_NoPR_UseDF_Nonce_0,
	  NIST_CTR_DRBG_AES128_NoPR_UseDF_PersonalizationString_0,
          NIST_CTR_DRBG_AES128_NoPR_UseDF_Nonce_1,
	  NIST_CTR_DRBG_AES128_NoPR_UseDF_PersonalizationString_1,
          NIST_CTR_DRBG_AES128_NoPR_UseDF_Nonce_2,
	  NIST_CTR_DRBG_AES128_NoPR_UseDF_PersonalizationString_2 };
uint8_t NIST_CTR_DRBG_AES128_NoPR_UseDF_AdditionalInput[] = {
          NIST_CTR_DRBG_AES128_NoPR_UseDF_AdditionalInputReseed_0,
          NIST_CTR_DRBG_AES128_NoPR_UseDF_AdditionalInput1_0,
	  NIST_CTR_DRBG_AES128_NoPR_UseDF_AdditionalInput2_0,
          NIST_CTR_DRBG_AES128_NoPR_UseDF_AdditionalInputReseed_1,
	  NIST_CTR_DRBG_AES128_NoPR_UseDF_AdditionalInput1_1,
	  NIST_CTR_DRBG_AES128_NoPR_UseDF_AdditionalInput2_1,
          NIST_CTR_DRBG_AES128_NoPR_UseDF_AdditionalInputReseed_2,
	  NIST_CTR_DRBG_AES128_NoPR_UseDF_AdditionalInput1_2,
	  NIST_CTR_DRBG_AES128_NoPR_UseDF_AdditionalInput2_2 };
uint8_t NIST_CTR_DRBG_AES128_NoPR_UseDF_ReturnedBytes[] = {
          NIST_CTR_DRBG_AES128_NoPR_UseDF_ReturnedBytes_0,
	  NIST_CTR_DRBG_AES128_NoPR_UseDF_ReturnedBytes_1,
	  NIST_CTR_DRBG_AES128_NoPR_UseDF_ReturnedBytes_2 };

// ============================================================
// [AES-128 no df]
// [PredictionResistance = True]
// [EntropyInputLen = 256]
// [NonceLen = 0]
// [PersonalizationStringLen = 256]
// [AdditionalInputLen = 256]
// [ReturnedBitsLen = 512]
// ============================================================
//
// COUNT = 0
// EntropyInput = 0bfde8b32c0adabd84e271d19b76cb9695e1f89a0813e63e7d9e6a0dd1691c55
// Nonce =
// PersonalizationString = f345ffd4138ea0b41e1b87a678e24ad87c14fade68e703b359caaf2e269e7c58
// AdditionalInput = 68388f4c45c349f076c31ebb972c31d25895dfadf3a8d8c336afacf3e3108caf
// EntropyInputPR = 2a046cb477afd54271b0f5d6924c706c4043702a5657e99b884bbcd7ec6aa267
// AdditionalInput = c8a95bcab76358608db1f66ea8b4537f3dd870e38df41bcfdf41d73eb5901c93
// EntropyInputPR = d142dfb39b02511f933120a88c2fabb88e824a916c826b20f444c62e9d9af0b9
// ReturnedBits = eda204a7c51b29a79ca1a7148dae6445b4c7d830410de131c5c4a28e85c74a15c768167dac0bba1cacc3dffdb25312f9186fdfc9a6e570081f41c997ab829524
//
// COUNT = 1
// EntropyInput = 3ac67c4f1926bfe80e8ad2931b6e6f203589316b7ca152dd00fbd0681f231f58
// Nonce =
// PersonalizationString = 6eedd4b2a1e41bfc470c790685f976d287a880f15f1ab893d87540d56772bdca
// AdditionalInput = ffff55b71b1ca8ad44b7d5011c1e685c88179783df0c555172f20189acdcb234
// EntropyInputPR = 8992b1092dc4e3db41f6ddabb1eb86d42c90c1ea9673646c6d69a8dc0cd9284f
// AdditionalInput = 7635a57b4f40add8fed1032e761a962c9fe2c6a9f01023884f502a0494c60f18
// EntropyInputPR = 9904cf4e3d810a2bec3a3009735e89a45a259eb372aa3da3ff0d53e2478b228b
// ReturnedBits = 8eeb30b024559e6c9cc3a6247f7d1184c11a5e6811a2dccebd6c3bedcceb8cb4eda156eaac309201a162ca2311b13c7fc05f0652b08507c2ce57c4bd819b4a2f
//
// COUNT = 2
// EntropyInput = 0e19a3e98ec9ccb8da195b61f41842c95c14e693749e55f5fc0e55ff63f74e68
// Nonce =
// PersonalizationString = 270883545ab41cf426f9a96752172a71abfc797552b3edd463bbb7231833552d
// AdditionalInput = 1078f54e5b60fd70b48b1b70d1d0b4d17549c3e6b0becfac8a7a4517b4d2f684
// EntropyInputPR = 82b68c3fd2e8750c2bfd834dd37b6a471c30f6581a93d762184f617825b2dd83
// AdditionalInput = 52dce3796b792a58878b370e789e3cdd13c39ee9361e0d0d8fe3bb33822e60c4
// EntropyInputPR = 47cd9dc710f07bb6f5eab81dad0e28a6ccaa8b745aca2249d49f3b4ae34f2375
// ReturnedBits = 99270ab2f44d0cc1e6c1888ad5bc638b28432559087ada30f63b37242c848c8fe048bf275bc546bee8283dd66b6faa3060e43423152021c7c36a8932528f5557
//
// COUNT = 3
// EntropyInput = cc03947619ebda1a32ff0da1ab50322d914a8c928f22171c5f5794f047c1b57c
// Nonce =
// PersonalizationString = 9b58fd044d90993a74c21b3dd39e478a671ed36ea7657124135859c271ad61d3
// AdditionalInput = 558e7e980536abcedb6c1654a965b3d078c26efdab1d1c6d8a5824303e8980e3
// EntropyInputPR = 3abdb22160ef499bccbd18163084fe44520682c21072d2b5a6aff25a282aadab
// AdditionalInput = c8599133dc1d2fb1e7b786c66c99365e49eb46615bdd4fdadbe5849af6ae1ae9
// EntropyInputPR = 8554af8eb31798fe3dc155a19f61dd68565e068daaef08f2546f4aeb461bf2b4
// ReturnedBits = 9d49501dd807ba3a98d5073ceb2ff14a5aa5d095b5ae0db50f74cf97e089cb1e4cb6eb594ec860ed764567a2eef70a38d3d243ab471ae973e293af10d602354f
//
// COUNT = 4
// EntropyInput = 8980744dac51876f032011c959b130fd633a230e5a5be1bb61467063a6c3edf2
// Nonce =
// PersonalizationString = 55742e9d45daafb4ee2d447e3c63e51e6f15b08f457843757117b68d4dc8b945
// AdditionalInput = a27adc9a12e0a03a5001e0184a63866895e3cd62b72852c3c1bb0918fbaff768
// EntropyInputPR = 2c11c236690bdeec275b8fec245978caf87ce36fe9b5b89c2c0c2d14db799b5d
// AdditionalInput = f38a3b202302e2a5f96e44c20ab91486e507a23ab5d7bc683ec64e4974c2424a
// EntropyInputPR = 4245a850fbfadb2e367142ae7df0b472c508b47e69423a83272d7eda6d721e1c
// ReturnedBits = 5c9329e786e0db03cb33b2d6c40f7bb0e2143030224e3f18e5eb43bbf73f8addaa140a32f40448d6d80d65828d9205e839ef460f74f0423e4c7b5e0c498cd1a8
//
// COUNT = 5
// EntropyInput = 610943e4b29745c87f045a45d3824308de7c452ffe27f07751e73191f275d72b
// Nonce =
// PersonalizationString = f404798a263e6cb7a4926b13f79eca7e300397ceb8b60922644a8a29d9962179
// AdditionalInput = 125507e6afab4b1c37ae637e7deb4a5350f8b23ffb786f2c53c8c398a22e8a14
// EntropyInputPR = 77c5bf631060f6e6e138bfcbf9343c428a910aea968a3c1d7ee1b04057828975
// AdditionalInput = 9746d7cd3f03a0b60cc3cd91560e05fc808ecb517ce57fdc196136ee5edcbee6
// EntropyInputPR = 2da9a87dedc5c120aa178dbe46a256121eae552468fecffe85733d4034b438d3
// ReturnedBits = be8eb3e4c980c2be06f8224bab24d6e6e5e373c7e2d8042476438f2507a0051d4da26970020a8e04d16b9373a2ceb46fda2d3e0aa0ed0eea2203c956276f36c7
//
// COUNT = 6
// EntropyInput = eebd8f787b465e82d645ee251ef49ab2432a8b918f4b7b98ac6cd9fb91331310
// Nonce =
// PersonalizationString = faeb92fc4ae073a7048d31cea517da5e3508001d37bbb6258bb78f6911326701
// AdditionalInput = d76b086fe59e22643413473460fea2ed4710bf18d4e18acbf516a03c90b41294
// EntropyInputPR = 58c40d6e41a178ad01a0d1f39d57e1cc851533b0c8b3bcd5813079287f9856e2
// AdditionalInput = 295d0d828b6e6effde05ab3bc9ce584e4e45cf9ba5025f390b709e3712434c1c
// EntropyInputPR = 4069c9097c3009c6dc7a040c59491ae083cb263b063d2676ab43bd039a78d3fb
// ReturnedBits = 26087547ae8a9791892eccddb646775c6de55fb1748fb5737f564a26eb6bca58dc80f8b8f42cdf918c35ae0f9aed508afa016c49dfd410860b7024c51eaeff83
//
// COUNT = 7
// EntropyInput = db921a905c58c845333a52f08caea97d47190e91902532f36ddc87938930d8cb
// Nonce =
// PersonalizationString = c63256f1c8f9aaa75356e9c7102ec960f07545a1b98025943b7883d04ad636c8
// AdditionalInput = 44bca1be19c6a5211fc9e0a19ea429beef235d24f5f513fccdbd8d76f1d6bc31
// EntropyInputPR = 4be83ab9f7cd8e06cb76c52580e23ceba70e99a8d98cdb006bede22dc80dfeb4
// AdditionalInput = 7e0378d475191a405f0f6f411d8e5ba8a8c75679dc918a390390ddd58478d6f0
// EntropyInputPR = 02a482fe9c1edd21cb1ef24ccba26508a21a0a15dc53d104352c9d3866984e4e
// ReturnedBits = 7df6cf442ee42ac3469807d01eb95004dacdaad20d26bd660fb7fb2b45c470f0e1c740e157b72f69aac6058f391f1aed378a5396356aea080e42098cba24b82f
//
// COUNT = 8
// EntropyInput = 567d90544230c7df900054dcfd6543a8f10bc35f4d71cb9b5739c928c64fe364
// Nonce =
// PersonalizationString = aa9afe7228348faa3e8175be71ecf31adf32b5860fdf4f68c9a03005ea1308a6
// AdditionalInput = 30866b52c589092d64d42e9903325bd622741ba211049473b008e015a4034846
// EntropyInputPR = 620d7f08e90365d7b218bbf950fd330cb312919fbcb719a507321c903b11e001
// AdditionalInput = a5c950c0bb9c3e4a189def6fa0faf2d210e0ea5b2719ac50e6298653534c8ae1
// EntropyInputPR = 1b09914dd7136be0922900b555980103dc24a286d1e4a51858a5af49031719db
// ReturnedBits = dfcbf4f98ece17c789cc7136c6cd3d62e5cdee1ab71c0f68f4a63d623b28d4f0f019606c5f42ac19eb21e8eac587b135496ae34d0d55898ece9cfa6c93a4d1dc
//
// COUNT = 9
// EntropyInput = c88ddcbd3a90e7690b344d473f7450f9c2d67fb1cd162d0d9a0beba5dd6429cf
// Nonce =
// PersonalizationString = 5aa96783856e229ce715637c7c9de402cb2978f842cbe30500147c9540470792
// AdditionalInput = 057b196c2c86b71e41ee58895a740d619722395d16be1323869272f6f107692f
// EntropyInputPR = 238102d759e017f8abf37358384ad49cbaab74ef3885c0c007fbce23348836cf
// AdditionalInput = c85af17f50264fb2dae70a3e52ffcdb0f3f85d13bf0472149136f15171c09dca
// EntropyInputPR = e9b81b2a767726a671ecc7f3a2a6867aff7b966064b31664273c02e8604915a0
// ReturnedBits = 14761b40c787425e2edf316d4b2da5aa7bf90ec87f1e12404ff1e86909342ea54e647e529ce3e47a876907dc9ab7df2985d5d0fbb541a24abb603c1bda90a98e
//
// COUNT = 10
// EntropyInput = 7d45c57c977442bfd940c1145b156bf93ab4a48a49509d08c19a97b12333f195
// Nonce =
// PersonalizationString = 0f85885f09e033d927295f17e338fd23a7f2712986227416faa0f447f566c87d
// AdditionalInput = ecef280b94fb1eca947941534fe6d9ee1d7ce8921ad5cf76c72a80b5d571686b
// EntropyInputPR = 4856945fb087f0a876a2cfc266461863ad5611a260aaa619145883f5f1882f4b
// AdditionalInput = 9640c1ef766c6ddf7e283766cceb0219b44cd4061f2c215c38a2bdecda74e768
// EntropyInputPR = 022001c425bb74653dcc968f79b6d142473ffc45294ccd08821925fe0e7b59f3
// ReturnedBits = 9747678c0bfce824484481483fbfb36e52b42bb61c7b9e81b8646cb961a5e23373cd1b381173e898f5985d6b8807e854afb7f9908b86b59f10ab7e5f40d995bf
//
// COUNT = 11
// EntropyInput = faceb477255e51951adfd7ed0eb99e391dc77785ea7c03a150047ff736382e86
// Nonce =
// PersonalizationString = eed0f3a4c9b7f12e46dfa775ecf848d963a1af097fa79c9d31d346d70bcecc44
// AdditionalInput = ab5891d66a2841d2df0fb66ea793f25069b3d3af08a5e07646bd4b1c53e41cc6
// EntropyInputPR = 233d42aa4a3a00b11b53bcab65749b8fc4b5dd7e6320732ec53654c3a8974fed
// AdditionalInput = 332fa3940adb217e5e66b1e7dcb0e15f06559373aed4b75bf6bb5b78d017ba6f
// EntropyInputPR = dac6cb6b3c8711f6253d0fdd20ddc740f860287a9f5fca78e362b583a7753f38
// ReturnedBits = 77b2e6d19eee7e2456e36014c4e9d23771cdaf6d173de5e7fd5eb5c2507856d27c454e823b9842dad157816e03e22f8728cee6b0fd2ef14fd4249e836f32ffa9
//
// COUNT = 12
// EntropyInput = be34f76ff2579aeea3a8e334424cf6fd212a356e60519ebe03e58ea0702052a3
// Nonce =
// PersonalizationString = 67dabd1f3a54383631f7aafabe446025aef8de21c9f200f15423bbd9b3bbcf02
// AdditionalInput = ff4eb8c00dec61f3e3fd1b26083190455f893121fb9785532fa33c1fe374bbb1
// EntropyInputPR = 49cd2b74eeea27523ae613e1bf000339fcbea5d912333d83cf416cb0cff74a5d
// AdditionalInput = a0f18ddd517865d2630321a41084d79dbd20255dba5b8691632e85deb3e0c072
// EntropyInputPR = 26d65b73fee9e6b26e3fba4ec5bfc08fa077f9c475ff68c6707f4876c821bc9e
// ReturnedBits = 49cdcec1d65b0e03ac4d19216ea45ac31f76bac7d630e1018e7a5b9d81ff20ac159110a1f93d3682052a0793e1923e59e503defbbed3118682d757d02227c4bc
//
// COUNT = 13
// EntropyInput = 81c47b1afb876f71159b3c7da5879099e36f72cd390b5f916831fb7d9a33cc8b
// Nonce =
// PersonalizationString = 05a24c4eef872ecc5909e464e0b9fd6b171a769b5c76a027114a0d11b302c3a8
// AdditionalInput = a66fa6277d8396d498ac82518aae10e67d961e679e5a89efd818f0811f4a28e6
// EntropyInputPR = b68332d2e1600022f33e3ddb8b7a5846bbde26b0eb12c63b394dd4dfc1ef187a
// AdditionalInput = adacffc945a621e6185d81c28ee0c4f71f5ecfb1122b43321c7778259375cc9f
// EntropyInputPR = c39204b1f056234d37e49dfe406318cf4177cce6d172c3f82add7f8a2f9bd355
// ReturnedBits = 44c52b3bde4f0ead733d15490d6fd9e2e9f71052563a62b5a13f871039c7a64691636b7c0b4aa9e8a527c71516ce9d9c9692d43e845d384daa172ff541e7996b
//
// COUNT = 14
// EntropyInput = 085daac12b2401e3ff884f03bff70763c38eae65bd24b5733c6bfd545011c69e
// Nonce =
// PersonalizationString = 6a9f2e3142a2808232c3fed7efd33436a5a4d9ef14815638c65d012e11470cc8
// AdditionalInput = 6d18d7d0ed6798bb4fd7fab8d2ab6d2d72997710700661913d05fcbc5d6d8652
// EntropyInputPR = db22d9a3f53fc739d30255a6617c986254153c519c34e79d2a6e8762a06909b0
// AdditionalInput = b44a499fdf9330170ebede64cf8fb19f4a8317596d80d8f9c9d100932298e4ae
// EntropyInputPR = a4341ae5155601af7ccfd9bc573968f99ff82ae2605a462af7e6ed6fd5f2cab6
// ReturnedBits = de6541dae09137dfe17fa3bc785c8f45d3d36cb621d76c53f9031b2853ee0657a1edba0f6f06dade6a5a62faec54cf69bbf15db2244909114b0486f75da3cf16
//
// ============================================================

// COUNT = 0
#define NIST_CTR_DRBG_AES128_UsePR_NoDF_EntropyInput_0 0x0b, 0xfd, 0xe8, 0xb3, 0x2c, 0x0a, 0xda, 0xbd, 0x84, 0xe2, 0x71, 0xd1, 0x9b, 0x76, 0xcb, 0x96, 0x95, 0xe1, 0xf8, 0x9a, 0x08, 0x13, 0xe6, 0x3e, 0x7d, 0x9e, 0x6a, 0x0d, 0xd1, 0x69, 0x1c, 0x55
#define NIST_CTR_DRBG_AES128_UsePR_NoDF_PersonalizationString_0 0xf3, 0x45, 0xff, 0xd4, 0x13, 0x8e, 0xa0, 0xb4, 0x1e, 0x1b, 0x87, 0xa6, 0x78, 0xe2, 0x4a, 0xd8, 0x7c, 0x14, 0xfa, 0xde, 0x68, 0xe7, 0x03, 0xb3, 0x59, 0xca, 0xaf, 0x2e, 0x26, 0x9e, 0x7c, 0x58
#define NIST_CTR_DRBG_AES128_UsePR_NoDF_AdditionalInput1_0 0x68, 0x38, 0x8f, 0x4c, 0x45, 0xc3, 0x49, 0xf0, 0x76, 0xc3, 0x1e, 0xbb, 0x97, 0x2c, 0x31, 0xd2, 0x58, 0x95, 0xdf, 0xad, 0xf3, 0xa8, 0xd8, 0xc3, 0x36, 0xaf, 0xac, 0xf3, 0xe3, 0x10, 0x8c, 0xaf
#define NIST_CTR_DRBG_AES128_UsePR_NoDF_EntropyInputPR1_0 0x2a, 0x04, 0x6c, 0xb4, 0x77, 0xaf, 0xd5, 0x42, 0x71, 0xb0, 0xf5, 0xd6, 0x92, 0x4c, 0x70, 0x6c, 0x40, 0x43, 0x70, 0x2a, 0x56, 0x57, 0xe9, 0x9b, 0x88, 0x4b, 0xbc, 0xd7, 0xec, 0x6a, 0xa2, 0x67
#define NIST_CTR_DRBG_AES128_UsePR_NoDF_AdditionalInput2_0 0xc8, 0xa9, 0x5b, 0xca, 0xb7, 0x63, 0x58, 0x60, 0x8d, 0xb1, 0xf6, 0x6e, 0xa8, 0xb4, 0x53, 0x7f, 0x3d, 0xd8, 0x70, 0xe3, 0x8d, 0xf4, 0x1b, 0xcf, 0xdf, 0x41, 0xd7, 0x3e, 0xb5, 0x90, 0x1c, 0x93
#define NIST_CTR_DRBG_AES128_UsePR_NoDF_EntropyInputPR2_0 0xd1, 0x42, 0xdf, 0xb3, 0x9b, 0x02, 0x51, 0x1f, 0x93, 0x31, 0x20, 0xa8, 0x8c, 0x2f, 0xab, 0xb8, 0x8e, 0x82, 0x4a, 0x91, 0x6c, 0x82, 0x6b, 0x20, 0xf4, 0x44, 0xc6, 0x2e, 0x9d, 0x9a, 0xf0, 0xb9
#define NIST_CTR_DRBG_AES128_UsePR_NoDF_ReturnedBytes_0 0xed, 0xa2, 0x04, 0xa7, 0xc5, 0x1b, 0x29, 0xa7, 0x9c, 0xa1, 0xa7, 0x14, 0x8d, 0xae, 0x64, 0x45, 0xb4, 0xc7, 0xd8, 0x30, 0x41, 0x0d, 0xe1, 0x31, 0xc5, 0xc4, 0xa2, 0x8e, 0x85, 0xc7, 0x4a, 0x15, 0xc7, 0x68, 0x16, 0x7d, 0xac, 0x0b, 0xba, 0x1c, 0xac, 0xc3, 0xdf, 0xfd, 0xb2, 0x53, 0x12, 0xf9, 0x18, 0x6f, 0xdf, 0xc9, 0xa6, 0xe5, 0x70, 0x08, 0x1f, 0x41, 0xc9, 0x97, 0xab, 0x82, 0x95, 0x24

// COUNT = 1
#define NIST_CTR_DRBG_AES128_UsePR_NoDF_EntropyInput_1 0x3a, 0xc6, 0x7c, 0x4f, 0x19, 0x26, 0xbf, 0xe8, 0x0e, 0x8a, 0xd2, 0x93, 0x1b, 0x6e, 0x6f, 0x20, 0x35, 0x89, 0x31, 0x6b, 0x7c, 0xa1, 0x52, 0xdd, 0x00, 0xfb, 0xd0, 0x68, 0x1f, 0x23, 0x1f, 0x58
#define NIST_CTR_DRBG_AES128_UsePR_NoDF_PersonalizationString_1 0x6e, 0xed, 0xd4, 0xb2, 0xa1, 0xe4, 0x1b, 0xfc, 0x47, 0x0c, 0x79, 0x06, 0x85, 0xf9, 0x76, 0xd2, 0x87, 0xa8, 0x80, 0xf1, 0x5f, 0x1a, 0xb8, 0x93, 0xd8, 0x75, 0x40, 0xd5, 0x67, 0x72, 0xbd, 0xca
#define NIST_CTR_DRBG_AES128_UsePR_NoDF_AdditionalInput1_1 0xff, 0xff, 0x55, 0xb7, 0x1b, 0x1c, 0xa8, 0xad, 0x44, 0xb7, 0xd5, 0x01, 0x1c, 0x1e, 0x68, 0x5c, 0x88, 0x17, 0x97, 0x83, 0xdf, 0x0c, 0x55, 0x51, 0x72, 0xf2, 0x01, 0x89, 0xac, 0xdc, 0xb2, 0x34
#define NIST_CTR_DRBG_AES128_UsePR_NoDF_EntropyInputPR1_1 0x89, 0x92, 0xb1, 0x09, 0x2d, 0xc4, 0xe3, 0xdb, 0x41, 0xf6, 0xdd, 0xab, 0xb1, 0xeb, 0x86, 0xd4, 0x2c, 0x90, 0xc1, 0xea, 0x96, 0x73, 0x64, 0x6c, 0x6d, 0x69, 0xa8, 0xdc, 0x0c, 0xd9, 0x28, 0x4f
#define NIST_CTR_DRBG_AES128_UsePR_NoDF_AdditionalInput2_1 0x76, 0x35, 0xa5, 0x7b, 0x4f, 0x40, 0xad, 0xd8, 0xfe, 0xd1, 0x03, 0x2e, 0x76, 0x1a, 0x96, 0x2c, 0x9f, 0xe2, 0xc6, 0xa9, 0xf0, 0x10, 0x23, 0x88, 0x4f, 0x50, 0x2a, 0x04, 0x94, 0xc6, 0x0f, 0x18
#define NIST_CTR_DRBG_AES128_UsePR_NoDF_EntropyInputPR2_1 0x99, 0x04, 0xcf, 0x4e, 0x3d, 0x81, 0x0a, 0x2b, 0xec, 0x3a, 0x30, 0x09, 0x73, 0x5e, 0x89, 0xa4, 0x5a, 0x25, 0x9e, 0xb3, 0x72, 0xaa, 0x3d, 0xa3, 0xff, 0x0d, 0x53, 0xe2, 0x47, 0x8b, 0x22, 0x8b
#define NIST_CTR_DRBG_AES128_UsePR_NoDF_ReturnedBytes_1 0x8e, 0xeb, 0x30, 0xb0, 0x24, 0x55, 0x9e, 0x6c, 0x9c, 0xc3, 0xa6, 0x24, 0x7f, 0x7d, 0x11, 0x84, 0xc1, 0x1a, 0x5e, 0x68, 0x11, 0xa2, 0xdc, 0xce, 0xbd, 0x6c, 0x3b, 0xed, 0xcc, 0xeb, 0x8c, 0xb4, 0xed, 0xa1, 0x56, 0xea, 0xac, 0x30, 0x92, 0x01, 0xa1, 0x62, 0xca, 0x23, 0x11, 0xb1, 0x3c, 0x7f, 0xc0, 0x5f, 0x06, 0x52, 0xb0, 0x85, 0x07, 0xc2, 0xce, 0x57, 0xc4, 0xbd, 0x81, 0x9b, 0x4a, 0x2f

// COUNT = 2
#define NIST_CTR_DRBG_AES128_UsePR_NoDF_EntropyInput_2 0x0e, 0x19, 0xa3, 0xe9, 0x8e, 0xc9, 0xcc, 0xb8, 0xda, 0x19, 0x5b, 0x61, 0xf4, 0x18, 0x42, 0xc9, 0x5c, 0x14, 0xe6, 0x93, 0x74, 0x9e, 0x55, 0xf5, 0xfc, 0x0e, 0x55, 0xff, 0x63, 0xf7, 0x4e, 0x68
#define NIST_CTR_DRBG_AES128_UsePR_NoDF_PersonalizationString_2 0x27, 0x08, 0x83, 0x54, 0x5a, 0xb4, 0x1c, 0xf4, 0x26, 0xf9, 0xa9, 0x67, 0x52, 0x17, 0x2a, 0x71, 0xab, 0xfc, 0x79, 0x75, 0x52, 0xb3, 0xed, 0xd4, 0x63, 0xbb, 0xb7, 0x23, 0x18, 0x33, 0x55, 0x2d
#define NIST_CTR_DRBG_AES128_UsePR_NoDF_AdditionalInput1_2 0x10, 0x78, 0xf5, 0x4e, 0x5b, 0x60, 0xfd, 0x70, 0xb4, 0x8b, 0x1b, 0x70, 0xd1, 0xd0, 0xb4, 0xd1, 0x75, 0x49, 0xc3, 0xe6, 0xb0, 0xbe, 0xcf, 0xac, 0x8a, 0x7a, 0x45, 0x17, 0xb4, 0xd2, 0xf6, 0x84
#define NIST_CTR_DRBG_AES128_UsePR_NoDF_EntropyInputPR1_2 0x82, 0xb6, 0x8c, 0x3f, 0xd2, 0xe8, 0x75, 0x0c, 0x2b, 0xfd, 0x83, 0x4d, 0xd3, 0x7b, 0x6a, 0x47, 0x1c, 0x30, 0xf6, 0x58, 0x1a, 0x93, 0xd7, 0x62, 0x18, 0x4f, 0x61, 0x78, 0x25, 0xb2, 0xdd, 0x83
#define NIST_CTR_DRBG_AES128_UsePR_NoDF_AdditionalInput2_2 0x52, 0xdc, 0xe3, 0x79, 0x6b, 0x79, 0x2a, 0x58, 0x87, 0x8b, 0x37, 0x0e, 0x78, 0x9e, 0x3c, 0xdd, 0x13, 0xc3, 0x9e, 0xe9, 0x36, 0x1e, 0x0d, 0x0d, 0x8f, 0xe3, 0xbb, 0x33, 0x82, 0x2e, 0x60, 0xc4
#define NIST_CTR_DRBG_AES128_UsePR_NoDF_EntropyInputPR2_2 0x47, 0xcd, 0x9d, 0xc7, 0x10, 0xf0, 0x7b, 0xb6, 0xf5, 0xea, 0xb8, 0x1d, 0xad, 0x0e, 0x28, 0xa6, 0xcc, 0xaa, 0x8b, 0x74, 0x5a, 0xca, 0x22, 0x49, 0xd4, 0x9f, 0x3b, 0x4a, 0xe3, 0x4f, 0x23, 0x75
#define NIST_CTR_DRBG_AES128_UsePR_NoDF_ReturnedBytes_2 0x99, 0x27, 0x0a, 0xb2, 0xf4, 0x4d, 0x0c, 0xc1, 0xe6, 0xc1, 0x88, 0x8a, 0xd5, 0xbc, 0x63, 0x8b, 0x28, 0x43, 0x25, 0x59, 0x08, 0x7a, 0xda, 0x30, 0xf6, 0x3b, 0x37, 0x24, 0x2c, 0x84, 0x8c, 0x8f, 0xe0, 0x48, 0xbf, 0x27, 0x5b, 0xc5, 0x46, 0xbe, 0xe8, 0x28, 0x3d, 0xd6, 0x6b, 0x6f, 0xaa, 0x30, 0x60, 0xe4, 0x34, 0x23, 0x15, 0x20, 0x21, 0xc7, 0xc3, 0x6a, 0x89, 0x32, 0x52, 0x8f, 0x55, 0x57

uint8_t NIST_CTR_DRBG_AES128_UsePR_NoDF_Count = 3;
bool NIST_CTR_DRBG_AES128_UsePR_NoDF_PredictionResistance = true;
bool NIST_CTR_DRBG_AES128_UsePR_NoDF_Reseed = false;
bool NIST_CTR_DRBG_AES128_UsePR_NoDF_DerivationFunction = false;

size_t NIST_CTR_DRBG_AES128_UsePR_NoDF_EntropyInputLen = 32;
size_t NIST_CTR_DRBG_AES128_UsePR_NoDF_PersonalizationStringLen = 32;
size_t NIST_CTR_DRBG_AES128_UsePR_NoDF_AdditionalInputLen = 32;
size_t NIST_CTR_DRBG_AES128_UsePR_NoDF_ReturnedBytesLen = 64;

uint8_t NIST_CTR_DRBG_AES128_UsePR_NoDF_EntropyInput[] = {
          NIST_CTR_DRBG_AES128_UsePR_NoDF_EntropyInput_0,
	  NIST_CTR_DRBG_AES128_UsePR_NoDF_EntropyInputPR1_0,
	  NIST_CTR_DRBG_AES128_UsePR_NoDF_EntropyInputPR2_0,
	  NIST_CTR_DRBG_AES128_UsePR_NoDF_EntropyInput_1,
	  NIST_CTR_DRBG_AES128_UsePR_NoDF_EntropyInputPR1_1,
	  NIST_CTR_DRBG_AES128_UsePR_NoDF_EntropyInputPR2_1,
	  NIST_CTR_DRBG_AES128_UsePR_NoDF_EntropyInput_2,
	  NIST_CTR_DRBG_AES128_UsePR_NoDF_EntropyInputPR1_2,
	  NIST_CTR_DRBG_AES128_UsePR_NoDF_EntropyInputPR2_2 };

uint8_t NIST_CTR_DRBG_AES128_UsePR_NoDF_PersonalizationString[] = {
          NIST_CTR_DRBG_AES128_UsePR_NoDF_PersonalizationString_0,
	  NIST_CTR_DRBG_AES128_UsePR_NoDF_PersonalizationString_1,
	  NIST_CTR_DRBG_AES128_UsePR_NoDF_PersonalizationString_2 };

uint8_t NIST_CTR_DRBG_AES128_UsePR_NoDF_AdditionalInput[] = {
          NIST_CTR_DRBG_AES128_UsePR_NoDF_AdditionalInput1_0,
	  NIST_CTR_DRBG_AES128_UsePR_NoDF_AdditionalInput2_0,
	  NIST_CTR_DRBG_AES128_UsePR_NoDF_AdditionalInput1_1,
	  NIST_CTR_DRBG_AES128_UsePR_NoDF_AdditionalInput2_1,
	  NIST_CTR_DRBG_AES128_UsePR_NoDF_AdditionalInput1_2,
	  NIST_CTR_DRBG_AES128_UsePR_NoDF_AdditionalInput2_2 };

uint8_t NIST_CTR_DRBG_AES128_UsePR_NoDF_ReturnedBytes[] = {
          NIST_CTR_DRBG_AES128_UsePR_NoDF_ReturnedBytes_0,
	  NIST_CTR_DRBG_AES128_UsePR_NoDF_ReturnedBytes_1,
	  NIST_CTR_DRBG_AES128_UsePR_NoDF_ReturnedBytes_2 };

// ============================================================
// [AES-128 use df]
// [PredictionResistance = True]
// [EntropyInputLen = 128]
// [NonceLen = 64]
// [PersonalizationStringLen = 0]
// [AdditionalInputLen = 128]
// [ReturnedBitsLen = 512]
// ============================================================
//
// COUNT = 0
// EntropyInput = c4e0e51d45891aa9b31f78634859b006
// Nonce = a046fc4cc969c0d9
// PersonalizationString =
// AdditionalInput = ac3d13132519b2e70b41893645c77bb3
// EntropyInputPR = dc1ac451efe417e711ecf69569b1b716
// AdditionalInput = 9758af204152eb669e5fc120039514be
// EntropyInputPR = 1b603b8ce153ff789d74508bbddb3d13
// ReturnedBits = d70c5ad5672a6e3080d626d0b975791de59993b952a81ac6b728c350958b270980aaea538ed5e69488a77fda336467a1a9593d8a66ad3091e05e17a75ed7fce6
//
// COUNT = 1
// EntropyInput = 1735e27b042dd1b5768e6ad7720b711d
// Nonce = ca473c140a8edbe0
// PersonalizationString =
// AdditionalInput = 3879e08844fcfcbf21e306e9efd7eb82
// EntropyInputPR = e53542cb32cf20603940158e47f72ea2
// AdditionalInput = ec043007399532d1af00d73f9a4f1b56
// EntropyInputPR = 82c047fce9a432b4e3ed2916ef570748
// ReturnedBits = ba1aa8e4cbbb8bd0232ce33eedf81d56ffa096f1f8e3617e3455e71d8a24bebcf27aeecfa9905e006006dcbd334eec4b99316bc6a1ef92dc6937934b4e8926d2
//
// COUNT = 2
// EntropyInput = 6a8fe1d7f9c3aeaa7745da2524d66c3e
// Nonce = 103f02622a44e56f
// PersonalizationString =
// AdditionalInput = 299f9739930f544e6c1febb87f214c71
// EntropyInputPR = a5f362b2f2fe26e4718120a13f4747c4
// AdditionalInput = c03fe239f0445a7600cc07cecb8646f8
// EntropyInputPR = 37436047126251da75c3eff749d15633
// ReturnedBits = b480e94ab21f13710b9fcb9694d635cbee3226bb88d641c3617fa4fc94477bbdda49e1e92f8c2989ddbb2ad0719ae8e3515f4b205cf646086d645c4a45e64135
//
// COUNT = 3
// EntropyInput = a367c942f98bd98cd2132e6916ae2644
// Nonce = f12f3e04143ad917
// PersonalizationString =
// AdditionalInput = 3c753649e0c737aca45ec207f07472c3
// EntropyInputPR = aa0d4350f299dc821880806668467314
// AdditionalInput = 4e746efb72f015250d5e4adcf522ff7b
// EntropyInputPR = 455937c025036823de50e09f15d56e5e
// ReturnedBits = 9bfb876590592fb92f1b53044ee08a66ec01086f76cb83a68b7ee3bbb4bb7d1531574b77ca418f28deb8fdf7ec54e56a3332099bdb6c1257cf2eb05ad04e989d
//
// COUNT = 4
// EntropyInput = 7a5241620584f85233eec93f989bd0d9
// Nonce = 95ab8f25d20815f0
// PersonalizationString =
// AdditionalInput = f2e223cf110b62dec97722071094dd00
// EntropyInputPR = 816ead1bf61980dad6ddf8088c324159
// AdditionalInput = 43ba196ddcc99056b0424e03429ed639
// EntropyInputPR = e4e6014ca06369ec806c6c5d70b49e31
// ReturnedBits = 0e8e57609f9fd1ea5c2575d047fc1166efb862c62649cfe0710484967c031a5ff39405dd9261c952a0dcca20a8107bb5df6037627512d5a62199105b4685b0c9
//
// COUNT = 5
// EntropyInput = feb289cf791beb7dd4b0d87f5c805ea0
// Nonce = 783961e0681a1fca
// PersonalizationString =
// AdditionalInput = 55063e0b56ee6adbbf00e6c1c775c803
// EntropyInputPR = 84ce9821b35f7fe98cbaefb3080c92db
// AdditionalInput = d3580eb4fb45a94af60794693df707cd
// EntropyInputPR = 1d3286234ae806eda33b275538e7fc0d
// ReturnedBits = 7009467952748c11b45ab8c1bfe992c9775c7152893f66c68ad7f36886ee25cb8f2e6d86453010854ac951629f1828fe5c85ba7c868d3dd45c2b13c0a0aa7cbf
//
// COUNT = 6
// EntropyInput = 2ac106674efab411f51d52e5d65e993a
// Nonce = 79790182b2c3d76a
// PersonalizationString =
// AdditionalInput = 20b944424d654b50386d0d80ec3a0cad
// EntropyInputPR = 3ea42388d2905ba5a69cd62e2fdeec66
// AdditionalInput = 3f16b219cda6e92f9085767c78fa5056
// EntropyInputPR = e5dc83b1303dbf5c83fcab48e2ae6a70
// ReturnedBits = d48a95390a20f0f75bd9e2e588e14ab0c88efd70016195e0338746b24648a92f32ca860ff10c50764364b8146465e1eadc7241b0309b2aca6a0130fafe567350
//
// COUNT = 7
// EntropyInput = 31f58dd368c2824e6e80a7c02dbc367b
// Nonce = 0e71a8c11ce8106f
// PersonalizationString =
// AdditionalInput = 9994997ed4324376cbebe98d601f13b5
// EntropyInputPR = 127d6a9fddae3be1b662f643a54ecc05
// AdditionalInput = bcf5b4ca255a3128ce84364985319481
// EntropyInputPR = 9fe8c75bd8fedfb3b2d1a52cca133dde
// ReturnedBits = 1fb4f43f8937c1b8b44e2349d81daf5737b4125329583513d957641cc16bc9a5228b4e2c2fc98a0b5f2df6dd610058d4539f0f507e7406efb893605b291d84fb
//
// COUNT = 8
// EntropyInput = d2dedd97eb0b45c590e1866143d13606
// Nonce = 30aca6cb2f691009
// PersonalizationString =
// AdditionalInput = 8d89b8a9140584480094eb88af6ecfae
// EntropyInputPR = c6b6834fe40e9595b03723894a3ea723
// AdditionalInput = 316a848250597e51e73143bc36e3d3b7
// EntropyInputPR = ceab7c1f7ce2997021a759dcae06ac14
// ReturnedBits = 695a2833b7f72f5b2b005fe3237b74ad562fca3b8d139c150c9f094c675402279eda56a79415486f20341b1af89c27f87eedd8aaaa773f025e4be3f0b83800ea
//
// COUNT = 9
// EntropyInput = f9becd9f9972d891a2863ba86c78839c
// Nonce = 8e2f8a508e1d0e56
// PersonalizationString =
// AdditionalInput = 6b7a9ee4190d9ebee2d257e9b2128ccb
// EntropyInputPR = 386074c4cae9b55026dc657d0c5abd58
// AdditionalInput = d30b83565c6ae1fd5420341c91b72cca
// EntropyInputPR = f8ca55a47a5d7e4e90aa4ab7185d4692
// ReturnedBits = b475155b6c5a8ff4d66e7ee2f9e48d3cd9045a6040f71f831b39cea490be035b76ef80c7f05589f4de02c7b4fcb1ba64f384132d2b1245992585c1d7cc91ab66
//
// COUNT = 10
// EntropyInput = 201eac70175e226b2965d06465879987
// Nonce = a9d7906ed85f620a
// PersonalizationString =
// AdditionalInput = ab7961fd742bba79fe58d7455cf865ad
// EntropyInputPR = b58ca87d157b8c2dc395dad55f805b2b
// AdditionalInput = d4fa5dca9ebdc601b0902528561339e0
// EntropyInputPR = 2eaa52d5563429dc47f2d6970461a4f2
// ReturnedBits = 4802babaf43054363be930ea6d1e6f3dbe73e520eec7c9707a016099d1881b1bf28bafafaadeb1dbc686177395b6b3cc4fcc605eddec7d57184407220bf5c89c
//
// COUNT = 11
// EntropyInput = dfc2e3b3b14f3aaf151b2f17749cfb5f
// Nonce = 0dd8eae1344acd0c
// PersonalizationString =
// AdditionalInput = aba55af266c8eb9153fcc145cc069e6e
// EntropyInputPR = b2bd8be7bfa0ee9f329af511c8ab631c
// AdditionalInput = d62a2a87d5d58a0f8224632e9a2d9da0
// EntropyInputPR = 42e2f384fb5ac867c64d40375e96309e
// ReturnedBits = a5d64131c446bc39baeb0169d7c25d107321240e7e6af0b405d8fcdb51b13171956cbd30898e559b9c04e9049d9f91c740bc65fdfcc6c1aede1c19be7c714b2e
//
// COUNT = 12
// EntropyInput = 9d32a93655ad5650994b230fb59ff642
// Nonce = f706a7de48c3eef0
// PersonalizationString =
// AdditionalInput = faf63877f747e7080a2e71842b63f104
// EntropyInputPR = 1b2adf4b386432d60aa7fb764b06dba6
// AdditionalInput = 4e50bb0c18fbca492482c1a7a0b66019
// EntropyInputPR = a6afd3417d2d04a5454aa485d15ce019
// ReturnedBits = 267847484cf6883347a475257a621267d975938d292c35dde4a78b5649024ddfb5c73a71523a4d9deb66f2d83fad2c2f858ed5ab2e5ffaab6aa87397738a4e66
//
// COUNT = 13
// EntropyInput = cc17b27afb86983a775ca42a2f5582be
// Nonce = 96b3194366d78cfd
// PersonalizationString =
// AdditionalInput = f6c94f8c76b4ac6479079332250d1a25
// EntropyInputPR = a068979be51f41f5ecf8b298cf507cec
// AdditionalInput = 16b64703f87fbbefba1981f473c46cb0
// EntropyInputPR = facbf390a4e597663ecb80d5ae0532c6
// ReturnedBits = f7bea94f8ef34567fc1e372991c4bcca008c0c6e607eede915b0e65253baf187797c8017f6e66fbb94c265fb1542ac919de5a6d1a1d64e131bf246c833f8ab21
//
// COUNT = 14
// EntropyInput = 8f9f23e05cf41f237fe8542d024d8fea
// Nonce = bf92dbde6cb620da
// PersonalizationString =
// AdditionalInput = 2bd895b6dc1224876be8ca9efe772ef3
// EntropyInputPR = 1e4b4f234904868bb6a636a5ca4fdd27
// AdditionalInput = eab62c59cf9660790bf932a8fbb5cb63
// EntropyInputPR = d4b1f764e548985902c476bf8319c935
// ReturnedBits = 9dc3680f2b1e78c0745e29dd72f40e0f1ed1ea4e2e09649db16403be15e62f58de1aa6e32be5827d347849895e700c2ab16f4b4c0f2e85e6d1c0f3f61d12e112
//
// ============================================================

// COUNT = 0
#define NIST_CTR_DRBG_AES128_UsePR_UseDF_EntropyInput_0 0xc4, 0xe0, 0xe5, 0x1d, 0x45, 0x89, 0x1a, 0xa9, 0xb3, 0x1f, 0x78, 0x63, 0x48, 0x59, 0xb0, 0x06
#define NIST_CTR_DRBG_AES128_UsePR_UseDF_Nonce_0 0xa0, 0x46, 0xfc, 0x4c, 0xc9, 0x69, 0xc0, 0xd9
#define NIST_CTR_DRBG_AES128_UsePR_UseDF_AdditionalInput1_0 0xac, 0x3d, 0x13, 0x13, 0x25, 0x19, 0xb2, 0xe7, 0x0b, 0x41, 0x89, 0x36, 0x45, 0xc7, 0x7b, 0xb3
#define NIST_CTR_DRBG_AES128_UsePR_UseDF_EntropyInputPR1_0 0xdc, 0x1a, 0xc4, 0x51, 0xef, 0xe4, 0x17, 0xe7, 0x11, 0xec, 0xf6, 0x95, 0x69, 0xb1, 0xb7, 0x16
#define NIST_CTR_DRBG_AES128_UsePR_UseDF_AdditionalInput2_0 0x97, 0x58, 0xaf, 0x20, 0x41, 0x52, 0xeb, 0x66, 0x9e, 0x5f, 0xc1, 0x20, 0x03, 0x95, 0x14, 0xbe
#define NIST_CTR_DRBG_AES128_UsePR_UseDF_EntropyInputPR2_0 0x1b, 0x60, 0x3b, 0x8c, 0xe1, 0x53, 0xff, 0x78, 0x9d, 0x74, 0x50, 0x8b, 0xbd, 0xdb, 0x3d, 0x13
#define NIST_CTR_DRBG_AES128_UsePR_UseDF_ReturnedBytes_0 0xd7, 0x0c, 0x5a, 0xd5, 0x67, 0x2a, 0x6e, 0x30, 0x80, 0xd6, 0x26, 0xd0, 0xb9, 0x75, 0x79, 0x1d, 0xe5, 0x99, 0x93, 0xb9, 0x52, 0xa8, 0x1a, 0xc6, 0xb7, 0x28, 0xc3, 0x50, 0x95, 0x8b, 0x27, 0x09, 0x80, 0xaa, 0xea, 0x53, 0x8e, 0xd5, 0xe6, 0x94, 0x88, 0xa7, 0x7f, 0xda, 0x33, 0x64, 0x67, 0xa1, 0xa9, 0x59, 0x3d, 0x8a, 0x66, 0xad, 0x30, 0x91, 0xe0, 0x5e, 0x17, 0xa7, 0x5e, 0xd7, 0xfc, 0xe6

// COUNT = 1
#define NIST_CTR_DRBG_AES128_UsePR_UseDF_EntropyInput_1 0x17, 0x35, 0xe2, 0x7b, 0x04, 0x2d, 0xd1, 0xb5, 0x76, 0x8e, 0x6a, 0xd7, 0x72, 0x0b, 0x71, 0x1d
#define NIST_CTR_DRBG_AES128_UsePR_UseDF_Nonce_1 0xca, 0x47, 0x3c, 0x14, 0x0a, 0x8e, 0xdb, 0xe0
#define NIST_CTR_DRBG_AES128_UsePR_UseDF_AdditionalInput1_1 0x38, 0x79, 0xe0, 0x88, 0x44, 0xfc, 0xfc, 0xbf, 0x21, 0xe3, 0x06, 0xe9, 0xef, 0xd7, 0xeb, 0x82
#define NIST_CTR_DRBG_AES128_UsePR_UseDF_EntropyInputPR1_1 0xe5, 0x35, 0x42, 0xcb, 0x32, 0xcf, 0x20, 0x60, 0x39, 0x40, 0x15, 0x8e, 0x47, 0xf7, 0x2e, 0xa2
#define NIST_CTR_DRBG_AES128_UsePR_UseDF_AdditionalInput2_1 0xec, 0x04, 0x30, 0x07, 0x39, 0x95, 0x32, 0xd1, 0xaf, 0x00, 0xd7, 0x3f, 0x9a, 0x4f, 0x1b, 0x56
#define NIST_CTR_DRBG_AES128_UsePR_UseDF_EntropyInputPR2_1 0x82, 0xc0, 0x47, 0xfc, 0xe9, 0xa4, 0x32, 0xb4, 0xe3, 0xed, 0x29, 0x16, 0xef, 0x57, 0x07, 0x48
#define NIST_CTR_DRBG_AES128_UsePR_UseDF_ReturnedBytes_1 0xba, 0x1a, 0xa8, 0xe4, 0xcb, 0xbb, 0x8b, 0xd0, 0x23, 0x2c, 0xe3, 0x3e, 0xed, 0xf8, 0x1d, 0x56, 0xff, 0xa0, 0x96, 0xf1, 0xf8, 0xe3, 0x61, 0x7e, 0x34, 0x55, 0xe7, 0x1d, 0x8a, 0x24, 0xbe, 0xbc, 0xf2, 0x7a, 0xee, 0xcf, 0xa9, 0x90, 0x5e, 0x00, 0x60, 0x06, 0xdc, 0xbd, 0x33, 0x4e, 0xec, 0x4b, 0x99, 0x31, 0x6b, 0xc6, 0xa1, 0xef, 0x92, 0xdc, 0x69, 0x37, 0x93, 0x4b, 0x4e, 0x89, 0x26, 0xd2

// COUNT = 2
#define NIST_CTR_DRBG_AES128_UsePR_UseDF_EntropyInput_2 0x6a, 0x8f, 0xe1, 0xd7, 0xf9, 0xc3, 0xae, 0xaa, 0x77, 0x45, 0xda, 0x25, 0x24, 0xd6, 0x6c, 0x3e
#define NIST_CTR_DRBG_AES128_UsePR_UseDF_Nonce_2 0x10, 0x3f, 0x02, 0x62, 0x2a, 0x44, 0xe5, 0x6f
#define NIST_CTR_DRBG_AES128_UsePR_UseDF_AdditionalInput1_2 0x29, 0x9f, 0x97, 0x39, 0x93, 0x0f, 0x54, 0x4e, 0x6c, 0x1f, 0xeb, 0xb8, 0x7f, 0x21, 0x4c, 0x71
#define NIST_CTR_DRBG_AES128_UsePR_UseDF_EntropyInputPR1_2 0xa5, 0xf3, 0x62, 0xb2, 0xf2, 0xfe, 0x26, 0xe4, 0x71, 0x81, 0x20, 0xa1, 0x3f, 0x47, 0x47, 0xc4
#define NIST_CTR_DRBG_AES128_UsePR_UseDF_AdditionalInput2_2 0xc0, 0x3f, 0xe2, 0x39, 0xf0, 0x44, 0x5a, 0x76, 0x00, 0xcc, 0x07, 0xce, 0xcb, 0x86, 0x46, 0xf8
#define NIST_CTR_DRBG_AES128_UsePR_UseDF_EntropyInputPR2_2 0x37, 0x43, 0x60, 0x47, 0x12, 0x62, 0x51, 0xda, 0x75, 0xc3, 0xef, 0xf7, 0x49, 0xd1, 0x56, 0x33
#define NIST_CTR_DRBG_AES128_UsePR_UseDF_ReturnedBytes_2 0xb4, 0x80, 0xe9, 0x4a, 0xb2, 0x1f, 0x13, 0x71, 0x0b, 0x9f, 0xcb, 0x96, 0x94, 0xd6, 0x35, 0xcb, 0xee, 0x32, 0x26, 0xbb, 0x88, 0xd6, 0x41, 0xc3, 0x61, 0x7f, 0xa4, 0xfc, 0x94, 0x47, 0x7b, 0xbd, 0xda, 0x49, 0xe1, 0xe9, 0x2f, 0x8c, 0x29, 0x89, 0xdd, 0xbb, 0x2a, 0xd0, 0x71, 0x9a, 0xe8, 0xe3, 0x51, 0x5f, 0x4b, 0x20, 0x5c, 0xf6, 0x46, 0x08, 0x6d, 0x64, 0x5c, 0x4a, 0x45, 0xe6, 0x41, 0x35

uint8_t NIST_CTR_DRBG_AES128_UsePR_UseDF_Count = 3;
bool NIST_CTR_DRBG_AES128_UsePR_UseDF_PredictionResistance = true;
bool NIST_CTR_DRBG_AES128_UsePR_UseDF_Reseed = false;
bool NIST_CTR_DRBG_AES128_UsePR_UseDF_DerivationFunction = true;

size_t NIST_CTR_DRBG_AES128_UsePR_UseDF_EntropyInputLen = 16;
size_t NIST_CTR_DRBG_AES128_UsePR_UseDF_PersonalizationStringLen = 8;
size_t NIST_CTR_DRBG_AES128_UsePR_UseDF_AdditionalInputLen = 16;
size_t NIST_CTR_DRBG_AES128_UsePR_UseDF_ReturnedBytesLen = 64;

uint8_t NIST_CTR_DRBG_AES128_UsePR_UseDF_EntropyInput[] = {
          NIST_CTR_DRBG_AES128_UsePR_UseDF_EntropyInput_0,
	  NIST_CTR_DRBG_AES128_UsePR_UseDF_EntropyInputPR1_0,
	  NIST_CTR_DRBG_AES128_UsePR_UseDF_EntropyInputPR2_0,
	  NIST_CTR_DRBG_AES128_UsePR_UseDF_EntropyInput_1,
	  NIST_CTR_DRBG_AES128_UsePR_UseDF_EntropyInputPR1_1,
	  NIST_CTR_DRBG_AES128_UsePR_UseDF_EntropyInputPR2_1,
	  NIST_CTR_DRBG_AES128_UsePR_UseDF_EntropyInput_2,
	  NIST_CTR_DRBG_AES128_UsePR_UseDF_EntropyInputPR1_2,
	  NIST_CTR_DRBG_AES128_UsePR_UseDF_EntropyInputPR2_2 };

uint8_t NIST_CTR_DRBG_AES128_UsePR_UseDF_PersonalizationString[] = {
          NIST_CTR_DRBG_AES128_UsePR_UseDF_Nonce_0,
	  NIST_CTR_DRBG_AES128_UsePR_UseDF_Nonce_1,
	  NIST_CTR_DRBG_AES128_UsePR_UseDF_Nonce_2 };

uint8_t NIST_CTR_DRBG_AES128_UsePR_UseDF_AdditionalInput[] = {
          NIST_CTR_DRBG_AES128_UsePR_UseDF_AdditionalInput1_0,
	  NIST_CTR_DRBG_AES128_UsePR_UseDF_AdditionalInput2_0,
	  NIST_CTR_DRBG_AES128_UsePR_UseDF_AdditionalInput1_1,
	  NIST_CTR_DRBG_AES128_UsePR_UseDF_AdditionalInput2_1,
	  NIST_CTR_DRBG_AES128_UsePR_UseDF_AdditionalInput1_2,
	  NIST_CTR_DRBG_AES128_UsePR_UseDF_AdditionalInput2_2 };

uint8_t NIST_CTR_DRBG_AES128_UsePR_UseDF_ReturnedBytes[] = {
          NIST_CTR_DRBG_AES128_UsePR_UseDF_ReturnedBytes_0,
	  NIST_CTR_DRBG_AES128_UsePR_UseDF_ReturnedBytes_1,
	  NIST_CTR_DRBG_AES128_UsePR_UseDF_ReturnedBytes_2 };

// ============================================================
// [AES-128 no df]
// [PredictionResistance = False]
// [EntropyInputLen = 256]
// [NonceLen = 0]
// [PersonalizationStringLen = 0]
// [AdditionalInputLen = 256]
// [ReturnedBitsLen = 512]
// ============================================================
//
// COUNT = 0
// EntropyInput = 6bd4f2ae649fc99350951ff0c5d460c1a9214154e7384975ee54b34b7cae0704
// Nonce =
// PersonalizationString =
// AdditionalInput = ecd4893b979ac92db1894ae3724518a2f78cf2dbe2f6bbc6fda596df87c7a4ae
// AdditionalInput = b23e9188687c88768b26738862c4791fa52f92502e1f94bf66af017c4228a0dc
// ReturnedBits = 5b2bf7a5c60d8ab6591110cbd61cd387b02de19784f496d1a109123d8b3562a5de2dd6d5d1aef957a6c4f371cecd93c15799d82e34d6a0dba7e915a27d8e65f3
//
// COUNT = 1
// EntropyInput = e2addbde2a76e769fc7aa3f45b31402f482b73bbe7067ad6254621f06d3ef68b
// Nonce =
// PersonalizationString =
// AdditionalInput = ad11643b019e31245e4ea41f18f7680458310580fa6efad275c5833e7f800dae
// AdditionalInput = b5d849616b3123c9725d188cd0005003220768d1200f9e7cc29ef6d88afb7b9a
// ReturnedBits = 132d0d50c8477a400bb8935be5928f916a85da9ffcf1a8f6e9f9a14cca861036cda14cf66d8953dab456b632cf687cd539b4b807926561d0b3562b9d3334fb61
//
// COUNT = 2
// EntropyInput = 7eca92b11313e2eb67e808d21272945be432e98933000fce655cce068dba428b
// Nonce =
// PersonalizationString =
// AdditionalInput = 0346ce8aa1a452027a9b5fb0aca418f43496fd97b3ad3fcdce17c4e3ee27b322
// AdditionalInput = 78ecfbacec06fc61da03522745ac4fb1fb6dee6466720c02a83587718a0d763a
// ReturnedBits = 904cd7904d59335bf5552023b6689c8d61eca24851866a8377aec90dd87e311c622d507e678581513935bde12c8b351f8d9a8efcfcc1f1ff76c0613bffe7b455
//
// COUNT = 3
// EntropyInput = 690e1ffaa969bd3aad20b2d646a6c693753467528d1dc1fc74e906f49d075d89
// Nonce =
// PersonalizationString =
// AdditionalInput = 4a4345db4fed24cac49822cac58dc44edcdf9c63d1dcba9f460af9dfa2b555b6
// AdditionalInput = e2a565b0f970d2d09d1b59a6259b866912b266b76c3f09a7b6f0dfaefd46074d
// ReturnedBits = 27b4cf56e9d41fe11dc2933110cfcfd402ce6bc87406cde7f0aac0b2df410eec9666ff52e1a74f3224742af58da23bf5eabb12d289e41347b6d25ee0d2b37267
//
// COUNT = 4
// EntropyInput = bdfa5ebb4e3c31e63a9ff14c3e80ea35f86eff0269f3389f9f2e9a5191b6065e
// Nonce =
// PersonalizationString =
// AdditionalInput = de02565f2564bec869800a818ba79bdd37c9b0ab7fbb9cfe953eb14f0218ec21
// AdditionalInput = ab7b1cc8b89bfe190293fb803a637675940cf2c38611968f770621dfb0ae35c2
// ReturnedBits = dd639aff7276f19d80fca5641ff90a2026ffce0f7e1bf4d50398b8aea052e6b0714f52a3e76c82a92736d307f1b732d045709d8455ba89aae108daee9f65cc4f
//
// COUNT = 5
// EntropyInput = c16dc70849117595cfe206c270c9a753355e1ef23bebd3c341e5b564aaf73156
// Nonce =
// PersonalizationString =
// AdditionalInput = ede7927dd374cf979d00724029169ecd9420a0e4a4f1233a46697089f7eab409
// AdditionalInput = 0506894144058825274ec92290714b33528209beea3010e22ae9119381b8fe58
// ReturnedBits = a2cc30e9bd520d3ffc806398f81fff95b653777ab03affdc2ceb9a30570565995b421a9faaf7a5bf83bf812913f68b95adbb4c46318b2a9f457cd3278d4a532b
//
// COUNT = 6
// EntropyInput = b28cb5e20fb770bd1cd57433bd0b19eb05e319f77e2b466d835af8955222256b
// Nonce =
// PersonalizationString =
// AdditionalInput = ffb0f84a08a5a84433c7ac5704addbce0c2968b6f83df29549f431ca3df3a32c
// AdditionalInput = 897a570f514a03fb60cd8af583065771c2fe93a6ad5153e7727c791a95239dc7
// ReturnedBits = 9ae70e0f02bfe75ce820eb8fd1ce18b40e1f3719773c9735edb2938708cfba2a801abc72c445ab0c436df53182dd90b46c1e357787f15ce100e43c3cf7f9d5b5
//
// COUNT = 7
// EntropyInput = f999e225a513b3a97d72b8263385d5dc8bc98f088ece74d2274bc2f4e343ae62
// Nonce =
// PersonalizationString =
// AdditionalInput = fce85294caaf4ccd08c27fe0534dc882a3cc0faa123e7f5e92432e0cb12f48c6
// AdditionalInput = f7b393d3eac0d2daa35335a3ad89d666cd94b85c45778febd75f32cbc577de9d
// ReturnedBits = e9f7bba79e223501509912575dba64f0830cd321cbb6f75ed2d319605d744cdaad7eb7409d4000e767976d2f0b1e6653f94b0533e4e6e796dc6a225867442314
//
// COUNT = 8
// EntropyInput = 01b09cbb44ab39f7dbfd3e8b4d173b5b5433325a3419364f451b7ffb6eb351db
// Nonce =
// PersonalizationString =
// AdditionalInput = de95e4484c98324f3c7771f9525c11ee268e7395d4dd8046ac23ae5db8e361cf
// AdditionalInput = 7d376e9d319a275351fb273274b6fad9e1420ee6b10ee1ae330698a1d15cd7fd
// ReturnedBits = 4315ec46ef983676ee61b61d3623c3b6731389b12c7d27c87d07cca22bd91d53ef80705aca3740af2d5dd8452ec0d06879f1bcef0132d77d3c311e1b34a0dc5f
//
// COUNT = 9
// EntropyInput = 6cfaca3c03b0d98f69dfc9f2859f0db128d05534a0e27fe7433f6afc2ddedb75
// Nonce =
// PersonalizationString =
// AdditionalInput = 203b29aa54fee3ae86685c5cdf0347773301db03043a910ed063bbd249271887
// AdditionalInput = 57ded4f3ae768b100c36035ec5ecbc093e141e5bfb8746d344554ca64bcce016
// ReturnedBits = 03990f68ca84dc13b92e76647fa24dba47f36da6f1dfa9c4124de86762703cb33df730cee68a6acf8a4a4253a575d8709111ca6bae80507c248ebe4ff52cb959
//
// COUNT = 10
// EntropyInput = b0967a4aea350f67e7081537a2519f4f608f317edd14a24a75ba4369d224786d
// Nonce =
// PersonalizationString =
// AdditionalInput = 8b48effd791238b8329b9db8517674f59c60b2bb9ab06ea1c5d54ee471caaf89
// AdditionalInput = c55bbdc2aaea7dbd840a7296e22295ae2b95b0ca6192237c125da1707fe8cd0f
// ReturnedBits = 28f4fb5d85e5b8b151ceaaf33b2acb02b0b25d9cf2760567fb54b0b0f560bc8926663cec6c84e3560c462e802584ba3ad58cace1da964899c52592a9ac61979c
//
// COUNT = 11
// EntropyInput = b37cd1d227f3c817d23982630b19aa8a81c0415f602d271112d4e31689c8e979
// Nonce =
// PersonalizationString =
// AdditionalInput = 1f25517ab677bd8c4bf204c4f3fde59b6f337388385f28ca8dac2bce27e1df67
// AdditionalInput = 0acacc50533c77bfe09de685b516d1d9f2b03874975131ac85918bcebebbd543
// ReturnedBits = 9031b77db5be9b79150d623474c21bf5197ba4144d737d261f77eb6ef07b74c2d4b4ed6177480423421f1925d61e97741155cbd5d3bfcf5e1f1d30f3fcfd7be9
//
// COUNT = 12
// EntropyInput = f334c970a213759a03cef16d3b3a440143242af57dade4332588351e24152e60
// Nonce =
// PersonalizationString =
// AdditionalInput = ff3567674fe5c8dc44d4f9196246402b9bf3e71cd00bdd1f0db0242347ea2a1c
// AdditionalInput = 7d4c242438617e5d2690a8776f3fd9cf27d32a9c5744a9f2bd5cf116f66664a0
// ReturnedBits = d1b631f7ff945cc7e2954f6895b439e99ca1fff75ee19c14f27f4446f627c1e45bc56350b432f718ce83f09ca493959ecdbca3dc24b3ef96e01fc4da229d22df
//
// COUNT = 13
// EntropyInput = 727cfb51100a98cc7082b181dba3fdad2dd197bc0065d970476679a40ac19766
// Nonce =
// PersonalizationString =
// AdditionalInput = 03920223900cb0abaa95fdb8eb369ff66399cf657e71f2fc9e4b8d4682781af5
// AdditionalInput = 198246d1fa95ff480be0b84f2a107baadfaf7333980ab628c78232ccd899dc4b
// ReturnedBits = 8625a80cbefdf7b822e82a1e71104d449d8cef6a400fa769701d1cf60f5f1a147d9465f7f3cdc7d2f0606fe459e431fcc36df41a51e87650f585cb626ac3e220
//
// COUNT = 14
// EntropyInput = 02fbc1081c13134d2d68ea586ecf4a598caf7bf42ec0a3cd30018e01c264c84b
// Nonce =
// PersonalizationString =
// AdditionalInput = 170a92d093d30f939b3eac628a18bab5faf86b3a5d91f30cfd0beafdec41191c
// AdditionalInput = fd0349af015037cdbb52983155c89fc59f37d512543559c3ee6589f7b93861f6
// ReturnedBits = 0273a1317f3dd36877a505ca2e440445094d3c702c4ff5f4a07daa3f810d8d7a4f4b9c54dce169a1307fbdc5d197e6a3edc3ea737bedc1c9857aa0e9f87943e2
//
// ============================================================

// COUNT = 0
#define NIST_CTR_DRBG_AES128_NoReseed_NoDF_EntropyInput_0 0x6b, 0xd4, 0xf2, 0xae, 0x64, 0x9f, 0xc9, 0x93, 0x50, 0x95, 0x1f, 0xf0, 0xc5, 0xd4, 0x60, 0xc1, 0xa9, 0x21, 0x41, 0x54, 0xe7, 0x38, 0x49, 0x75, 0xee, 0x54, 0xb3, 0x4b, 0x7c, 0xae, 0x07, 0x04
#define NIST_CTR_DRBG_AES128_NoReseed_NoDF_AdditionalInput1_0 0xec, 0xd4, 0x89, 0x3b, 0x97, 0x9a, 0xc9, 0x2d, 0xb1, 0x89, 0x4a, 0xe3, 0x72, 0x45, 0x18, 0xa2, 0xf7, 0x8c, 0xf2, 0xdb, 0xe2, 0xf6, 0xbb, 0xc6, 0xfd, 0xa5, 0x96, 0xdf, 0x87, 0xc7, 0xa4, 0xae
#define NIST_CTR_DRBG_AES128_NoReseed_NoDF_AdditionalInput2_0 0xb2, 0x3e, 0x91, 0x88, 0x68, 0x7c, 0x88, 0x76, 0x8b, 0x26, 0x73, 0x88, 0x62, 0xc4, 0x79, 0x1f, 0xa5, 0x2f, 0x92, 0x50, 0x2e, 0x1f, 0x94, 0xbf, 0x66, 0xaf, 0x01, 0x7c, 0x42, 0x28, 0xa0, 0xdc
#define NIST_CTR_DRBG_AES128_NoReseed_NoDF_ReturnedBytes_0 0x5b, 0x2b, 0xf7, 0xa5, 0xc6, 0x0d, 0x8a, 0xb6, 0x59, 0x11, 0x10, 0xcb, 0xd6, 0x1c, 0xd3, 0x87, 0xb0, 0x2d, 0xe1, 0x97, 0x84, 0xf4, 0x96, 0xd1, 0xa1, 0x09, 0x12, 0x3d, 0x8b, 0x35, 0x62, 0xa5, 0xde, 0x2d, 0xd6, 0xd5, 0xd1, 0xae, 0xf9, 0x57, 0xa6, 0xc4, 0xf3, 0x71, 0xce, 0xcd, 0x93, 0xc1, 0x57, 0x99, 0xd8, 0x2e, 0x34, 0xd6, 0xa0, 0xdb, 0xa7, 0xe9, 0x15, 0xa2, 0x7d, 0x8e, 0x65, 0xf3

// COUNT = 1
#define NIST_CTR_DRBG_AES128_NoReseed_NoDF_EntropyInput_1 0xe2, 0xad, 0xdb, 0xde, 0x2a, 0x76, 0xe7, 0x69, 0xfc, 0x7a, 0xa3, 0xf4, 0x5b, 0x31, 0x40, 0x2f, 0x48, 0x2b, 0x73, 0xbb, 0xe7, 0x06, 0x7a, 0xd6, 0x25, 0x46, 0x21, 0xf0, 0x6d, 0x3e, 0xf6, 0x8b
#define NIST_CTR_DRBG_AES128_NoReseed_NoDF_AdditionalInput1_1 0xad, 0x11, 0x64, 0x3b, 0x01, 0x9e, 0x31, 0x24, 0x5e, 0x4e, 0xa4, 0x1f, 0x18, 0xf7, 0x68, 0x04, 0x58, 0x31, 0x05, 0x80, 0xfa, 0x6e, 0xfa, 0xd2, 0x75, 0xc5, 0x83, 0x3e, 0x7f, 0x80, 0x0d, 0xae
#define NIST_CTR_DRBG_AES128_NoReseed_NoDF_AdditionalInput2_1 0xb5, 0xd8, 0x49, 0x61, 0x6b, 0x31, 0x23, 0xc9, 0x72, 0x5d, 0x18, 0x8c, 0xd0, 0x00, 0x50, 0x03, 0x22, 0x07, 0x68, 0xd1, 0x20, 0x0f, 0x9e, 0x7c, 0xc2, 0x9e, 0xf6, 0xd8, 0x8a, 0xfb, 0x7b, 0x9a
#define NIST_CTR_DRBG_AES128_NoReseed_NoDF_ReturnedBytes_1 0x13, 0x2d, 0x0d, 0x50, 0xc8, 0x47, 0x7a, 0x40, 0x0b, 0xb8, 0x93, 0x5b, 0xe5, 0x92, 0x8f, 0x91, 0x6a, 0x85, 0xda, 0x9f, 0xfc, 0xf1, 0xa8, 0xf6, 0xe9, 0xf9, 0xa1, 0x4c, 0xca, 0x86, 0x10, 0x36, 0xcd, 0xa1, 0x4c, 0xf6, 0x6d, 0x89, 0x53, 0xda, 0xb4, 0x56, 0xb6, 0x32, 0xcf, 0x68, 0x7c, 0xd5, 0x39, 0xb4, 0xb8, 0x07, 0x92, 0x65, 0x61, 0xd0, 0xb3, 0x56, 0x2b, 0x9d, 0x33, 0x34, 0xfb, 0x61

// COUNT = 2
#define NIST_CTR_DRBG_AES128_NoReseed_NoDF_EntropyInput_2 0x7e, 0xca, 0x92, 0xb1, 0x13, 0x13, 0xe2, 0xeb, 0x67, 0xe8, 0x08, 0xd2, 0x12, 0x72, 0x94, 0x5b, 0xe4, 0x32, 0xe9, 0x89, 0x33, 0x00, 0x0f, 0xce, 0x65, 0x5c, 0xce, 0x06, 0x8d, 0xba, 0x42, 0x8b
#define NIST_CTR_DRBG_AES128_NoReseed_NoDF_AdditionalInput1_2 0x03, 0x46, 0xce, 0x8a, 0xa1, 0xa4, 0x52, 0x02, 0x7a, 0x9b, 0x5f, 0xb0, 0xac, 0xa4, 0x18, 0xf4, 0x34, 0x96, 0xfd, 0x97, 0xb3, 0xad, 0x3f, 0xcd, 0xce, 0x17, 0xc4, 0xe3, 0xee, 0x27, 0xb3, 0x22
#define NIST_CTR_DRBG_AES128_NoReseed_NoDF_AdditionalInput2_2 0x78, 0xec, 0xfb, 0xac, 0xec, 0x06, 0xfc, 0x61, 0xda, 0x03, 0x52, 0x27, 0x45, 0xac, 0x4f, 0xb1, 0xfb, 0x6d, 0xee, 0x64, 0x66, 0x72, 0x0c, 0x02, 0xa8, 0x35, 0x87, 0x71, 0x8a, 0x0d, 0x76, 0x3a
#define NIST_CTR_DRBG_AES128_NoReseed_NoDF_ReturnedBytes_2 0x90, 0x4c, 0xd7, 0x90, 0x4d, 0x59, 0x33, 0x5b, 0xf5, 0x55, 0x20, 0x23, 0xb6, 0x68, 0x9c, 0x8d, 0x61, 0xec, 0xa2, 0x48, 0x51, 0x86, 0x6a, 0x83, 0x77, 0xae, 0xc9, 0x0d, 0xd8, 0x7e, 0x31, 0x1c, 0x62, 0x2d, 0x50, 0x7e, 0x67, 0x85, 0x81, 0x51, 0x39, 0x35, 0xbd, 0xe1, 0x2c, 0x8b, 0x35, 0x1f, 0x8d, 0x9a, 0x8e, 0xfc, 0xfc, 0xc1, 0xf1, 0xff, 0x76, 0xc0, 0x61, 0x3b, 0xff, 0xe7, 0xb4, 0x55

uint8_t NIST_CTR_DRBG_AES128_NoReseed_NoDF_Count = 3;
bool NIST_CTR_DRBG_AES128_NoReseed_NoDF_PredictionResistance = false;
bool NIST_CTR_DRBG_AES128_NoReseed_NoDF_Reseed = false;
bool NIST_CTR_DRBG_AES128_NoReseed_NoDF_DerivationFunction = false;

size_t NIST_CTR_DRBG_AES128_NoReseed_NoDF_EntropyInputLen = 32;
size_t NIST_CTR_DRBG_AES128_NoReseed_NoDF_PersonalizationStringLen = 0;
size_t NIST_CTR_DRBG_AES128_NoReseed_NoDF_AdditionalInputLen = 32;
size_t NIST_CTR_DRBG_AES128_NoReseed_NoDF_ReturnedBytesLen = 64;

uint8_t NIST_CTR_DRBG_AES128_NoReseed_NoDF_EntropyInput[] = {
          NIST_CTR_DRBG_AES128_NoReseed_NoDF_EntropyInput_0,
	  NIST_CTR_DRBG_AES128_NoReseed_NoDF_EntropyInput_1,
	  NIST_CTR_DRBG_AES128_NoReseed_NoDF_EntropyInput_2 };

uint8_t *NIST_CTR_DRBG_AES128_NoReseed_NoDF_PersonalizationString = NULL;

uint8_t NIST_CTR_DRBG_AES128_NoReseed_NoDF_AdditionalInput[] = {
          NIST_CTR_DRBG_AES128_NoReseed_NoDF_AdditionalInput1_0,
	  NIST_CTR_DRBG_AES128_NoReseed_NoDF_AdditionalInput2_0,
	  NIST_CTR_DRBG_AES128_NoReseed_NoDF_AdditionalInput1_1,
	  NIST_CTR_DRBG_AES128_NoReseed_NoDF_AdditionalInput2_1,
	  NIST_CTR_DRBG_AES128_NoReseed_NoDF_AdditionalInput1_2,
	  NIST_CTR_DRBG_AES128_NoReseed_NoDF_AdditionalInput2_2 };

uint8_t NIST_CTR_DRBG_AES128_NoReseed_NoDF_ReturnedBytes[] = {
          NIST_CTR_DRBG_AES128_NoReseed_NoDF_ReturnedBytes_0,
	  NIST_CTR_DRBG_AES128_NoReseed_NoDF_ReturnedBytes_1,
	  NIST_CTR_DRBG_AES128_NoReseed_NoDF_ReturnedBytes_2 };

// ============================================================
// END
// ============================================================
