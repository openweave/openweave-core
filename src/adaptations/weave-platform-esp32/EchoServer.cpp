#include <internal/WeavePlatformInternal.h>
#include <internal/EchoServer.h>

using namespace ::nl;
using namespace ::nl::Weave;

namespace WeavePlatform {
namespace Internal {

WEAVE_ERROR EchoServer::Init()
{
    WEAVE_ERROR err;

    err = ServerBaseClass::Init(&::WeavePlatform::ExchangeMgr, ServerBaseClass::DefaultEventHandler);
    SuccessOrExit(err);

exit:
    return err;
}


} // namespace Internal
} // namespace WeavePlatform
