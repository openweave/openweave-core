#include <internal/WeavePlatformInternal.h>
#include <Weave/Support/logging/WeaveLogging.h>

namespace nl {
namespace Weave {
namespace Logging {

void Log(uint8_t module, uint8_t category, const char *msg, ...)
{
    va_list v;

    va_start(v, msg);

    if (IsCategoryEnabled(category))
    {
        char tag[7 + nlWeaveLoggingModuleNameLen + 1];
        size_t tagLen;
        char formattedMsg[256];

        strcpy(tag, "weave[");
        tagLen = strlen(tag);
        nl::Weave::Logging::GetModuleName(tag + tagLen, module);
        tagLen = strlen(tag);
        tag[tagLen++] = ']';
        tag[tagLen] = 0;

        vsnprintf(formattedMsg, sizeof(formattedMsg), msg, v);

        switch (category) {
        case kLogCategory_Error:
            ESP_LOGE(tag, "%s", formattedMsg);
            break;
        case kLogCategory_Progress:
        case kLogCategory_Retain:
        default:
            ESP_LOGI(tag, "%s", formattedMsg);
            break;
        case kLogCategory_Detail:
            ESP_LOGV(tag, "%s", formattedMsg);
            break;
        }
    }

    va_end(v);
}

} // namespace Logging
} // namespace Weave
} // namespace nl

