#include <WeavePlatform-ESP32-Internal.h>

#include <Weave/Profiles/echo/Next/WeaveEchoServer.h>

#include <new>

namespace WeavePlatform {
namespace Internal {

using namespace ::nl;
using namespace ::nl::Weave;
using namespace ::nl::Weave::Profiles::Echo_Next;

static WeaveEchoServer gWeaveEchoServer;

bool InitEchoServer()
{
    WEAVE_ERROR err;

    err = gWeaveEchoServer.Init(&ExchangeMgr, WeaveEchoServer::DefaultEventHandler);
    SuccessOrExit(err);

exit:
    if (err == WEAVE_NO_ERROR)
    {
        ESP_LOGI(TAG, "Weave Echo server initialized");
    }
    else
    {
        ESP_LOGE(TAG, "Weave Echo server initialization failed: %s", ErrorStr(err));
    }
    return (err == WEAVE_NO_ERROR);
}

} // namespace Internal
} // namespace WeavePlatform
