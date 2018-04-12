#include <internal/WeavePlatformInternal.h>
#include <Weave/Support/crypto/WeaveRNG.h>

using namespace ::nl;
using namespace ::nl::Weave;

namespace WeavePlatform {
namespace Internal {

namespace {

int GetEntropy_ESP32(uint8_t *buf, size_t bufSize)
{
    while (bufSize > 0) {
        union {
            uint32_t asInt;
            uint8_t asBytes[sizeof(asInt)];
        } rnd;

        rnd.asInt = esp_random();

        size_t n = nl::Weave::min(bufSize, sizeof(rnd.asBytes));

        memcpy(buf, rnd.asBytes, n);

        buf += n;
        bufSize -= n;
    }

    return 0;
}

} // unnamed namespace

WEAVE_ERROR InitEntropy()
{
    WEAVE_ERROR err;
    unsigned int seed;

    // Initialize the source used by Weave to get secure random data.
    err = ::nl::Weave::Platform::Security::InitSecureRandomDataSource(GetEntropy_ESP32, 64, NULL, 0);
    SuccessOrExit(err);

    // Seed the standard rand() pseudo-random generator with data from the secure random source.
    err = ::nl::Weave::Platform::Security::GetSecureRandomData((uint8_t *)&seed, sizeof(seed));
    SuccessOrExit(err);
    srand(seed);
    ESP_LOGI(TAG, "srand seed set: %u", seed);

exit:
    if (err != WEAVE_NO_ERROR)
    {
        ESP_LOGE(TAG, "InitEntropy() failed: %s", ErrorStr(err));
    }
    return err;
}

} // namespace Internal
} // namespace WeavePlatform
