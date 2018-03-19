#include <WeavePlatform-ESP32-Internal.h>

#include <Weave/Support/crypto/WeaveRNG.h>


namespace WeavePlatform {
namespace Internal {

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

} // namespace Internal
} // namespace WeavePlatform
