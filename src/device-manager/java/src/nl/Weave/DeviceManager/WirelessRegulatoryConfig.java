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
public class WirelessRegulatoryConfig
{
    /** Active wireless regulatory domain.
     */
    public String RegDomain;

    /** Active operating location.
     */
    public WirelessOperatingLocation OpLocation;
    
    /** Supported regulatory domains
     */
    public String[] SupportedRegDomains;
    
    public WirelessRegulatoryConfig()
    {
        RegDomain = null;
        OpLocation = WirelessOperatingLocation.NotSpecified;
        SupportedRegDomains = null;
    }
    
    public static WirelessRegulatoryConfig Make(String regDomain, int opLocation, String[] supportedRegDomains)
    {
        WirelessRegulatoryConfig regConfig = new WirelessRegulatoryConfig();
        regConfig.RegDomain = regDomain;
        regConfig.OpLocation = WirelessOperatingLocation.fromVal(opLocation);
        regConfig.SupportedRegDomains = supportedRegDomains;
        return regConfig;
    }
};
