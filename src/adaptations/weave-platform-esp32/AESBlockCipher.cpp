#include <internal/WeavePlatformInternal.h>

#include <string.h>

#include <Weave/Support/crypto/WeaveCrypto.h>
#include <Weave/Support/crypto/AESBlockCipher.h>

#include <hwcrypto/aes.h>

namespace nl {
namespace Weave {
namespace Platform {
namespace Security {

using namespace nl::Weave::Crypto;

AES128BlockCipher::AES128BlockCipher()
{
    memset(&mKey, 0, sizeof(mKey));
}

AES128BlockCipher::~AES128BlockCipher()
{
    Reset();
}

void AES128BlockCipher::Reset()
{
    ClearSecretData((uint8_t *)&mKey, sizeof(mKey));
}

void AES128BlockCipherEnc::SetKey(const uint8_t *key)
{
    memcpy(mKey, key, kKeyLength);
}

void AES128BlockCipherEnc::EncryptBlock(const uint8_t *inBlock, uint8_t *outBlock)
{
    esp_aes_context ctx;

    esp_aes_init(&ctx);
    esp_aes_setkey(&ctx, mKey, kKeyLengthBits);
    esp_aes_encrypt(&ctx, inBlock, outBlock);
    esp_aes_free(&ctx);
}

void AES128BlockCipherDec::SetKey(const uint8_t *key)
{
    memcpy(mKey, key, kKeyLength);
}

void AES128BlockCipherDec::DecryptBlock(const uint8_t *inBlock, uint8_t *outBlock)
{
    esp_aes_context ctx;

    esp_aes_init(&ctx);
    esp_aes_setkey(&ctx, mKey, kKeyLengthBits);
    esp_aes_decrypt(&ctx, inBlock, outBlock);
    esp_aes_free(&ctx);
}

AES256BlockCipher::AES256BlockCipher()
{
    memset(&mKey, 0, sizeof(mKey));
}

AES256BlockCipher::~AES256BlockCipher()
{
    Reset();
}

void AES256BlockCipher::Reset()
{
    ClearSecretData((uint8_t *)&mKey, sizeof(mKey));
}

void AES256BlockCipherEnc::SetKey(const uint8_t *key)
{
    memcpy(mKey, key, kKeyLength);
}

void AES256BlockCipherEnc::EncryptBlock(const uint8_t *inBlock, uint8_t *outBlock)
{
    esp_aes_context ctx;

    esp_aes_init(&ctx);
    esp_aes_setkey(&ctx, mKey, kKeyLengthBits);
    esp_aes_encrypt(&ctx, inBlock, outBlock);
    esp_aes_free(&ctx);
}

void AES256BlockCipherDec::SetKey(const uint8_t *key)
{
    memcpy(mKey, key, kKeyLength);
}

void AES256BlockCipherDec::DecryptBlock(const uint8_t *inBlock, uint8_t *outBlock)
{
    esp_aes_context ctx;

    esp_aes_init(&ctx);
    esp_aes_setkey(&ctx, mKey, kKeyLengthBits);
    esp_aes_decrypt(&ctx, inBlock, outBlock);
    esp_aes_free(&ctx);
}

} // namespace Security
} // namespace Platform
} // namespace Weave
} // namespace nl

