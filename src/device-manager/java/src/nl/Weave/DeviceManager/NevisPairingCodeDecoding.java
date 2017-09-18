/*
 *
 *    Copyright (c) 013-2017 Nest Labs, Inc.
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
package nl.Weave.DeviceManager;

/**
 * NevisPairingCodeDecoding enables decoding of pairing codes to yield a device ID.
 * 
 * @deprecated Please use methods on PairingCodeUtils instead.
 */
public class NevisPairingCodeDecoding
{
	/**
	 * Returns the device ID encoded in the pairing code, if valid.
	 * Otherwise, returns 0.
	 */
	public static long extractDeviceId(String pairingCode)
	{
	    return PairingCodeUtils.nevisPairingCodeToDeviceId(pairingCode);
	}

	/**
	 * Returns the pairing code decoded from the given Nevis device ID, if valid; otherwise, returns
	 * null
	 *
	 * @param deviceId the Nevis device ID (must begin with 0x18b430040)
	 */
	public static String extractPairingCode(long deviceId)
	{
	    return PairingCodeUtils.nevisDeviceIdToPairingCode(deviceId);
	}
}