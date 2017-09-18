#include <openssl/opensslconf.h>

#ifdef OPENSSL_NO_ECJPAKE

# include <stdio.h>

int main(int argc, char *argv[])
{
    printf("No EC J-PAKE support\n");
    return(0);
}

#else

# include "../e_os.h"
# include <openssl/ecjpake.h>
# include <openssl/crypto.h>
# include <openssl/bio.h>
# include <openssl/rand.h>
# include <openssl/obj_mac.h>
# include <openssl/err.h>
# include <memory.h>

static void showkey(const char *name, const unsigned char *key)
{
    int i;

    fputs(name, stdout);
    fputs(" = ", stdout);
    for (i = 0; i < SHA256_DIGEST_LENGTH; i++)
        fprintf(stdout, "%02X", key[i]);
    putc('\n', stdout);
}

# if 0
static void showbn(const char *name, const BIGNUM *bn)
{
    fputs(name, stdout);
    fputs(" = ", stdout);
    BN_print_fp(stdout, bn);
    putc('\n', stdout);
}

static void showpoint(const char *name, const EC_POINT *point, ECJPAKE_CTX *ctx)
{
    BIGNUM *point_x = BN_new();
    BIGNUM *point_y = BN_new();
    BN_CTX *bn_ctx = BN_CTX_new();
    const EC_GROUP *group = ECJPAKE_get_ecGroup(ctx);

    if (bn_ctx == NULL || point_y == NULL || point_x == NULL)
        goto err;

    EC_POINT_get_affine_coordinates_GFp(group, point, point_x, point_y, bn_ctx);

    fputs(name, stdout);
    showbn(" X:", point_x);
    fputs(name, stdout);
    showbn(" Y:", point_y);

err:
    if (bn_ctx != NULL)
        BN_CTX_free(bn_ctx);
    if (point_y != NULL)
        BN_free(point_y);
    if (point_x != NULL)
        BN_free(point_x);
}
# endif

static const char rnd_seed[] =
    "string to make the random number generator think it has entropy";

/* generates random length value in a range [len_bottom, len_top] */
static int generate_rand_len(size_t *rand_len,
                             unsigned int len_bottom,
                             unsigned int len_top)
{
    if (len_top > 0xffff || len_bottom > len_top)
        return 0;

    if (!RAND_bytes((unsigned char *)(rand_len), sizeof(size_t)))
        return 0;

    *rand_len = *rand_len % (len_top - len_bottom + 1) + len_bottom;

    return 1;
}

static int run_ecjpake(ECJPAKE_CTX *alice, ECJPAKE_CTX *bob)
{
    ECJPAKE_STEP1 alice_s1;
    ECJPAKE_STEP1 bob_s1;
    ECJPAKE_STEP2 alice_s2;
    ECJPAKE_STEP2 bob_s2;
    ECJPAKE_STEP3A alice_s3a;
    ECJPAKE_STEP3B bob_s3b;

    /* Alice --> Bob: Step 1 */
    fputs("\tAlice --> Bob: Step 1\n", stdout);
    ECJPAKE_STEP1_init(&alice_s1, alice);
    ECJPAKE_STEP1_generate(&alice_s1, alice);

    if (!ECJPAKE_STEP1_process(bob, &alice_s1)) {
        ECJPAKE_STEP1_release(&alice_s1);
        fprintf(stderr, "Bob fails to process Alice's step 1\n");
        ERR_print_errors_fp(stdout);
        return 1;
    }
    ECJPAKE_STEP1_release(&alice_s1);

    /* Bob --> Alice: Step 1 */
    fputs("\tBob --> Alice: Step 1\n", stdout);
    ECJPAKE_STEP1_init(&bob_s1, bob);
    ECJPAKE_STEP1_generate(&bob_s1, bob);
    if (!ECJPAKE_STEP1_process(alice, &bob_s1)) {
        ECJPAKE_STEP1_release(&bob_s1);
        fprintf(stderr, "Alice fails to process Bob's step 1\n");
        ERR_print_errors_fp(stdout);
        return 2;
    }
    ECJPAKE_STEP1_release(&bob_s1);

    /* Alice --> Bob: Step 2 */
    fputs("\tAlice --> Bob: Step 2\n", stdout);
    ECJPAKE_STEP2_init(&alice_s2, alice);
    ECJPAKE_STEP2_generate(&alice_s2, alice);
    if (!ECJPAKE_STEP2_process(bob, &alice_s2)) {
        ECJPAKE_STEP2_release(&alice_s2);
        fprintf(stderr, "Bob fails to process Alice's step 2\n");
        ERR_print_errors_fp(stdout);
        return 3;
    }
    ECJPAKE_STEP2_release(&alice_s2);

    /* Bob --> Alice: Step 2 */
    fputs("\tBob --> Alice: Step 2\n", stdout);
    ECJPAKE_STEP2_init(&bob_s2, bob);
    ECJPAKE_STEP2_generate(&bob_s2, bob);
    if (!ECJPAKE_STEP2_process(alice, &bob_s2)) {
        ECJPAKE_STEP2_release(&bob_s2);
        fprintf(stderr, "Alice fails to process Bob's step 2\n");
        ERR_print_errors_fp(stdout);
        return 4;
    }
    ECJPAKE_STEP2_release(&bob_s2);

    showkey("\tAlice's key", ECJPAKE_get_shared_key(alice));
    showkey("\tBob's key  ", ECJPAKE_get_shared_key(bob));

    /* Alice --> Bob: Step 3A */
    fputs("\tAlice --> Bob: Step 3A\n", stdout);
    ECJPAKE_STEP3A_init(&alice_s3a);
    ECJPAKE_STEP3A_generate(&alice_s3a, alice);
    if (!ECJPAKE_STEP3A_process(bob, &alice_s3a)) {
        ECJPAKE_STEP3A_release(&alice_s3a);
        return 5;
    }
    ECJPAKE_STEP3A_release(&alice_s3a);

    /* Bob --> Alice: Step 3B */
    fputs("\tBob --> Alice: Step 3B\n", stdout);
    ECJPAKE_STEP3B_init(&bob_s3b);
    ECJPAKE_STEP3B_generate(&bob_s3b, bob);
    if (!ECJPAKE_STEP3B_process(alice, &bob_s3b)) {
        ECJPAKE_STEP3B_release(&bob_s3b);
        return 6;
    }
    ECJPAKE_STEP3B_release(&bob_s3b);

    return 0;
}

int main(int argc, char **argv)
{
    ECJPAKE_CTX *alice = NULL;
    ECJPAKE_CTX *bob = NULL;
    unsigned char *alice_id_num = NULL;
    unsigned char *bob_id_num = NULL;
    size_t alice_id_len;
    size_t bob_id_len;
    BIGNUM *secret = NULL;;
    BIGNUM *secret_wrong = NULL;
    size_t secret_len;
    EC_GROUP *group = NULL;
    BIO *bio_err;
    int i;
    int ret = 1;

    typedef struct test_curve {
        int nid;
        char *name;
    } test_curve;

    test_curve test_curves[] = {
        /* SECG PRIME CURVES TESTS */
        {NID_secp160r1, "SECG Prime-Curve P-160"},
        /* NIST PRIME CURVES TESTS */
        {NID_X9_62_prime192v1, "NIST Prime-Curve P-192"},
        {NID_secp224r1, "NIST Prime-Curve P-224"},
        {NID_X9_62_prime256v1, "NIST Prime-Curve P-256"},
        {NID_secp384r1, "NIST Prime-Curve P-384"},
        {NID_secp521r1, "NIST Prime-Curve P-521"},
# ifndef OPENSSL_NO_EC2M
        /* NIST BINARY CURVES TESTS */
        {NID_sect163k1, "NIST Binary-Curve K-163"},
        {NID_sect163r2, "NIST Binary-Curve B-163"},
        {NID_sect233k1, "NIST Binary-Curve K-233"},
        {NID_sect233r1, "NIST Binary-Curve B-233"},
        {NID_sect283k1, "NIST Binary-Curve K-283"},
        {NID_sect283r1, "NIST Binary-Curve B-283"},
        {NID_sect409k1, "NIST Binary-Curve K-409"},
        {NID_sect409r1, "NIST Binary-Curve B-409"},
        {NID_sect571k1, "NIST Binary-Curve K-571"},
        {NID_sect571r1, "NIST Binary-Curve B-571"},
# endif
    };

    CRYPTO_malloc_debug_init();
    CRYPTO_dbg_set_options(V_CRYPTO_MDEBUG_ALL);
    CRYPTO_mem_ctrl(CRYPTO_MEM_CHECK_ON);

# ifdef OPENSSL_SYS_WIN32
    CRYPTO_malloc_init();
# endif

    RAND_seed(rnd_seed, sizeof rnd_seed);

    bio_err = BIO_new(BIO_s_file());
    if (bio_err == NULL)
        EXIT(1);
    BIO_set_fp(bio_err, stdout, BIO_NOCLOSE);

    for (i = 0; i < sizeof(test_curves)/sizeof(test_curve); i++) {

        fprintf(stdout, "\nTesting ECJPAKE protocol for %s\n",
		test_curves[i].name);

        group = EC_GROUP_new_by_curve_name(test_curves[i].nid);
        if (group == NULL)
            goto err;

        /* randomize length of a secret in a range [32, 1024] bits */
        if (!generate_rand_len(&secret_len, 32, 512))
            goto err;
        /* randomize secret of secret_len bits */
        secret = BN_new();
	if (secret == NULL)
	    goto err;
        if (!BN_rand(secret, secret_len, 0, 0))
            goto err;
        /* randomize alice_id_len in a range [4, 128] bytes */
        if (!generate_rand_len(&alice_id_len, 4, 128))
            goto err;
        /* randomize alice_id_num of alice_id_len bytes */
        alice_id_num = (unsigned char *)OPENSSL_malloc(alice_id_len);
	if (alice_id_num == NULL)
            goto err;
        if (!RAND_bytes(alice_id_num, alice_id_len))
            goto err;
        /* randomize bob_id_len in a range [4, 128] bytes */
        if (!generate_rand_len(&bob_id_len, 4, 128))
            goto err;
        /* randomize bob_id_num of bob_id_len bytes */
        bob_id_num = (unsigned char *)OPENSSL_malloc(bob_id_len);
	if (bob_id_num == NULL)
            goto err;
        if (!RAND_bytes(bob_id_num, bob_id_len))
            goto err;

        /* Initialize ECJPAKE_CTX for Alice and Bob */
        alice = ECJPAKE_CTX_new(group, secret, alice_id_num, alice_id_len,
                                bob_id_num, bob_id_len);
        if (alice == NULL)
            goto err;
        bob = ECJPAKE_CTX_new(group, secret, bob_id_num, bob_id_len,
                              alice_id_num, alice_id_len);
        if (bob == NULL)
            goto err;

        fprintf(stdout, "Plain EC J-PAKE run\n");
        if (run_ecjpake(alice, bob) != 0) {
            fprintf(stderr, "Plain EC J-PAKE run failed\n");
            goto err;
        }

        ECJPAKE_CTX_free(bob);
        bob = NULL;
        ECJPAKE_CTX_free(alice);
        alice = NULL;

        /* Now give Alice and Bob different secrets */
        alice = ECJPAKE_CTX_new(group, secret, alice_id_num, alice_id_len,
                                bob_id_num, bob_id_len);
        if (alice == NULL)
            goto err;
        /* randomize secret_wrong of secret_len bits */
        secret_wrong = BN_new();
	if (secret_wrong == NULL)
	    goto err;
        if (!BN_rand(secret_wrong, secret_len, 0, 0))
            goto err;
        if (!BN_add(secret_wrong, secret_wrong, secret))
            goto err;
        bob = ECJPAKE_CTX_new(group, secret_wrong, bob_id_num, bob_id_len,
                              alice_id_num, alice_id_len);
        if (bob == NULL)
            goto err;

        fprintf(stdout, "Mismatch secret EC J-PAKE run\n");
        if (run_ecjpake(alice, bob) != 5) {
            fprintf(stderr, "Mismatched secret EC J-PAKE run failed\n");
            goto err;
        }

        ECJPAKE_CTX_free(bob);
        bob = NULL;
        ECJPAKE_CTX_free(alice);
        alice = NULL;
        BN_free(secret);
        secret = NULL;
        BN_free(secret_wrong);
        secret_wrong = NULL;
	OPENSSL_free(alice_id_num);
	alice_id_num = NULL;
	OPENSSL_free(bob_id_num);
	bob_id_num = NULL;
        EC_GROUP_free(group);
	group = NULL;
    }

    ret = 0;

err:
    if (ret)
        fprintf(stderr, "Exiting ecjpaketest with error.\n");
    if (bob != NULL)
        ECJPAKE_CTX_free(bob);
    if (alice != NULL)
        ECJPAKE_CTX_free(alice);
    if (secret != NULL)
        BN_free(secret);
    if (secret_wrong != NULL)
        BN_free(secret_wrong);
    if (alice_id_num != NULL)
	OPENSSL_free(alice_id_num);
    if (bob_id_num != NULL)
	OPENSSL_free(bob_id_num);
    if (group != NULL)
        EC_GROUP_free(group);
    BIO_free(bio_err);
    CRYPTO_cleanup_all_ex_data();
    ERR_remove_thread_state(NULL);
    CRYPTO_mem_leaks_fp(stderr);
    return ret;
}

#endif
