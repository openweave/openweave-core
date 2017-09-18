/*
 *
 *    Copyright (c) 2014 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    This document is the property of Nest. It is considered
 *    confidential and proprietary information.
 *
 *    This document may not be reproduced or transmitted in any form,
 *    in whole or in part, without the express written permission of
 *    Nest.
 *
 */

/**
 *    @file
 *      This file defines Nest Labs Weave product identifiers.
 *
 *      Product identifiers are assigned and administered by vendors who
 *      have been assigned an official Weave vendor identifier by Nest Labs.
 *
 */

#ifndef NEST_LABS_WEAVE_PRODUCT_IDENTIFIERS_HPP
#define NEST_LABS_WEAVE_PRODUCT_IDENTIFIERS_HPP

namespace nl {

namespace Weave {

namespace Profiles {

namespace Vendor {

namespace Nestlabs {

namespace DeviceDescription {

//
// Nest Labs Weave Product Identifiers (16 bits max)
//

enum NestWeaveProductId
{
    kNestWeaveProduct_Diamond                    = 0x0001,
    kNestWeaveProduct_DiamondBackplate           = 0x0002,
    kNestWeaveProduct_Diamond2                   = 0x0003,
    kNestWeaveProduct_Diamond2Backplate          = 0x0004,
    kNestWeaveProduct_Topaz                      = 0x0005,
    kNestWeaveProduct_AmberBackplate             = 0x0006,
    kNestWeaveProduct_Amber                      = 0x0007,  // DEPRECATED -- Use kNestWeaveProduct_AmberHeatLink
    kNestWeaveProduct_AmberHeatLink              = 0x0007,
    kNestWeaveProduct_Topaz2                     = 0x0009,
    kNestWeaveProduct_Diamond3                   = 0x000A,
    kNestWeaveProduct_Diamond3Backplate          = 0x000B,
    kNestWeaveProduct_Quartz                     = 0x000D,
    kNestWeaveProduct_Amber2HeatLink             = 0x000F,
    kNestWeaveProduct_SmokyQuartz                = 0x0010,
    kNestWeaveProduct_Quartz2                    = 0x0011,
    kNestWeaveProduct_BlackQuartz                = 0x0012,
    kNestWeaveProduct_Onyx                       = 0x0014,
    kNestWeaveProduct_OnyxBackplate              = 0x0015,
};

}; // namespace DeviceDescription

}; // namespace Nestlabs

}; // namespace Vendor

}; // namesapce Profiles

}; // namespace Weave

}; // namespace nl

#endif // NEST_LABS_WEAVE_PRODUCT_IDENTIFIERS_HPP
