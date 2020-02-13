/*
 *
 *    Copyright (c) 2019 Google LLC
 *    Copyright (c) 2013-2017 Nest Labs, Inc.
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
 * Container for wireless regulatory configuration information.
 */
public final class WirelessRegulatoryConfig
{
    /** Active wireless regulatory domain.
     */
    public final String RegDomain;

    /** Active operating location.
     */
    public final WirelessOperatingLocation OpLocation;
    
    /** Supported regulatory domains
     */
    public final String[] SupportedRegDomains;
    
    public WirelessRegulatoryConfig(String regDomain, WirelessOperatingLocation opLocation)
    {
		this.RegDomain = regDomain;
		this.OpLocation = opLocation;
		this.SupportedRegDomains = new String[0];
    }

    public WirelessRegulatoryConfig(String regDomain, WirelessOperatingLocation opLocation, String[] supportedRegDomains)
    {
        this.RegDomain = regDomain;
        this.OpLocation = opLocation;
        this.SupportedRegDomains = supportedRegDomains.clone();
    }
    
    // Convenience constructor for JNI code
    WirelessRegulatoryConfig(String regDomain, int opLocation, String[] supportedRegDomains)
    {
        this.RegDomain = regDomain;
        this.OpLocation = WirelessOperatingLocation.fromVal(opLocation);
        this.SupportedRegDomains = supportedRegDomains;
    }
};
