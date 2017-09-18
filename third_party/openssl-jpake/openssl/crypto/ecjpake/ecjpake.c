/* ecjpake.c */
/*
 * Written by Evgeny Margolis (emargolis@nestlabs.com) for the OpenSSL project
 * 2015.
 */

#include "ecjpake.h"

#include <openssl/crypto.h>
#include <openssl/err.h>
#include <string.h>

#ifdef OPENSSL_IS_BORINGSSL
#include <assert.h>
#define OPENSSL_assert(TEST) (assert(TEST))
#endif

/*
 * In the definition, (xa, xb, xc, xd) are Alice's (x1, x2, x3, x4) or
 * Bob's (x3, x4, x1, x2).
 */

typedef struct {
    unsigned char *num;        /* Must be unique */
    size_t len;
} ECJPAKE_ID;

struct ECJPAKE_CTX {
    /* public values */
    ECJPAKE_ID local_id;
    ECJPAKE_ID peer_id;
    const EC_GROUP *group;     /* Elliptic Curve Group */
    EC_POINT *Gxc;             /* Alice's G*x3 or Bob's G*x1 */
    EC_POINT *Gxd;             /* Alice's G*x4 or Bob's G*x2 */
    /* secret values - should not be revealed publicly and
                       should be cleared when released */
    BIGNUM *secret;            /* The shared secret */
    BN_CTX *ctx;
    BIGNUM *xa;                /* Alice's x1 or Bob's x3 */
    BIGNUM *xb;                /* Alice's x2 or Bob's x4 */
    unsigned char key[SHA256_DIGEST_LENGTH]; /* The calculated (shared) key */
};

static int zkp_init(ECJPAKE_ZKP *zkp, const EC_GROUP *group)
{
    zkp->Gr = EC_POINT_new(group);
    if (zkp->Gr == NULL)
        return 0;
    zkp->b = BN_new();
    if (zkp->b == NULL)
        return 0;
    return 1;
}

static void zkp_release(ECJPAKE_ZKP *zkp)
{
    if (zkp->b != NULL)
        BN_free(zkp->b);
    if (zkp->Gr != NULL)
        EC_POINT_free(zkp->Gr);
}

#define step_part_init       ECJPAKE_STEP2_init
#define step_part_release    ECJPAKE_STEP2_release

int step_part_init(ECJPAKE_STEP_PART *p, const ECJPAKE_CTX *ctx)
{
    memset(p, 0, sizeof(*p));
    p->Gx = EC_POINT_new(ctx->group);
    if (p->Gx == NULL)
        goto err;
    if (!zkp_init(&p->zkpx, ctx->group))
        goto err;
    return 1;

err:
    ECJPAKEerr(ECJPAKE_F_STEP_PART_INIT, ERR_R_MALLOC_FAILURE);
    step_part_release(p);
    return 0;
}

void step_part_release(ECJPAKE_STEP_PART *p)
{
    zkp_release(&p->zkpx);
    if (p->Gx != NULL)
        EC_POINT_free(p->Gx);
}

int ECJPAKE_STEP1_init(ECJPAKE_STEP1 *s1, const ECJPAKE_CTX *ctx)
{
    if (!step_part_init(&s1->p1, ctx))
        return 0;
    if (!step_part_init(&s1->p2, ctx))
        return 0;
    return 1;
}

void ECJPAKE_STEP1_release(ECJPAKE_STEP1 *s1)
{
    step_part_release(&s1->p2);
    step_part_release(&s1->p1);
}

ECJPAKE_CTX *ECJPAKE_CTX_new(const EC_GROUP *group, const BIGNUM *secret,
                             const unsigned char *local_id_num,
                             const size_t local_id_len,
                             const unsigned char *peer_id_num,
                             const size_t peer_id_len)
{
    ECJPAKE_CTX *ctx = NULL;

    /* init ecjpake context */
    ctx = OPENSSL_malloc(sizeof(*ctx));
    if (ctx == NULL)
        goto err;
    memset(ctx, 0, sizeof(*ctx));

    /* init elliptic curve group */
    if (group == NULL)
        goto err;
    ctx->group = group;

    /* init local id */
    ctx->local_id.num = (unsigned char *)OPENSSL_malloc(local_id_len);
    if (ctx->local_id.num == NULL)
        goto err;
    memcpy(ctx->local_id.num, local_id_num, local_id_len);
    ctx->local_id.len = local_id_len;

    /* init peer id */
    ctx->peer_id.num = (unsigned char *)OPENSSL_malloc(peer_id_len);
    if (ctx->peer_id.num == NULL)
        goto err;
    memcpy(ctx->peer_id.num, peer_id_num, peer_id_len);
    ctx->peer_id.len = peer_id_len;

    /* init secret */
    ctx->secret = BN_dup(secret);
    if (ctx->secret == NULL)
        goto err;

    /* init remaining ecjpake context fields */
    ctx->Gxc = EC_POINT_new(ctx->group);
    if (ctx->Gxc == NULL)
        goto err;
    ctx->Gxd = EC_POINT_new(ctx->group);
    if (ctx->Gxd == NULL)
        goto err;
    ctx->xa = BN_new();
    if (ctx->xa == NULL)
        goto err;
    ctx->xb = BN_new();
    if (ctx->xb == NULL)
        goto err;
    ctx->ctx = BN_CTX_new();
    if (ctx->ctx == NULL)
        goto err;

    return ctx;

err:
    ECJPAKEerr(ECJPAKE_F_ECJPAKE_CTX_NEW, ERR_R_MALLOC_FAILURE);
    ECJPAKE_CTX_free(ctx);
    return NULL;
}

void ECJPAKE_CTX_free(ECJPAKE_CTX *ctx)
{
    if (ctx != NULL) {
        if (ctx->ctx != NULL)
            BN_CTX_free(ctx->ctx);
        if (ctx->xb != NULL)
            BN_clear_free(ctx->xb);
        if (ctx->xa != NULL)
            BN_clear_free(ctx->xa);
        if (ctx->Gxd != NULL)
            EC_POINT_free(ctx->Gxd);
        if (ctx->Gxc != NULL)
            EC_POINT_free(ctx->Gxc);
        if (ctx->secret != NULL)
            BN_clear_free(ctx->secret);
        if (ctx->peer_id.num != NULL)
            OPENSSL_free(ctx->peer_id.num);
        if (ctx->local_id.num != NULL)
            OPENSSL_free(ctx->local_id.num);
        OPENSSL_free(ctx);
    }
}

static void hashlength(SHA256_CTX *sha, size_t l)
{
    unsigned char b[2];

    OPENSSL_assert(l <= 0xffff);
    b[0] = l >> 8;
    b[1] = l & 0xff;
    SHA256_Update(sha, b, 2);
}

static int hashpoint_default(ECJPAKE_CTX *ctx, SHA256_CTX *sha,
                             const EC_POINT *point)
{
    size_t point_len;
    unsigned char *point_oct = NULL;
    int ret = 0;

    point_len = EC_POINT_point2oct(ctx->group, point,
                                   POINT_CONVERSION_UNCOMPRESSED, NULL, 0, NULL);
    if (point_len == 0)
        goto err;

    point_oct = (unsigned char *)OPENSSL_malloc(point_len);
    if (point_oct == NULL)
        goto err;

    point_len = EC_POINT_point2oct(ctx->group, point,
                                   POINT_CONVERSION_UNCOMPRESSED, point_oct,
                                   point_len, ctx->ctx);
    if (point_len == 0)
        goto err;

    hashlength(sha, point_len);
    SHA256_Update(sha, point_oct, point_len);
    ret = 1;

err:
    if (point_oct != NULL)
        OPENSSL_free(point_oct);
    return ret;
}

static ECJPAKE_HASHPOINT_FUNC_PTR hashpoint = &hashpoint_default;

void ECJPAKE_Set_HashECPoint(ECJPAKE_HASHPOINT_FUNC_PTR hashpoint_custom)
{
    hashpoint = hashpoint_custom;
}

/* h = hash(G, G*r, G*x, ecjpake_id) */
static int zkp_hash(ECJPAKE_CTX *ctx, BIGNUM *h, const EC_POINT *zkpG,
                    const ECJPAKE_STEP_PART *p, const int use_local_id)
{
    unsigned char md[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha;

    SHA256_Init(&sha);
    if (!hashpoint(ctx, &sha, zkpG))
        goto err;
    if (!hashpoint(ctx, &sha, p->zkpx.Gr))
        goto err;
    if (!hashpoint(ctx, &sha, p->Gx))
        goto err;
    if (use_local_id)
        SHA256_Update(&sha, ctx->local_id.num, ctx->local_id.len);
    else
        SHA256_Update(&sha, ctx->peer_id.num, ctx->peer_id.len);
    SHA256_Final(md, &sha);
    if (BN_bin2bn(md, SHA256_DIGEST_LENGTH, h) == NULL)
        goto err;
    return 1;

err:
    ECJPAKEerr(ECJPAKE_F_ZKP_HASH, ERR_R_MALLOC_FAILURE);
    return 0;
}

/* Generate random number in [1, n - 1] ( i.e. [1, n) ) */
static int genrand(BIGNUM *rnd, const BIGNUM *n)
{
    BIGNUM *nm1 = NULL;
    int ret = 0;

    nm1 = BN_new();
    if (nm1 == NULL)
        goto err;
    /* n - 1 */
    if (!BN_copy(nm1, n))
        goto err;
    if (!BN_sub_word(nm1, 1))
        goto err;
    /* random number in [0, n - 1) */
    if (!BN_rand_range(rnd, nm1))
        goto err;
    /* [1, n) */
    if (!BN_add_word(rnd, 1))
        goto err;
    ret = 1;

err:
    if (!ret)
        ECJPAKEerr(ECJPAKE_F_GENRAND, ERR_R_MALLOC_FAILURE);
    if (nm1 != NULL)
        BN_free(nm1);
    return ret;
}

/* Prove knowledge of x. (Note that p->Gx has already been calculated) */
static int generate_zkp(ECJPAKE_STEP_PART *p, const BIGNUM *x,
                        const EC_POINT *zkpG, ECJPAKE_CTX *ctx)
{
    BIGNUM *order = NULL;
    BIGNUM *r = NULL;
    BIGNUM *h = NULL;
    BIGNUM *t = NULL;
    int ret = 0;

    order = BN_new();
    if (order == NULL)
        goto err;
    if (!EC_GROUP_get_order(ctx->group, order, ctx->ctx))
        goto err;
    /* r in [1,n-1] */
    r = BN_new();
    if (r == NULL)
        goto err;
    if (!genrand(r, order))
        goto err;
    /* G * r */
    if (!EC_POINT_mul(ctx->group, p->zkpx.Gr, NULL, zkpG, r, ctx->ctx))
        goto err;
    /* h = hash(G, G * r, G * x, ecjpake_id) */
    h = BN_new();
    if (h == NULL)
        goto err;
    if (!zkp_hash(ctx, h, zkpG, p, 1))
        goto err;
    /* b = r - x*h */
    t = BN_new();
    if (t == NULL)
        goto err;
    if (!BN_mod_mul(t, x, h, order, ctx->ctx))
        goto err;
    if (!BN_mod_sub(p->zkpx.b, r, t, order, ctx->ctx))
        goto err;
    ret = 1;

err:
    if (!ret)
        ECJPAKEerr(ECJPAKE_F_GENERATE_ZKP, ERR_R_MALLOC_FAILURE);
    if (t != NULL)
        BN_free(t);
    if (h != NULL)
        BN_free(h);
    if (r != NULL)
        BN_free(r);
    if (order != NULL)
        BN_free(order);
    return ret;
}

static int verify_zkp(const ECJPAKE_STEP_PART *p, const EC_POINT *zkpG,
                      ECJPAKE_CTX *ctx)
{
    BIGNUM *h = NULL;
    EC_POINT *point1 = NULL;
    EC_POINT *point2 = NULL;
    int ret = 0;

    /* h = hash(G, G * r, G * x, ecjpake_id) */
    h = BN_new();
    if (h == NULL)
        goto err;
    if (!zkp_hash(ctx, h, zkpG, p, 0))
        goto err;
    /* point1 = G * b */
    point1 = EC_POINT_new(ctx->group);
    if (point1 == NULL)
        goto err;
    if (!EC_POINT_mul(ctx->group, point1, NULL, zkpG, p->zkpx.b, ctx->ctx))
        goto err;
    /* point2 = (G * x) * h = G * {h * x} */
    point2 = EC_POINT_new(ctx->group);
    if (point2 == NULL)
        goto err;
    if (!EC_POINT_mul(ctx->group, point2, NULL, p->Gx, h, ctx->ctx))
        goto err;
    /* point2 = point1 + point2 = G*{hx} + G*b = G*{hx+b} = G*r (allegedly) */
    if (!EC_POINT_add(ctx->group, point2, point1, point2, ctx->ctx))
        goto err;
    /* verify (point2 == G * r) */
    if (0 != EC_POINT_cmp(ctx->group, point2, p->zkpx.Gr, ctx->ctx))
    {
        ECJPAKEerr(ECJPAKE_F_VERIFY_ZKP, ECJPAKE_R_ZKP_VERIFY_FAILED);
        goto clean;
    }

    ret = 1;
    goto clean;

err:
    ECJPAKEerr(ECJPAKE_F_VERIFY_ZKP, ERR_R_MALLOC_FAILURE);
clean:
    if (point2 != NULL)
        EC_POINT_free(point2);
    if (point1 != NULL)
        EC_POINT_free(point1);
    if (h != NULL)
        BN_free(h);
    return ret;
}

static int step_part_generate(ECJPAKE_STEP_PART *p, const BIGNUM *x,
                              const EC_POINT *G, ECJPAKE_CTX *ctx)
{
    if (!EC_POINT_mul(ctx->group, p->Gx, NULL, G, x, ctx->ctx))
        goto err;
    if (!generate_zkp(p, x, G, ctx))
        goto err;
    return 1;

err:
    ECJPAKEerr(ECJPAKE_F_STEP_PART_GENERATE, ERR_R_MALLOC_FAILURE);
    return 0;
}

int ECJPAKE_STEP1_generate(ECJPAKE_STEP1 *send, ECJPAKE_CTX *ctx)
{
    BIGNUM *order = NULL;
    const EC_POINT *generator = NULL;
    int ret = 0;

    order = BN_new();
    if (order == NULL)
        goto err;
    if (!EC_GROUP_get_order(ctx->group, order, ctx->ctx))
        goto err;

    if (!genrand(ctx->xa, order))
        goto err;
    if (!genrand(ctx->xb, order))
        goto err;

    generator = EC_GROUP_get0_generator(ctx->group);
    if (!step_part_generate(&send->p1, ctx->xa, generator, ctx))
        goto err;
    if (!step_part_generate(&send->p2, ctx->xb, generator, ctx))
        goto err;

    ret = 1;

err:
    if (!ret)
        ECJPAKEerr(ECJPAKE_F_ECJPAKE_STEP1_GENERATE, ERR_R_MALLOC_FAILURE);
    if (order != NULL)
        BN_free(order);
    return ret;
}

/*-
 * Elliptic Curve Point Validity Check based on p. 25
 * http://cs.ucsb.edu/~koc/ccs130h/notes/ecdsa-cert.pdf
 */
static int EC_POINT_is_legal(const EC_POINT *point, const ECJPAKE_CTX *ctx)
{
    BIGNUM *point_x = NULL;
    BIGNUM *point_y = NULL;
    BIGNUM *p = NULL;
    BIGNUM *order = NULL;
    EC_POINT *tmp_point = NULL;
    int res = 0;

    /* 1. Verify that point is not at Infinity */
    if (EC_POINT_is_at_infinity(ctx->group, point))
        goto illegal_point;

    /* 2. Verify that point.X and point.Y are in the prime field */
    point_x = BN_new();
    if (point_x == NULL)
        goto err;
    point_y = BN_new();
    if (point_y == NULL)
        goto err;
    p = BN_new();
    if (p == NULL)
        goto err;
    if (!EC_POINT_get_affine_coordinates_GFp(ctx->group, point, point_x,
                                             point_y, ctx->ctx))
        goto err;
    if (!EC_GROUP_get_curve_GFp(ctx->group, p, NULL, NULL, ctx->ctx))
        goto err;
    if (BN_is_negative(point_x) || BN_is_negative(point_y) ||
        BN_cmp(point_x, p) >= 0 || BN_cmp(point_y, p) >= 0)
        goto illegal_point;

    /* 3. Check point lies on the curve */
    if (!EC_POINT_is_on_curve(ctx->group, point, ctx->ctx))
        goto illegal_point;

    /* 4. Check that point*n is at Infinity */
    order = BN_new();
    if (order == NULL)
        goto err;
    tmp_point = EC_POINT_new(ctx->group);
    if (tmp_point == NULL)
        goto err;
    if (!EC_GROUP_get_order(ctx->group, order, ctx->ctx))
        goto err;
    if (!EC_POINT_mul(ctx->group, tmp_point, NULL, point, order, ctx->ctx))
        goto err;
    if (!EC_POINT_is_at_infinity(ctx->group, tmp_point))
        goto illegal_point;

    res = 1;
    goto clean;

err:
    ECJPAKEerr(ECJPAKE_F_EC_POINT_IS_LEGAL, ERR_R_MALLOC_FAILURE);
    goto clean;
illegal_point:
    ECJPAKEerr(ECJPAKE_F_EC_POINT_IS_LEGAL, ECJPAKE_R_G_IS_NOT_LEGAL);
clean:
    if (tmp_point != NULL)
        EC_POINT_free(tmp_point);
    if (order != NULL)
        BN_free(order);
    if (p != NULL)
        BN_free(p);
    if (point_y != NULL)
        BN_free(point_y);
    if (point_x != NULL)
        BN_free(point_x);
    return res;
}

int ECJPAKE_STEP1_process(ECJPAKE_CTX *ctx, const ECJPAKE_STEP1 *received)
{

    /* check Gxc is a legal point on Elliptic Curve */
    if (!EC_POINT_is_legal(received->p1.Gx, ctx))
    {
        ECJPAKEerr(ECJPAKE_F_ECJPAKE_STEP1_PROCESS,
                   ECJPAKE_R_G_TO_THE_X3_IS_NOT_LEGAL);
        return 0;
    }

    /* check Gxd is a legal point on Elliptic Curve */
    if (!EC_POINT_is_legal(received->p2.Gx, ctx))
    {
        ECJPAKEerr(ECJPAKE_F_ECJPAKE_STEP1_PROCESS,
                   ECJPAKE_R_G_TO_THE_X4_IS_NOT_LEGAL);
        return 0;
    }

    /* verify ZKP(xc) */
    if (!verify_zkp(&received->p1, EC_GROUP_get0_generator(ctx->group), ctx))
    {
        ECJPAKEerr(ECJPAKE_F_ECJPAKE_STEP1_PROCESS,
                   ECJPAKE_R_VERIFY_X3_FAILED);
        return 0;
    }

    /* verify ZKP(xd) */
    if (!verify_zkp(&received->p2, EC_GROUP_get0_generator(ctx->group), ctx))
    {
        ECJPAKEerr(ECJPAKE_F_ECJPAKE_STEP1_PROCESS,
                   ECJPAKE_R_VERIFY_X4_FAILED);
        return 0;
    }

    /* Save the points we need for later */
    if (!EC_POINT_copy(ctx->Gxc, received->p1.Gx) ||
        !EC_POINT_copy(ctx->Gxd, received->p2.Gx))
    {
        ECJPAKEerr(ECJPAKE_F_ECJPAKE_STEP1_PROCESS, ERR_R_MALLOC_FAILURE);
        return 0;
    }

    return 1;
}

int ECJPAKE_STEP2_generate(ECJPAKE_STEP2 *send, ECJPAKE_CTX *ctx)
{
    EC_POINT *point = NULL;
    BIGNUM *order = NULL;
    BIGNUM *xbs = NULL;
    int ret = 0;

    /*-
     * X = G * {(xa + xc + xd) * xb * s}
     */
    point = EC_POINT_new(ctx->group);
    if (point == NULL)
        goto err;
    /* point = G * xa */
    if (!EC_POINT_mul(ctx->group, point, NULL,
                      EC_GROUP_get0_generator(ctx->group), ctx->xa, ctx->ctx))
        goto err;
    /* point = G * xa + G * xc = G * {xa + xc} */
    if (!EC_POINT_add(ctx->group, point, point, ctx->Gxc, ctx->ctx))
        goto err;
    /* point = G * {xa + xc} + G * xd = G * {xa + xc + xd} */
    if (!EC_POINT_add(ctx->group, point, point, ctx->Gxd, ctx->ctx))
        goto err;
    /* xbs = xb * s */
    order = BN_new();
    if (order == NULL)
        goto err;
    xbs = BN_new();
    if (xbs == NULL)
        goto err;
    if (!EC_GROUP_get_order(ctx->group, order, ctx->ctx))
        goto err;
    if (!BN_mod_mul(xbs, ctx->xb, ctx->secret, order, ctx->ctx))
        goto err;

    /*-
     * ZKP(xb * s)
     * For STEP2 the generator is:
     *     G' = G * {xa + xc + xd}
     * which means X is G' * {xb * s}
     *     X  = G' * {xb * s} = G * {(xa + xc + xd) * xb * s}
     */
    if (!step_part_generate(send, xbs, point, ctx))
        goto err;
    ret = 1;

err:
    if (!ret)
        ECJPAKEerr(ECJPAKE_F_ECJPAKE_STEP2_GENERATE, ERR_R_MALLOC_FAILURE);
    if (xbs != NULL)
        BN_clear_free(xbs);
    if (order != NULL)
        BN_free(order);
    if (point != NULL)
        EC_POINT_free(point);
    return ret;
}

/* Gx = G * {(xc + xa + xb) * xd * secret} */
static int compute_key(ECJPAKE_CTX *ctx, const EC_POINT *Gx)
{
    EC_POINT *point = NULL;
    SHA256_CTX sha;
    int ret = 0;

    /*-
     * K = (Gx - G * {xb * xd * secret}) * xb
     *   = (G * {(xc + xa + xb) * xd * secret - xb * xd * secret}) * xb
     *   = (G * {(xc + xa) * xd * secret}) * xb
     *   =  G * {(xa + xc) * xb * xd * secret}
     * [which is the same regardless of who calculates it]
     */

    /* point = (G * xd) * xb = G * {xb * xd} */
    point = EC_POINT_new(ctx->group);
    if (point == NULL)
        goto err;
    if (!EC_POINT_mul(ctx->group, point, NULL, ctx->Gxd, ctx->xb, ctx->ctx))
        goto err;
    /* point = - G * {xb * xd} */
    if (!EC_POINT_invert(ctx->group, point, ctx->ctx))
        goto err;
    /* point = - G * {xb * xd * secret} */
    if (!EC_POINT_mul(ctx->group, point, NULL, point, ctx->secret, ctx->ctx))
        goto err;
    /* point = Gx - G * {xb * xd * secret} */
    if (!EC_POINT_add(ctx->group, point, Gx, point, ctx->ctx))
        goto err;
    /* point = point * xb */
    if (!EC_POINT_mul(ctx->group, point, NULL, point, ctx->xb, ctx->ctx))
        goto err;
    /* Hash point to generate shared secret key */
    SHA256_Init(&sha);
    if (!hashpoint(ctx, &sha, point))
        goto err;
    SHA256_Final(ctx->key, &sha);
    ret = 1;

err:
    if (!ret)
        ECJPAKEerr(ECJPAKE_F_COMPUTE_KEY, ERR_R_MALLOC_FAILURE);
    if (point != NULL)
        EC_POINT_clear_free(point);
    return ret;
}

int ECJPAKE_STEP2_process(ECJPAKE_CTX *ctx, const ECJPAKE_STEP2 *received)
{
    BIGNUM *order = NULL;
    BIGNUM *tmp = NULL;
    EC_POINT *point = NULL;
    int ret = 0;

    /* Get Order */
    order = BN_new();
    if (order == NULL)
        goto err;
    if (!EC_GROUP_get_order(ctx->group, order, ctx->ctx))
        goto err;
    /* G' = G * {xc + xa + xb} */
    /* tmp = xa + xb */
    tmp = BN_new();
    if (tmp == NULL)
        goto err;
    if (!BN_mod_add(tmp, ctx->xa, ctx->xb, order, ctx->ctx))
        goto err;
    /* point = G * {xa + xb} */
    point = EC_POINT_new(ctx->group);
    if (point == NULL)
        goto err;
    if (!EC_POINT_mul(ctx->group, point, NULL,
                      EC_GROUP_get0_generator(ctx->group), tmp, ctx->ctx))
        goto err;
    /* point = G * {xc + xa + xb} */
    if (!EC_POINT_add(ctx->group, point, ctx->Gxc, point, ctx->ctx))
        goto err;
    /* Verify ZKP */
    if (!verify_zkp(received, point, ctx))
    {
        ECJPAKEerr(ECJPAKE_F_ECJPAKE_STEP2_PROCESS, ECJPAKE_R_VERIFY_X4S_FAILED);
        goto clean;
    }
    /* calculate shared secret (key) */
    if (!compute_key(ctx, received->Gx))
        goto err;

    ret = 1;
    goto clean;

err:
    ECJPAKEerr(ECJPAKE_F_ECJPAKE_STEP2_PROCESS, ERR_R_MALLOC_FAILURE);
clean:
    if (point != NULL)
        EC_POINT_free(point);
    if (tmp != NULL)
        BN_free(tmp);
    if (order != NULL)
        BN_free(order);
    return ret;
}

void ECJPAKE_STEP3A_init(ECJPAKE_STEP3A *s3a)
{
}

int ECJPAKE_STEP3A_generate(ECJPAKE_STEP3A *send, ECJPAKE_CTX *ctx)
{
    SHA256(ctx->key, sizeof ctx->key, send->hhk);
    SHA256(send->hhk, sizeof send->hhk, send->hhk);

    return 1;
}

int ECJPAKE_STEP3A_process(ECJPAKE_CTX *ctx, const ECJPAKE_STEP3A *received)
{
    unsigned char hhk[SHA256_DIGEST_LENGTH];

    SHA256(ctx->key, sizeof ctx->key, hhk);
    SHA256(hhk, sizeof hhk, hhk);
    if (memcmp(hhk, received->hhk, sizeof hhk)) {
        ECJPAKEerr(ECJPAKE_F_ECJPAKE_STEP3A_PROCESS,
                   ECJPAKE_R_HASH_OF_HASH_OF_KEY_MISMATCH);
        return 0;
    }
    return 1;
}

void ECJPAKE_STEP3A_release(ECJPAKE_STEP3A *s3a)
{
}

void ECJPAKE_STEP3B_init(ECJPAKE_STEP3B *s3b)
{
}

int ECJPAKE_STEP3B_generate(ECJPAKE_STEP3B *send, ECJPAKE_CTX *ctx)
{
    SHA256(ctx->key, sizeof(ctx->key), send->hk);
    return 1;
}

int ECJPAKE_STEP3B_process(ECJPAKE_CTX *ctx, const ECJPAKE_STEP3B *received)
{
    unsigned char hk[SHA256_DIGEST_LENGTH];

    SHA256(ctx->key, sizeof(ctx->key), hk);
    if (memcmp(hk, received->hk, sizeof(hk))) {
        ECJPAKEerr(ECJPAKE_F_ECJPAKE_STEP3B_PROCESS,
                   ECJPAKE_R_HASH_OF_KEY_MISMATCH);
        return 0;
    }
    return 1;
}

void ECJPAKE_STEP3B_release(ECJPAKE_STEP3B *s3b)
{
}

const EC_GROUP *ECJPAKE_get_ecGroup(const ECJPAKE_CTX *ctx)
{
    return ctx->group;
}

const unsigned char *ECJPAKE_get_shared_key(const ECJPAKE_CTX *ctx)
{
    return ctx->key;
}
