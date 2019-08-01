/*
 *
 *    Copyright (c) 2019 Google LLC.
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


#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>

#if WEAVE_DEVICE_CONFIG_ENABLE_SOFTWARE_UPDATE_MANAGER

#include <esp_system.h>
#include <esp_http_client.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/common/CommonProfile.h>

#include <Weave/DeviceLayer/internal/GenericSoftwareUpdateManagerImpl_BDX.ipp>
#include <Weave/DeviceLayer/internal/GenericSoftwareUpdateManagerImpl.ipp>

namespace nl {
namespace Weave {
namespace DeviceLayer {

SoftwareUpdateManagerImpl SoftwareUpdateManagerImpl::sInstance;

WEAVE_ERROR SoftwareUpdateManagerImpl::_Init(void)
{
    Internal::GenericSoftwareUpdateManagerImpl<SoftwareUpdateManagerImpl>::DoInit();
    Internal::GenericSoftwareUpdateManagerImpl_BDX<SoftwareUpdateManagerImpl>::DoInit();

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR SoftwareUpdateManagerImpl::StartImageDownload(char *aURI, uint64_t aStartOffset)
{
	esp_http_client_config_t config = {
	   .url = "http://httpbin.org/redirect/2",
	   .event_handler = _http_event_handle,
	};

	esp_http_client_handle_t client = esp_http_client_init(&config);

	esp_err_t err = esp_http_client_perform(client);

	if (err == ESP_OK) {
	   ESP_LOGI(TAG, "Status = %d, content_length = %d",
	           esp_http_client_get_status_code(client),
	           esp_http_client_get_content_length(client));
	}

	esp_http_client_cleanup(client);
}

esp_err_t SoftwareUpdateManagerImpl::_http_event_handle(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER");
            printf("%.*s", evt->data_len, (char*)evt->data);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (!esp_http_client_is_chunked_response(evt->client)) {
                printf("%.*s", evt->data_len, (char*)evt->data);
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
    }
    return ESP_OK;
}

WEAVE_ERROR SoftwareUpdateManagerImpl::GetUpdateSchemeList(::nl::Weave::Profiles::SoftwareUpdate::UpdateSchemeList * aUpdateSchemeList)
{
    uint8_t supportedSchemes[] = { Profiles::SoftwareUpdate::kUpdateScheme_Http , Profiles::SoftwareUpdate::kUpdateScheme_BDX };
    aUpdateSchemeList->init(ArraySize(supportedSchemes), supportedSchemes);

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR SoftwareUpdateManagerImpl::AbortDownload(void)
{
	// if (mESPHttpClient)
	{
		// If the client is active, shut it down.
	}

	return WEAVE_NO_ERROR;
}

} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

#endif // WEAVE_DEVICE_CONFIG_ENABLE_SOFTWARE_UPDATE_MANAGER