#ifndef ECHO_SERVER_H
#define ECHO_SERVER_H

#include <internal/WeavePlatformInternal.h>
#include <Weave/Profiles/echo/Next/WeaveEchoServer.h>

namespace WeavePlatform {
namespace Internal {

class EchoServer : public ::nl::Weave::Profiles::Echo_Next::WeaveEchoServer
{
public:
    typedef ::nl::Weave::Profiles::Echo_Next::WeaveEchoServer ServerBaseClass;

    WEAVE_ERROR Init();
};

extern EchoServer EchoSvr;

} // namespace Internal
} // namespace WeavePlatform

#endif // ECHO_SERVER_H
