#!/usr/bin/env python

#
#    Copyright (c) 2013-2017 Nest Labs, Inc.
#    All rights reserved.
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

#
#    @file
#      This file implements a Python script to generate a C/C++ header
#      for individual ASN1 Object IDs (OIDs) that are used in Weave
#      TLV encodings (notably the Weave Certificate object).
#

import sys

def identity(n):
    return n


# OID labels
ansi_X9_62              = identity
certicom                = identity
characteristicTwo       = identity
curve                   = identity
curves                  = identity
digest_algorithm        = identity
dod                     = identity
ds                      = identity
enterprise              = identity
organization            = identity
internet                = identity
iso                     = identity
itu_t                   = identity
joint_iso_ccitt         = identity
keyType                 = identity
mechanisms              = identity
member_body             = identity
nest                    = identity
pkcs1                   = identity
pkcs                    = identity
pkix                    = identity
prime                   = identity
private                 = identity
rsadsi                  = identity
schemes                 = identity
security                = identity
signatures              = identity
us                      = identity
weave                   = identity

# OID Categories
oidCategories = [
    ( "PubKeyAlgo",         0x0100 ),
    ( "SigAlgo",            0x0200 ),
    ( "AttributeType",      0x0300 ),
    ( "EllipticCurve",      0x0400 ),
    ( "Extension",          0x0500 ),
    ( "KeyPurpose",         0x0600 )
]

# Table of well-known ASN.1 object IDs
#
oids = [

    # !!! WARNING !!!
    #
    # The enumerated values associated with individual object IDs are used in Weave TLV encodings (notably the Weave Certificate object).
    # Because of this, the Enum Values assigned to object IDs in this table MUST NOT BE CHANGED once in use.


    #                                              Enum
    # Category          Name                       Value    Object ID
    # ----------------- -------------------------- -------- ------------------------------------------------------------------------------------------------

    # Public Key Algorithms
    ( "PubKeyAlgo",     "RSAEncryption",           1,       [ iso(1), member_body(2), us(840), rsadsi(113549), pkcs(1), pkcs1(1), 1 ]                       ),
    ( "PubKeyAlgo",     "ECPublicKey",             2,       [ iso(1), member_body(2), us(840), ansi_X9_62(10045), keyType(2), 1 ]                           ),
    ( "PubKeyAlgo",     "ECDH",                    3,       [ iso(1), organization(3), certicom(132), schemes(1), 12 ]                                      ),
    ( "PubKeyAlgo",     "ECMQV",                   4,       [ iso(1), organization(3), certicom(132), schemes(1), 13 ]                                      ),

    # Signature Algorithms
    # RFC 3279
    ( "SigAlgo",        "MD2WithRSAEncryption",    1,       [ iso(1), member_body(2), us(840), rsadsi(113549), pkcs(1), pkcs1(1), 2  ]                      ),
    ( "SigAlgo",        "MD5WithRSAEncryption",    2,       [ iso(1), member_body(2), us(840), rsadsi(113549), pkcs(1), pkcs1(1), 4  ]                      ),
    ( "SigAlgo",        "SHA1WithRSAEncryption",   3,       [ iso(1), member_body(2), us(840), rsadsi(113549), pkcs(1), pkcs1(1), 5  ]                      ),
    ( "SigAlgo",        "ECDSAWithSHA1",           4,       [ iso(1), member_body(2), us(840), ansi_X9_62(10045), signatures(4), 1 ]                        ),
    ( "SigAlgo",        "ECDSAWithSHA256",         5,       [ iso(1), member_body(2), us(840), ansi_X9_62(10045), signatures(4), 3, 2 ]                     ),
    # RFC 4231
    ( "SigAlgo",        "HMACWithSHA256",          6,       [ iso(1), member_body(2), us(840), rsadsi(113549), digest_algorithm(2), 9 ]                     ),
    # RFC 4055
    ( "SigAlgo",        "SHA256WithRSAEncryption", 7,       [ iso(1), member_body(2), us(840), rsadsi(113549), pkcs(1), pkcs1(1), 11 ]                      ),

    # X.509 Distinguished Name Attribute Types
    #   WARNING -- Assign no values higher than 127.
    ( "AttributeType",  "CommonName",              1,       [ joint_iso_ccitt(2), ds(5), 4, 3 ]                                                             ),
    ( "AttributeType",  "Surname",                 2,       [ joint_iso_ccitt(2), ds(5), 4, 4 ]                                                             ),
    ( "AttributeType",  "SerialNumber",            3,       [ joint_iso_ccitt(2), ds(5), 4, 5 ]                                                             ),
    ( "AttributeType",  "CountryName",             4,       [ joint_iso_ccitt(2), ds(5), 4, 6 ]                                                             ),
    ( "AttributeType",  "LocalityName",            5,       [ joint_iso_ccitt(2), ds(5), 4, 7 ]                                                             ),
    ( "AttributeType",  "StateOrProvinceName",     6,       [ joint_iso_ccitt(2), ds(5), 4, 8 ]                                                             ),
    ( "AttributeType",  "OrganizationName",        7,       [ joint_iso_ccitt(2), ds(5), 4, 10 ]                                                            ),
    ( "AttributeType",  "OrganizationalUnitName",  8,       [ joint_iso_ccitt(2), ds(5), 4, 11 ]                                                            ),
    ( "AttributeType",  "Title",                   9,       [ joint_iso_ccitt(2), ds(5), 4, 12 ]                                                            ),
    ( "AttributeType",  "Name",                    10,      [ joint_iso_ccitt(2), ds(5), 4, 41 ]                                                            ),
    ( "AttributeType",  "GivenName",               11,      [ joint_iso_ccitt(2), ds(5), 4, 42 ]                                                            ),
    ( "AttributeType",  "Initials",                12,      [ joint_iso_ccitt(2), ds(5), 4, 43 ]                                                            ),
    ( "AttributeType",  "GenerationQualifier",     13,      [ joint_iso_ccitt(2), ds(5), 4, 44 ]                                                            ),
    ( "AttributeType",  "DNQualifier",             14,      [ joint_iso_ccitt(2), ds(5), 4, 46 ]                                                            ),
    ( "AttributeType",  "Pseudonym",               15,      [ joint_iso_ccitt(2), ds(5), 4, 65 ]                                                            ),
    ( "AttributeType",  "DomainComponent",         16,      [ itu_t(0), 9, 2342, 19200300, 100, 1, 25 ]                                                     ),
    ( "AttributeType",  "WeaveDeviceId",           17,      [ iso(1), organization(3), dod(6), internet(1), private(4), enterprise(1), nest(41387), weave(1), 1 ] ),
    ( "AttributeType",  "WeaveServiceEndpointId",  18,      [ iso(1), organization(3), dod(6), internet(1), private(4), enterprise(1), nest(41387), weave(1), 2 ] ),
    ( "AttributeType",  "WeaveCAId",               19,      [ iso(1), organization(3), dod(6), internet(1), private(4), enterprise(1), nest(41387), weave(1), 3 ] ),
    ( "AttributeType",  "WeaveSoftwarePublisherId",20,      [ iso(1), organization(3), dod(6), internet(1), private(4), enterprise(1), nest(41387), weave(1), 4 ] ),

    # Elliptic Curves
    #   NOTE: The enumeration values assigned here must match the values assigned to WeaveCurveIds.
    ( "EllipticCurve",  "c2pnb163v1",              1,       [ iso(1), member_body(2), us(840), ansi_X9_62(10045), curves(3), characteristicTwo(0), 1 ]      ),
    ( "EllipticCurve",  "c2pnb163v2",              2,       [ iso(1), member_body(2), us(840), ansi_X9_62(10045), curves(3), characteristicTwo(0), 2 ]      ),
    ( "EllipticCurve",  "c2pnb163v3",              3,       [ iso(1), member_body(2), us(840), ansi_X9_62(10045), curves(3), characteristicTwo(0), 3 ]      ),
    ( "EllipticCurve",  "c2pnb176w1",              4,       [ iso(1), member_body(2), us(840), ansi_X9_62(10045), curves(3), characteristicTwo(0), 4 ]      ),
    ( "EllipticCurve",  "c2tnb191v1",              5,       [ iso(1), member_body(2), us(840), ansi_X9_62(10045), curves(3), characteristicTwo(0), 5 ]      ),
    ( "EllipticCurve",  "c2tnb191v2",              6,       [ iso(1), member_body(2), us(840), ansi_X9_62(10045), curves(3), characteristicTwo(0), 6 ]      ),
    ( "EllipticCurve",  "c2tnb191v3",              7,       [ iso(1), member_body(2), us(840), ansi_X9_62(10045), curves(3), characteristicTwo(0), 7 ]      ),
    ( "EllipticCurve",  "c2onb191v4",              8,       [ iso(1), member_body(2), us(840), ansi_X9_62(10045), curves(3), characteristicTwo(0), 8 ]      ),
    ( "EllipticCurve",  "c2onb191v5",              9,       [ iso(1), member_body(2), us(840), ansi_X9_62(10045), curves(3), characteristicTwo(0), 9 ]      ),
    ( "EllipticCurve",  "c2pnb208w1",              10,      [ iso(1), member_body(2), us(840), ansi_X9_62(10045), curves(3), characteristicTwo(0), 10 ]     ),
    ( "EllipticCurve",  "c2tnb239v1",              11,      [ iso(1), member_body(2), us(840), ansi_X9_62(10045), curves(3), characteristicTwo(0), 11 ]     ),
    ( "EllipticCurve",  "c2tnb239v2",              12,      [ iso(1), member_body(2), us(840), ansi_X9_62(10045), curves(3), characteristicTwo(0), 12 ]     ),
    ( "EllipticCurve",  "c2tnb239v3",              13,      [ iso(1), member_body(2), us(840), ansi_X9_62(10045), curves(3), characteristicTwo(0), 13 ]     ),
    ( "EllipticCurve",  "c2onb239v4",              14,      [ iso(1), member_body(2), us(840), ansi_X9_62(10045), curves(3), characteristicTwo(0), 14 ]     ),
    ( "EllipticCurve",  "c2onb239v5",              15,      [ iso(1), member_body(2), us(840), ansi_X9_62(10045), curves(3), characteristicTwo(0), 15 ]     ),
    ( "EllipticCurve",  "c2pnb272w1",              16,      [ iso(1), member_body(2), us(840), ansi_X9_62(10045), curves(3), characteristicTwo(0), 16 ]     ),
    ( "EllipticCurve",  "c2pnb304w1",              17,      [ iso(1), member_body(2), us(840), ansi_X9_62(10045), curves(3), characteristicTwo(0), 17 ]     ),
    ( "EllipticCurve",  "c2tnb359v1",              18,      [ iso(1), member_body(2), us(840), ansi_X9_62(10045), curves(3), characteristicTwo(0), 18 ]     ),
    ( "EllipticCurve",  "c2pnb368w1",              19,      [ iso(1), member_body(2), us(840), ansi_X9_62(10045), curves(3), characteristicTwo(0), 19 ]     ),
    ( "EllipticCurve",  "c2tnb431r1",              20,      [ iso(1), member_body(2), us(840), ansi_X9_62(10045), curves(3), characteristicTwo(0), 20 ]     ),
    ( "EllipticCurve",  "prime192v1",              21,      [ iso(1), member_body(2), us(840), ansi_X9_62(10045), curves(3), prime(1), 1 ]                  ),
    ( "EllipticCurve",  "prime192v2",              22,      [ iso(1), member_body(2), us(840), ansi_X9_62(10045), curves(3), prime(1), 2 ]                  ),
    ( "EllipticCurve",  "prime192v3",              23,      [ iso(1), member_body(2), us(840), ansi_X9_62(10045), curves(3), prime(1), 3 ]                  ),
    ( "EllipticCurve",  "prime239v1",              24,      [ iso(1), member_body(2), us(840), ansi_X9_62(10045), curves(3), prime(1), 4 ]                  ),
    ( "EllipticCurve",  "prime239v2",              25,      [ iso(1), member_body(2), us(840), ansi_X9_62(10045), curves(3), prime(1), 5 ]                  ),
    ( "EllipticCurve",  "prime239v3",              26,      [ iso(1), member_body(2), us(840), ansi_X9_62(10045), curves(3), prime(1), 6 ]                  ),
    ( "EllipticCurve",  "prime256v1",              27,      [ iso(1), member_body(2), us(840), ansi_X9_62(10045), curves(3), prime(1), 7 ]                  ),
    ( "EllipticCurve",  "secp112r1",               28,      [ iso(1), organization(3), certicom(132), curve(0), 6 ]                                         ),
    ( "EllipticCurve",  "secp112r2",               29,      [ iso(1), organization(3), certicom(132), curve(0), 7 ]                                         ),
    ( "EllipticCurve",  "secp128r1",               30,      [ iso(1), organization(3), certicom(132), curve(0), 28 ]                                        ),
    ( "EllipticCurve",  "secp128r2",               31,      [ iso(1), organization(3), certicom(132), curve(0), 29 ]                                        ),
    ( "EllipticCurve",  "secp160k1",               32,      [ iso(1), organization(3), certicom(132), curve(0), 9 ]                                         ),
    ( "EllipticCurve",  "secp160r1",               33,      [ iso(1), organization(3), certicom(132), curve(0), 8 ]                                         ),
    ( "EllipticCurve",  "secp160r2",               34,      [ iso(1), organization(3), certicom(132), curve(0), 30 ]                                        ),
    ( "EllipticCurve",  "secp192k1",               35,      [ iso(1), organization(3), certicom(132), curve(0), 31 ]                                        ),
    ( "EllipticCurve",  "secp224k1",               36,      [ iso(1), organization(3), certicom(132), curve(0), 32 ]                                        ),
    ( "EllipticCurve",  "secp224r1",               37,      [ iso(1), organization(3), certicom(132), curve(0), 33 ]                                        ),
    ( "EllipticCurve",  "secp256k1",               38,      [ iso(1), organization(3), certicom(132), curve(0), 10 ]                                        ),
    ( "EllipticCurve",  "secp384r1",               39,      [ iso(1), organization(3), certicom(132), curve(0), 34 ]                                        ),
    ( "EllipticCurve",  "secp521r1",               40,      [ iso(1), organization(3), certicom(132), curve(0), 35 ]                                        ),
    ( "EllipticCurve",  "sect113r1",               41,      [ iso(1), organization(3), certicom(132), curve(0), 4 ]                                         ),
    ( "EllipticCurve",  "sect113r2",               42,      [ iso(1), organization(3), certicom(132), curve(0), 5 ]                                         ),
    ( "EllipticCurve",  "sect131r1",               43,      [ iso(1), organization(3), certicom(132), curve(0), 22 ]                                        ),
    ( "EllipticCurve",  "sect131r2",               44,      [ iso(1), organization(3), certicom(132), curve(0), 23 ]                                        ),
    ( "EllipticCurve",  "sect163k1",               45,      [ iso(1), organization(3), certicom(132), curve(0), 1 ]                                         ),
    ( "EllipticCurve",  "sect163r1",               46,      [ iso(1), organization(3), certicom(132), curve(0), 2 ]                                         ),
    ( "EllipticCurve",  "sect163r2",               47,      [ iso(1), organization(3), certicom(132), curve(0), 15 ]                                        ),
    ( "EllipticCurve",  "sect193r1",               48,      [ iso(1), organization(3), certicom(132), curve(0), 24 ]                                        ),
    ( "EllipticCurve",  "sect193r2",               49,      [ iso(1), organization(3), certicom(132), curve(0), 25 ]                                        ),
    ( "EllipticCurve",  "sect233k1",               50,      [ iso(1), organization(3), certicom(132), curve(0), 26 ]                                        ),
    ( "EllipticCurve",  "sect233r1",               51,      [ iso(1), organization(3), certicom(132), curve(0), 27 ]                                        ),
    ( "EllipticCurve",  "sect239k1",               52,      [ iso(1), organization(3), certicom(132), curve(0), 3 ]                                         ),
    ( "EllipticCurve",  "sect283k1",               53,      [ iso(1), organization(3), certicom(132), curve(0), 16 ]                                        ),
    ( "EllipticCurve",  "sect283r1",               54,      [ iso(1), organization(3), certicom(132), curve(0), 17 ]                                        ),
    ( "EllipticCurve",  "sect409k1",               55,      [ iso(1), organization(3), certicom(132), curve(0), 36 ]                                        ),
    ( "EllipticCurve",  "sect409r1",               56,      [ iso(1), organization(3), certicom(132), curve(0), 37 ]                                        ),
    ( "EllipticCurve",  "sect571k1",               57,      [ iso(1), organization(3), certicom(132), curve(0), 38 ]                                        ),
    ( "EllipticCurve",  "sect571r1",               58,      [ iso(1), organization(3), certicom(132), curve(0), 39 ]                                        ),

    # Certificate Extensions
    ( "Extension",      "AuthorityKeyIdentifier",  1,       [ joint_iso_ccitt(2), ds(5), 29, 35 ]                                                           ),
    ( "Extension",      "SubjectKeyIdentifier",    2,       [ joint_iso_ccitt(2), ds(5), 29, 14 ]                                                           ),
    ( "Extension",      "KeyUsage",                3,       [ joint_iso_ccitt(2), ds(5), 29, 15 ]                                                           ),
    ( "Extension",      "BasicConstraints",        4,       [ joint_iso_ccitt(2), ds(5), 29, 19 ]                                                           ),
    ( "Extension",      "ExtendedKeyUsage",        5,       [ joint_iso_ccitt(2), ds(5), 29, 37 ]                                                           ),

    # Key Purposes
    ( "KeyPurpose",     "ServerAuth",              1,       [ iso(1), organization(3), dod(6), internet(1), security(5), mechanisms(5), pkix(7), 3, 1 ]     ),
    ( "KeyPurpose",     "ClientAuth",              2,       [ iso(1), organization(3), dod(6), internet(1), security(5), mechanisms(5), pkix(7), 3, 2 ]     ),
    ( "KeyPurpose",     "CodeSigning",             3,       [ iso(1), organization(3), dod(6), internet(1), security(5), mechanisms(5), pkix(7), 3, 3 ]     ),
    ( "KeyPurpose",     "EmailProtection",         4,       [ iso(1), organization(3), dod(6), internet(1), security(5), mechanisms(5), pkix(7), 3, 4 ]     ),
    ( "KeyPurpose",     "TimeStamping",            5,       [ iso(1), organization(3), dod(6), internet(1), security(5), mechanisms(5), pkix(7), 3, 8 ]     ),
    ( "KeyPurpose",     "OCSPSigning",             6,       [ iso(1), organization(3), dod(6), internet(1), security(5), mechanisms(5), pkix(7), 3, 9 ]     ),
]


def encodeOID(oid):

    assert len(oid) >= 2

    oid = [ (oid[0]*40 + oid[1]) ] + oid[2:]

    encodedOID = []
    for val in oid:
        val, byte = divmod(val, 128)
        seg = [ byte ]
        while val > 0:
            val, byte = divmod(val, 128)
            seg.insert(0, byte + 0x80)
        encodedOID += (seg)

    return encodedOID

print "/*"
print " *"
print " *    Copyright (c) 2019 Google LLC."
print " *    Copyright (c) 2013-2017 Nest Labs, Inc."
print " *    All rights reserved."
print " *"
print " *    Licensed under the Apache License, Version 2.0 (the \"License\");"
print " *    you may not use this file except in compliance with the License."
print " *    You may obtain a copy of the License at"
print " *"
print " *        http://www.apache.org/licenses/LICENSE-2.0"
print " *"
print " *    Unless required by applicable law or agreed to in writing, software"
print " *    distributed under the License is distributed on an \"AS IS\" BASIS,"
print " *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied."
print " *    See the License for the specific language governing permissions and"
print " *    limitations under the License."
print " *"
print " */"
print ""
print "/**"
print " *    @file"
print " *      ASN.1 Object ID Definitions"
print " *"
print " *      !!! WARNING !!! WARNING !!! WARNING !!!"
print " *"
print " *      DO NOT EDIT THIS FILE! This file is generated by the"
print " *      gen-oid-table.py script."
print " *"
print " *      To make changes, edit the script and re-run it to generate"
print " *      this file."
print " *"
print " */"
print ""
print "#ifndef ASN1OID_H_"
print "#define ASN1OID_H_"
print ""
print "enum OIDCategory"
print "{"
for (catName, catEnum) in oidCategories:
    print "    kOIDCategory_%s = 0x%04X," % (catName, catEnum)
print ""
print "    kOIDCategory_NotSpecified = 0,"
print "    kOIDCategory_Unknown = 0x0F00,"
print "    kOIDCategory_Mask = 0x0F00"
print "};"
print ""

print "typedef uint16_t OID;"
print ""

print "enum"
print "{"
for (catName, catEnum) in oidCategories:
    for (oidCatName, oidName, oidEnum, oid) in oids:
        if (oidCatName == catName):
            print "    kOID_%s_%s = 0x%04X," % (catName, oidName, catEnum + oidEnum)
    print ""
print "    kOID_NotSpecified = 0,"
print "    kOID_Unknown = 0xFFFF,"
print "    kOID_Mask = 0x00FF"
print "};"
print ""

print "struct OIDTableEntry"
print "{"
print "    OID EnumVal;"
print "    const uint8_t *EncodedOID;"
print "    uint16_t EncodedOIDLen;"
print "};"
print ""

print "struct OIDNameTableEntry"
print "{"
print "    OID EnumVal;"
print "    const char *Name;"
print "};"
print ""

print "extern const OIDTableEntry sOIDTable[];"
print "extern const OIDNameTableEntry sOIDNameTable[];"
print "extern const size_t sOIDTableSize;"
print ""

print "#ifdef ASN1_DEFINE_OID_TABLE"
print ""

for (catName, oidName, oidEnum, oid) in oids:
    print "static const uint8_t sOID_%s_%s[] = { %s };" % (catName, oidName, ", ".join([ "0x%02X" % (x) for x in encodeOID(oid) ]))
print ""

print "const OIDTableEntry sOIDTable[] ="
print "{"
oidTableSize = 0
for (catName, oidName, oidEnum, oid) in oids:
    print "    { kOID_%s_%s, sOID_%s_%s, sizeof(sOID_%s_%s) }," % (catName, oidName, catName, oidName, catName, oidName)
    oidTableSize += 1
print "    { kOID_NotSpecified, NULL, 0 }"
print "};"
print ""

print "const size_t sOIDTableSize = %d;" % (oidTableSize)
print ""

print "#endif // ASN1_DEFINE_OID_TABLE"
print ""

print "#ifdef ASN1_DEFINE_OID_NAME_TABLE"
print ""

print "const OIDNameTableEntry sOIDNameTable[] ="
print "{"
for (catName, oidName, oidEnum, oid) in oids:
    print "    { kOID_%s_%s, \"%s\" }," % (catName, oidName, oidName)
print "    { kOID_NotSpecified, NULL }"
print "};"
print ""

print "#endif // ASN1_DEFINE_OID_NAME_TABLE"
print ""

print ""
print "#endif // ASN1OID_H_"
