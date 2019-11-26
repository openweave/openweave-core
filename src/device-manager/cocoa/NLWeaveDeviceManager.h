/*
 *
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

/**
 *    @file
 *      This file defines NLWeaveDeviceManager interface
 *
 */

#import <Foundation/Foundation.h>
#import <CoreBluetooth/CoreBluetooth.h>
#import "NLWeaveErrorCodes.h"
#import "NLIdentifyDeviceCriteria.h"
#import "NLNetworkInfo.h"
#import "NLServiceInfo.h"

typedef void (^WDMCompletionBlock)(id owner, id data);
typedef void (^WDMFailureBlock)(id owner, NSError * error);

@interface NLWeaveDeviceManager : NSObject

@property (copy, readonly) NSString * name;
@property (readonly) CBPeripheral * blePeripheral;
@property (readonly) dispatch_queue_t resultCallbackQueue;
@property (weak) id owner;

/**
 *  @brief Disable default initializer inherited from NSObject
 */
- (instancetype)init NS_UNAVAILABLE;

/**
 *  @brief Close all connections gracifully.
 *
 *  The device manager would be ready for another connection after completion.
 */
- (void)Close:(WDMCompletionBlock)completionHandler failure:(WDMFailureBlock)failureHandler;

/**
 *  @brief Forcifully release all resources and destroy all references.
 *
 *  There is no way to revive this device manager after this call.
 */
- (void)Shutdown:(WDMCompletionBlock)completionHandler;

// ----- Device Information -----
- (WEAVE_ERROR)GetDeviceId:(uint64_t *)deviceId;
- (WEAVE_ERROR)GetDeviceMgrPtr:(long long *)deviceMgrPtr;
- (WEAVE_ERROR)GetDeviceAddress:(NSMutableString *)strAddr;

// ----- Connection Management -----

- (void)rendezvousWithDevicePairingCode:(NSString *)pairingCode
                             completion:(WDMCompletionBlock)completionBlock
                                failure:(WDMFailureBlock)failureBlock;

- (void)rendezvousWithDevicePairingCode:(NSString *)pairingCode
                 identifyDeviceCriteria:(NLIdentifyDeviceCriteria *)identifyDeviceCriteria
                             completion:(WDMCompletionBlock)completionBlock
                                failure:(WDMFailureBlock)failureBlock;

- (void)rendezvousWithDeviceAccessToken:(NSString *)accessToken
                 identifyDeviceCriteria:(NLIdentifyDeviceCriteria *)identifyDeviceCriteria
                             completion:(WDMCompletionBlock)completionBlock
                                failure:(WDMFailureBlock)failureBlock;

/**
    \defgroup PassiveRendezvous Passive Rendezvous

    Passive Rendezvous differs from Active in that the connection establishment phase is initiated before the
    identify phase by the provisionee. In addition, the provisioner rejects and drops initiated connections
    in which the Identify Response does not contain a Device Description matching what was expected.

    In a Passive Rendezvous scenario, the installed device already on a 15.4 PAN and Weave fabric (the "existing device")
    puts the PAN in joinable mode and listens passively for incoming TCP connections on the unsecured Weave port. While
    the PAN is joinable, a new device (the "joiner") may join in a provisional mode that directs all of their traffic to
    a specific port (in this case, the unsecured Weave port) on the host which made the network joinable. This traffic is
    unsecured at the link-layer, since by definition a provisionally-joined device does not have a copy of the PAN
    encryption keys. When its battery tab is pulled or it's activated by the user in some other manner, the joiner
    actively scans for joinable PANs. For each joinable PAN, the joiner provisionally joins the network and attempts to
    perform PASE authentication with the existing device on the unsecured Weave port. When the joiner device finds the
    right PAN, its PASE authentication attempt will succeed. At this point, the joiner and existing device will perform a
    secure key exchange at the Weave level, after which they may perform the rest of the pairing interaction over a
    secured channel.

    @{
 */

- (void)passiveRendezvousWithCompletion:(WDMCompletionBlock)completionBlock failure:(WDMFailureBlock)failureBlock;

- (void)passiveRendezvousWithDevicePairingCode:(NSString *)pairingCode
                                    completion:(WDMCompletionBlock)completionBlock
                                       failure:(WDMFailureBlock)failureBlock;

- (void)passiveRendezvousWithDeviceAccessToken:(NSString *)accessToken
                                    completion:(WDMCompletionBlock)completionBlock
                                       failure:(WDMFailureBlock)failureBlock;

/**
    @}
*/

/**
    \defgroup RemotePassiveRendezvous Remote Passive Rendezvous

    Remote Passive Rendezvous differs from Passive Rendezvous in that an assisting device acts as a relay
    for the provisionee, relaying messages between the provisionee and provisioner.

    Perform Remote Passive Rendezvous with PASE authentication for rendezvoused device. DM will attempt to
    authenticate each rendezvoused, would-be joiner using the given PASE credentials. If a device fails to
    authenticate, the DM will close its tunneled connection to that device and reconnect to the assisting device,
    starting over the RPR process to listen for new connections on its unsecured Weave port. This cycle repeats
    until either the rendezvous timeout expires or a joiner successfully authenticates.

    It is expected that this function will be used to perform RPR in the case of Thread-assisted pairing.

    If the variant with the IPAddress is used, the rendezvousAddress is the PAN IPv6 link local address of the
    joiner.  The address is formed by taking the Weave node id of the joiner, and appending it to the "FE80::" prefix.  Note
    that for fully Thread compliant networks it is more appropriate to use the rendezvousAddress of "::", as the link local
    addresses in Thread are chosen based on a random ID.

    @{
 */
- (void)remotePassiveRendezvousWithDevicePairingCode:(NSString *)pairingCode
                                           IPAddress:(NSString *)IPAddress
                                   rendezvousTimeout:(uint16_t)rendezvousTimeoutSec
                                   inactivityTimeout:(uint16_t)inactivityTimeoutSec
                                          completion:(WDMCompletionBlock)completionBlock
                                             failure:(WDMFailureBlock)failureBlock;

/**
    @}
*/

- (NSInteger)setRendezvousAddress:(NSString *)aRendezvousAddress;

- (void)identifyDevice:(WDMCompletionBlock)completionBlock failure:(WDMFailureBlock)failureBlock;

- (void)startDeviceEnumerationWithIdentifyDeviceCriteria:(NLIdentifyDeviceCriteria *)identifyDeviceCriteria
                                              completion:(WDMCompletionBlock)completionBlock
                                                 failure:(WDMFailureBlock)failureBlock;

- (void)stopDeviceEnumeration;

- (void)connectDevice:(uint64_t)deviceId
        deviceAddress:(NSString *)deviceAddress
           completion:(WDMCompletionBlock)completionBlock
              failure:(WDMFailureBlock)failureBlock;

- (void)reconnectDevice:(WDMCompletionBlock)completionBlock failure:(WDMFailureBlock)failureBlock;

- (void)connectBle:(CBPeripheral *)peripheral completion:(WDMCompletionBlock)completionBlock failure:(WDMFailureBlock)failureBlock;

- (void)connectBleWithPairingCode:(CBPeripheral *)peripheral
                      pairingCode:(NSString *)pairingCode
                       completion:(WDMCompletionBlock)completionBlock
                          failure:(WDMFailureBlock)failureBlock;

- (void)connectBleWithDeviceAccessToken:(CBPeripheral *)peripheral
                            accessToken:(NSString *)accessToken
                             completion:(WDMCompletionBlock)completionBlock
                                failure:(WDMFailureBlock)failureBlock;

- (BOOL)isConnected;

- (BOOL)isValidPairingCode:(NSString *)pairingCode;

- (void)getCameraAuthData:(NSString *)nonce completion:(WDMCompletionBlock)completionBlock failure:(WDMFailureBlock)failureBlock;

/**
    \defgroup NetworkProvisioning Network Provisioning
    @{
 */

- (void)scanNetworks:(NLNetworkType)networkType
          completion:(WDMCompletionBlock)completionBlock
             failure:(WDMFailureBlock)failureBlock;

- (void)addNetwork:(NLNetworkInfo *)nlNetworkInfo
        completion:(WDMCompletionBlock)completionBlock
           failure:(WDMFailureBlock)failureBlock;

- (void)updateNetwork:(NLNetworkInfo *)netInfo completion:(WDMCompletionBlock)completionBlock failure:(WDMFailureBlock)failureBlock;

- (void)removeNetwork:(NLNetworkID)networkId completion:(WDMCompletionBlock)completionBlock failure:(WDMFailureBlock)failureBlock;

- (void)getNetworks:(uint8_t)flags completion:(WDMCompletionBlock)completionBlock failure:(WDMFailureBlock)failureBlock;

- (void)enableNetwork:(NLNetworkID)networkId completion:(WDMCompletionBlock)completionBlock failure:(WDMFailureBlock)failureBlock;

- (void)disableNetwork:(NLNetworkID)networkId completion:(WDMCompletionBlock)completionBlock failure:(WDMFailureBlock)failureBlock;

- (void)testNetworkConnectivity:(NLNetworkID)networkId
                     completion:(WDMCompletionBlock)completionBlock
                        failure:(WDMFailureBlock)failureBlock;

- (void)getRendezvousMode:(WDMCompletionBlock)completionBlock failure:(WDMFailureBlock)failureBlock;

- (void)setRendezvousMode:(uint16_t)rendezvousFlags
               completion:(WDMCompletionBlock)completionBlock
                  failure:(WDMFailureBlock)failureBlock;

- (WEAVE_ERROR)setAutoReconnect:(BOOL)autoReconnect;

/**
    @}
 */

/**
    \defgroup FabricProvisioning Fabric Provisioning
    @{
 */

- (void)createFabric:(WDMCompletionBlock)completionBlock failure:(WDMFailureBlock)failureBlock;
- (void)leaveFabric:(WDMCompletionBlock)completionBlock failure:(WDMFailureBlock)failureBlock;
- (void)getFabricConfig:(WDMCompletionBlock)completionBlock failure:(WDMFailureBlock)failureBlock;

- (void)joinExistingFabric:(NSData *)fabricConfig
                completion:(WDMCompletionBlock)completionBlock
                   failure:(WDMFailureBlock)failureBlock;

/**
    @}
 */

/**
    \defgroup ServiceProvisioning Service Provisioning
    @{
 */

- (void)registerServicePairAccount:(NLServiceInfo *)nlServiceInfo
                        completion:(WDMCompletionBlock)completionBlock
                           failure:(WDMFailureBlock)failureBlock;

- (void)updateService:(NLServiceInfo *)nlServiceInfo
           completion:(WDMCompletionBlock)completionBlock
              failure:(WDMFailureBlock)failureBlock;

- (void)unregisterService:(uint64_t)serviceId completion:(WDMCompletionBlock)completionBlock failure:(WDMFailureBlock)failureBlock;

- (void)getLastNetworkProvisioningResult:(WDMCompletionBlock)completionBlock failure:(WDMFailureBlock)failureBlock;

/**
    @}
 */

/**
    \defgroup DeviceControl Device Control
    @{
 */

- (void)armFailSafe:(uint8_t)armMode
      failSafeToken:(uint32_t)failSafeToken
         completion:(WDMCompletionBlock)completionBlock
            failure:(WDMFailureBlock)failureBlock;

- (void)disarmFailSafe:(WDMCompletionBlock)completionBlock failure:(WDMFailureBlock)failureBlock;

- (void)resetConfig:(uint16_t)resetFlags completion:(WDMCompletionBlock)completionBlock failure:(WDMFailureBlock)failureBlock;

- (void)enableConnectionMonitor:(NSInteger)intervalMs
                        timeout:(NSInteger)timeoutMs
                     completion:(WDMCompletionBlock)completionBlock
                        failure:(WDMFailureBlock)failureBlock;

- (void)disableConnectionMonitor:(WDMCompletionBlock)completionBlock failure:(WDMFailureBlock)failureBlock;

- (void)startSystemTest:(uint32_t)profileId
                 testId:(uint32_t)testId
             completion:(WDMCompletionBlock)completionBlock
                failure:(WDMFailureBlock)failureBlock;

/**
    @}
 */

/**
    \defgroup TokenPairing Token Pairing
    @{
 */

- (void)pairToken:(NSData *)pairingToken completion:(WDMCompletionBlock)completionBlock failure:(WDMFailureBlock)failureBlock;

/**
    @}
 */

- (void)ping:(WDMCompletionBlock)completionBlock failure:(WDMFailureBlock)failureBlock;

// ----- Error Logging -----
- (NSString *)toErrorString:(WEAVE_ERROR)err;
//-(NSError *)toError:(WEAVE_ERROR)err;

@end
