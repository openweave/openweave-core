#include <WeavePlatform-ESP32-Internal.h>

using namespace nl;
using namespace nl::Inet;
using namespace nl::Weave;

namespace WeavePlatform {

using namespace WeavePlatform::Internal;

bool InitWeaveStack()
{
    WEAVE_ERROR err;

    // Initialize the source used by Weave to get secure random data.
    err = nl::Weave::Platform::Security::InitSecureRandomDataSource(GetEntropy_ESP32, 64, NULL, 0);
    if (err != WEAVE_NO_ERROR)
    {
        ESP_LOGE(TAG, "Secure random data source initialization failed: %s", ErrorStr(err));
        return false;
    }
    ESP_LOGI(TAG, "Secure random data source initialized");

    // Initialize the master Weave event queue.
    if (!InitWeaveEventQueue())
    {
        return false;
    }

    // Initialize the Configuration Manager object.
    err = ConfigMgr.Init();
    if (err != WEAVE_NO_ERROR)
    {
        ESP_LOGE(TAG, "Configuration Manager initialization failed: %s", ErrorStr(err));
        return false;
    }

    // Initialize the Weave system layer.
    err = SystemLayer.Init(NULL);
    if (err != WEAVE_SYSTEM_NO_ERROR)
    {
        ESP_LOGE(TAG, "SystemLayer initialization failed: %s", ErrorStr(err));
        return false;
    }

    // Initialize the Weave Inet layer.
    err = InetLayer.Init(SystemLayer, NULL);
    if (err != INET_NO_ERROR)
    {
        ESP_LOGE(TAG, "InetLayer initialization failed: %s", ErrorStr(err));
        return false;
    }

    // Initialize the Weave fabric state object.
    err = FabricState.Init();
    if (err != WEAVE_NO_ERROR)
    {
        ESP_LOGE(TAG, "FabricState initialization failed: %s", ErrorStr(err));
        return false;
    }

    FabricState.DefaultSubnet = kWeaveSubnetId_PrimaryWiFi;

#if WEAVE_CONFIG_SECURITY_TEST_MODE
    FabricState.LogKeys = true;
#endif

    WeaveMessageLayer::InitContext initContext;
    initContext.systemLayer = &SystemLayer;
    initContext.inet = &InetLayer;
    initContext.fabricState = &FabricState;
    initContext.listenTCP = true;
    initContext.listenUDP = true;

    // Initialize the Weave message layer.
    err = MessageLayer.Init(&initContext);
    if (err != WEAVE_NO_ERROR) {
        ESP_LOGE(TAG, "MessageLayer initialization failed: %s", ErrorStr(err));
        return false;
    }

    // Initialize the Weave exchange manager.
    err = ExchangeMgr.Init(&MessageLayer);
    if (err != WEAVE_NO_ERROR) {
        ESP_LOGE(TAG, "ExchangeMgr initialization failed: %s", ErrorStr(err));
        return false;
    }

    // Initialize the Weave security manager.
    err = SecurityMgr.Init(ExchangeMgr, SystemLayer);
    if (err != WEAVE_NO_ERROR) {
        ESP_LOGE(TAG, "SecurityMgr initialization failed: %s", ErrorStr(err));
        return false;
    }

    SecurityMgr.IdleSessionTimeout = 30000;
    SecurityMgr.SessionEstablishTimeout = 15000;

    // Initialize the CASE auth delegate object.
    if (!InitCASEAuthDelegate())
    {
        return false;
    }

#if WEAVE_CONFIG_SECURITY_TEST_MODE
    SecurityMgr.CASEUseKnownECDHKey = true;
#endif

    // Perform dynamic configuration of the Weave stack.
    err = ConfigMgr.ConfigureWeaveStack();
    if (err != WEAVE_NO_ERROR)
    {
        ESP_LOGE(TAG, "FabricState initialization failed: %s", ErrorStr(err));
        return false;
    }

    // Initialize the Connectivity Manager object.
    err = ConnectivityMgr.Init();
    if (err != WEAVE_NO_ERROR)
    {
        ESP_LOGE(TAG, "Connectivity Manager initialization failed: %s", ErrorStr(err));
        return false;
    }

    // Initialize the Weave server objects.
    bool serverInitSuccessful = (InitEchoServer() &&
                                 InitDeviceDescriptionServer() &&
                                 InitDeviceControlServer() &&
                                 InitFabricProvisioningServer() &&
                                 InitServiceProvisioningServer());

    if (serverInitSuccessful)
    {
        ESP_LOGI(TAG, "Weave stack initialized");
    }

    return serverInitSuccessful;
}

} // namespace WeavePlatform
