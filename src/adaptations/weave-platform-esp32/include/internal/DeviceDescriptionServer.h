#ifndef DEVICE_DESCRIPTION_SERVER_H
#define DEVICE_DESCRIPTION_SERVER_H

#include <internal/WeavePlatformInternal.h>
#include <Weave/Profiles/device-description/DeviceDescription.h>

namespace WeavePlatform {
namespace Internal {

class DeviceDescriptionServer : public ::nl::Weave::Profiles::DeviceDescription::DeviceDescriptionServer
{
    typedef ::nl::Weave::Profiles::DeviceDescription::DeviceDescriptionServer ServerBaseClass;

public:
    WEAVE_ERROR Init();

    void OnPlatformEvent(const struct ::WeavePlatform::Internal::WeavePlatformEvent * event);

private:
    static void HandleIdentifyRequest(void *appState, uint64_t nodeId, const IPAddress& nodeAddr,
            const ::nl::Weave::Profiles::DeviceDescription::IdentifyRequestMessage& reqMsg, bool& sendResp,
            ::nl::Weave::Profiles::DeviceDescription::IdentifyResponseMessage& respMsg);
};

extern DeviceDescriptionServer DeviceDescriptionSvr;

} // namespace Internal
} // namespace WeavePlatform


#endif // DEVICE_DESCRIPTION_SERVER_H
