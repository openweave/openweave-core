/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package libcore.java.security;

import java.io.PrintStream;
import java.math.BigInteger;
import java.net.InetAddress;
import java.security.KeyPair;
import java.security.KeyPairGenerator;
import java.security.KeyStore.PasswordProtection;
import java.security.KeyStore.PrivateKeyEntry;
import java.security.KeyStore;
import java.security.PrivateKey;
import java.security.PublicKey;
import java.security.SecureRandom;
import java.security.Security;
import java.security.UnrecoverableKeyException;
import java.security.cert.X509Certificate;
import java.util.Collections;
import java.util.Date;
import java.util.Hashtable;
import javax.net.ssl.KeyManager;
import javax.net.ssl.KeyManagerFactory;
import javax.net.ssl.TrustManager;
import javax.net.ssl.TrustManagerFactory;
import junit.framework.Assert;
import org.bouncycastle.asn1.x509.BasicConstraints;
import org.bouncycastle.asn1.x509.X509Extensions;
import org.bouncycastle.jce.X509Principal;
import org.bouncycastle.jce.provider.BouncyCastleProvider;
import org.bouncycastle.x509.X509V3CertificateGenerator;

/**
 * TestKeyStore is a convenience class for other tests that
 * want a canned KeyStore with a variety of key pairs.
 *
 * Creating a key store is relatively slow, so a singleton instance is
 * accessible via TestKeyStore.get().
 */
public final class TestKeyStore extends Assert {

    static {
        if (StandardNames.IS_RI) {
            // Needed to create BKS keystore
            Security.addProvider(new BouncyCastleProvider());
        }
    }
    public final KeyStore keyStore;
    public final char[] storePassword;
    public final char[] keyPassword;
    public final KeyManager[] keyManagers;
    public final TrustManager[] trustManagers;

    private TestKeyStore(KeyStore keyStore,
                         char[] storePassword,
                         char[] keyPassword) {
        this.keyStore = keyStore;
        this.storePassword = storePassword;
        this.keyPassword = keyPassword;
        this.keyManagers = createKeyManagers(keyStore, storePassword);
        this.trustManagers = createTrustManagers(keyStore);
    }

    public static KeyManager[] createKeyManagers(final KeyStore keyStore,
                                                 final char[] storePassword) {
        try {
            String kmfa = KeyManagerFactory.getDefaultAlgorithm();
            KeyManagerFactory kmf = KeyManagerFactory.getInstance(kmfa);
            kmf.init(keyStore, storePassword);
            return kmf.getKeyManagers();
        } catch (Exception e) {
            throw new RuntimeException();
        }
    }

    public static TrustManager[] createTrustManagers(final KeyStore keyStore) {
        try {
            String tmfa = TrustManagerFactory.getDefaultAlgorithm();
            TrustManagerFactory tmf = TrustManagerFactory.getInstance(tmfa);
            tmf.init(keyStore);
            return tmf.getTrustManagers();
        } catch (Exception e) {
            throw new RuntimeException();
        }
    }

    private static final TestKeyStore ROOT_CA
            = create(new String[] { "RSA" },
                     null,
                     null,
                     "RootCA",
                     x509Principal("Test Root Certificate Authority"),
                     true,
                     null);
    private static final TestKeyStore INTERMEDIATE_CA
            = create(new String[] { "RSA" },
                     null,
                     null,
                     "IntermediateCA",
                     x509Principal("Test Intermediate Certificate Authority"),
                     true,
                     ROOT_CA);
    private static final TestKeyStore SERVER
            = create(new String[] { "RSA" },
                     null,
                     null,
                     "server",
                     localhost(),
                     false,
                     INTERMEDIATE_CA);
    private static final TestKeyStore CLIENT
            = new TestKeyStore(createClient(INTERMEDIATE_CA.keyStore), null, null);
    private static final TestKeyStore CLIENT_CERTIFICATE
            = create(new String[] { "RSA" },
                     null,
                     null,
                     "client",
                     x509Principal("test@user"),
                     false,
                     INTERMEDIATE_CA);

    private static final TestKeyStore ROOT_CA_2
            = create(new String[] { "RSA" },
                     null,
                     null,
                     "RootCA2",
                     x509Principal("Test Root Certificate Authority 2"),
                     true,
                     null);
    private static final TestKeyStore CLIENT_2
            = new TestKeyStore(createClient(ROOT_CA_2.keyStore), null, null);

    /**
     * Return a server keystore with a matched RSA certificate and
     * private key as well as a CA certificate.
     */
    public static TestKeyStore getServer() {
        return SERVER;
    }

    /**
     * Return a keystore with a CA certificate
     */
    public static TestKeyStore getClient() {
        return CLIENT;
    }

    /**
     * Return a client keystore with a matched RSA certificate and
     * private key as well as a CA certificate.
     */
    public static TestKeyStore getClientCertificate() {
        return CLIENT_CERTIFICATE;
    }

    /**
     * Return a keystore with a second CA certificate that does not
     * trust the server certificate returned by getServer for negative
     * testing.
     */
    public static TestKeyStore getClientCA2() {
        return CLIENT_2;
    }

    /**
     * Create a new KeyStore containing the requested key types.
     * Since key generation can be expensive, most tests should reuse
     * the RSA-only singleton instance returned by TestKeyStore.get
     *
     * @param keyAlgorithms The requested key types to generate and include
     * @param keyStorePassword Password used to protect the private key
     * @param aliasPrefix A unique prefix to identify the key aliases
     * @param ca true If the keys being created are for a CA
     * @param signer If non-null, key store used for signing key, otherwise self-signed
     */
    public static TestKeyStore create(String[] keyAlgorithms,
                                      char[] storePassword,
                                      char[] keyPassword,
                                      String aliasPrefix,
                                      X509Principal subject,
                                      boolean ca,
                                      TestKeyStore signer) {
        try {
            KeyStore keyStore = createKeyStore();
            for (String keyAlgorithm : keyAlgorithms) {
                String publicAlias  = aliasPrefix + "-public-"  + keyAlgorithm;
                String privateAlias = aliasPrefix + "-private-" + keyAlgorithm;
                createKeys(keyStore, keyPassword,
                           keyAlgorithm,
                           publicAlias, privateAlias,
                           subject,
                           ca,
                           signer);
            }
            if (signer != null) {
                copySelfSignedCertificates(keyStore, signer.keyStore);
            }
            return new TestKeyStore(keyStore, storePassword, keyPassword);
        } catch (RuntimeException e) {
            throw e;
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    /**
     * Create an empty BKS KeyStore
     *
     * The KeyStore is optionally password protected by the
     * keyStorePassword argument, which can be null if a password is
     * not desired.
     */
    public static KeyStore createKeyStore() throws Exception {
        KeyStore keyStore = KeyStore.getInstance("BKS");
        keyStore.load(null, null);
        return keyStore;
    }

    /**
     * Add newly generated keys of a given key type to an existing
     * KeyStore. The PrivateKey will be stored under the specified
     * private alias name. The X509Certificate will be stored on the
     * public alias name and have the given subject distiguished
     * name.
     *
     * If a CA is provided, it will be used to sign the generated
     * certificate. Otherwise, the certificate will be self
     * signed. The certificate will be valid for one day before and
     * one day after the time of creation.
     *
     * Based on:
     * org.bouncycastle.jce.provider.test.SigTest
     * org.bouncycastle.jce.provider.test.CertTest
     */
    public static KeyStore createKeys(KeyStore keyStore,
                                      char[] keyPassword,
                                      String keyAlgorithm,
                                      String publicAlias,
                                      String privateAlias,
                                      X509Principal subject,
                                      boolean ca,
                                      TestKeyStore signer) throws Exception {
        PrivateKey caKey;
        X509Certificate caCert;
        X509Certificate[] caCertChain;
        if (signer == null) {
            caKey = null;
            caCert = null;
            caCertChain = null;
        } else {
            PrivateKeyEntry privateKeyEntry
                    = privateKey(signer.keyStore, signer.keyPassword, keyAlgorithm);
            caKey = privateKeyEntry.getPrivateKey();
            caCert = (X509Certificate)privateKeyEntry.getCertificate();
            caCertChain = (X509Certificate[])privateKeyEntry.getCertificateChain();
        }

        PrivateKey privateKey;
        X509Certificate x509c;
        if (publicAlias == null && privateAlias == null) {
            // don't want anything apparently
            privateKey = null;
            x509c = null;
        } else {
            // 1.) we make the keys
            int keysize = 1024;
            KeyPairGenerator kpg = KeyPairGenerator.getInstance(keyAlgorithm);
            kpg.initialize(keysize, new SecureRandom());
            KeyPair kp = kpg.generateKeyPair();
            privateKey = (PrivateKey)kp.getPrivate();
            PublicKey publicKey  = (PublicKey)kp.getPublic();
            // 2.) use keys to make certficate

            // note that there doesn't seem to be a standard way to make a
            // certificate using java.* or javax.*. The CertificateFactory
            // interface assumes you want to read in a stream of bytes a
            // factory specific format. So here we use Bouncy Castle's
            // X509V3CertificateGenerator and related classes.
            X509Principal issuer;
            if (caCert == null) {
                issuer = subject;
            } else {
                issuer = (X509Principal) caCert.getSubjectDN();
            }

            long millisPerDay = 24 * 60 * 60 * 1000;
            long now = System.currentTimeMillis();
            Date start = new Date(now - millisPerDay);
            Date end = new Date(now + millisPerDay);
            BigInteger serial = BigInteger.valueOf(1);

            X509V3CertificateGenerator x509cg = new X509V3CertificateGenerator();
            x509cg.setSubjectDN(subject);
            x509cg.setIssuerDN(issuer);
            x509cg.setNotBefore(start);
            x509cg.setNotAfter(end);
            x509cg.setPublicKey(publicKey);
            x509cg.setSignatureAlgorithm("sha1With" + keyAlgorithm);
            x509cg.setSerialNumber(serial);
            if (ca) {
                x509cg.addExtension(X509Extensions.BasicConstraints,
                                    true,
                                    new BasicConstraints(true));
            }
            PrivateKey signingKey = (caKey == null) ? privateKey : caKey;
            x509c = x509cg.generateX509Certificate(signingKey);
        }

        X509Certificate[] x509cc;
        if (privateAlias == null) {
            // don't need certificate chain
            x509cc = null;
        } else if (caCertChain == null) {
            x509cc = new X509Certificate[] { x509c };
        } else {
            x509cc = new X509Certificate[caCertChain.length+1];
            x509cc[0] = x509c;
            System.arraycopy(caCertChain, 0, x509cc, 1, caCertChain.length);
        }

        // 3.) put certificate and private key into the key store
        if (privateAlias != null) {
            keyStore.setKeyEntry(privateAlias, privateKey, keyPassword, x509cc);
        }
        if (publicAlias != null) {
            keyStore.setCertificateEntry(publicAlias, x509c);
        }
        return keyStore;
    }

    /**
     * Create an X509Principal with the given attributes
     */
    public static X509Principal localhost() {
        try {
            return x509Principal(InetAddress.getLocalHost().getCanonicalHostName());
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    /**
     * Create an X509Principal with the given attributes
     */
    public static X509Principal x509Principal(String commonName) {
        Hashtable attributes = new Hashtable();
        attributes.put(X509Principal.CN, commonName);
        return new X509Principal(attributes);
    }

    /**
     * Return the only private key in a keystore for the given
     * algorithm. Throws IllegalStateException if there are are more
     * or less than one.
     */
    public static PrivateKeyEntry privateKey(KeyStore keyStore,
                                             char[] keyPassword,
                                             String algorithm) {
        try {
            PrivateKeyEntry found = null;
            PasswordProtection password = new PasswordProtection(keyPassword);
            for (String alias: Collections.list(keyStore.aliases())) {
                if (!keyStore.entryInstanceOf(alias, KeyStore.PrivateKeyEntry.class)) {
                    continue;
                }
                PrivateKeyEntry privateKey = (PrivateKeyEntry) keyStore.getEntry(alias, password);
                if (!privateKey.getPrivateKey().getAlgorithm().equals(algorithm)) {
                    continue;
                }
                if (found != null) {
                    throw new IllegalStateException("keyStore has more than one private key");
                }
                found = privateKey;
            }
            if (found == null) {
                throw new IllegalStateException("keyStore contained no private key");
            }
            return found;
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    /**
     * Create a client key store that only contains self-signed certificates but no private keys
     */
    public static KeyStore createClient(KeyStore caKeyStore) {
        try {
            KeyStore clientKeyStore = clientKeyStore = KeyStore.getInstance("BKS");
            clientKeyStore.load(null, null);
            copySelfSignedCertificates(clientKeyStore, caKeyStore);
            return clientKeyStore;
        } catch (RuntimeException e) {
            throw e;
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    /**
     * Copy self-signed certifcates from one key store to another
     */
    public static void copySelfSignedCertificates(KeyStore dst, KeyStore src) throws Exception {
        for (String alias: Collections.list(src.aliases())) {
            if (!src.isCertificateEntry(alias)) {
                continue;
            }
            X509Certificate cert = (X509Certificate)src.getCertificate(alias);
            if (!cert.getSubjectDN().equals(cert.getIssuerDN())) {
                continue;
            }
            dst.setCertificateEntry(alias, cert);
        }
    }

    /**
     * Dump a key store for debugging.
     */
    public static void dump(String context,
                            KeyStore keyStore,
                            char[] keyPassword) {
        try {
            PrintStream out = System.out;
            out.println("context=" + context);
            out.println("\tkeyStore=" + keyStore);
            out.println("\tkeyStore.type=" + keyStore.getType());
            out.println("\tkeyStore.provider=" + keyStore.getProvider());
            out.println("\tkeyPassword="
                        + ((keyPassword == null) ? null : new String(keyPassword)));
            out.println("\tsize=" + keyStore.size());
            for (String alias: Collections.list(keyStore.aliases())) {
                out.println("alias=" + alias);
                out.println("\tcreationDate=" + keyStore.getCreationDate(alias));
                if (keyStore.isCertificateEntry(alias)) {
                    out.println("\tcertificate:");
                    out.println("==========================================");
                    out.println(keyStore.getCertificate(alias));
                    out.println("==========================================");
                    continue;
                }
                if (keyStore.isKeyEntry(alias)) {
                    out.println("\tkey:");
                    out.println("==========================================");
                    String key;
                    try {
                        key = ("Key retreived using password\n"
                               + keyStore.getKey(alias, keyPassword).toString());
                    } catch (UnrecoverableKeyException e1) {
                        try {
                            key = ("Key retreived without password\n"
                                   + keyStore.getKey(alias, null).toString());
                        } catch (UnrecoverableKeyException e2) {
                            key = "Key could not be retreived";
                        }
                    }
                    out.println(key);
                    out.println("==========================================");
                    continue;
                }
                out.println("\tunknown entry type");
            }
        } catch (RuntimeException e) {
            throw e;
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    public static void assertChainLength(Object[] chain) {
        /*
         * Note chain is Object[] to support both
         * java.security.cert.X509Certificate and
         * javax.security.cert.X509Certificate
         */
        assertEquals(3, chain.length);
    }

}
