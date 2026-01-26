#if defined(GEKKO)

#if !defined(MBEDTLS_DECLARE_PRIVATE_IDENTIFIERS)
#define WII_RESET_PRIVATE_IDENTS
#define MBEDTLS_DECLARE_PRIVATE_IDENTIFIERS
#endif
#if !defined(MBEDTLS_ALLOW_PRIVATE_ACCESS)
#define WII_RESET_PRIVATE_ACCESS
#define MBEDTLS_ALLOW_PRIVATE_ACCESS
#endif


#include <gccore.h>
#include <gctypes.h>
#include <network.h>
#include <ogc/system.h>
#include <ogc/lwp_watchdog.h>

#include "psa/crypto_driver_random.h"
#include "mbedtls/platform.h"
#include "mbedtls/platform_time.h"
#include "tf_psa_crypto_common.h"
#include "mbedtls/private/sha3.h"

int mbedtls_platform_get_entropy( psa_driver_get_entropy_flags_t flags,
                                  size_t *estimate_bits,
                                  unsigned char *output,
                                  size_t output_size )
{
    if( flags != 0 )
        return PSA_ERROR_NOT_SUPPORTED;

    static u64 seed = 0;
    static mbedtls_sha3_context ctx;
    static u8 buffer[28];

    mbedtls_sha3_init( &ctx );

    if( seed == 0 )
    {
        seed = (u64)gettime();

        u32 devid;
        if( ES_GetDeviceID( &devid ) < 0 )
            goto fail;
        seed ^= ((u64)devid) << 32;

        u64 mac;
        net_init();
        if( net_get_mac_address( (u8*)&mac ) < 0 )
            goto fail;
        seed ^= (mac << 1);

        if( mbedtls_sha3( MBEDTLS_SHA3_224, (u8*)&seed, sizeof(seed), buffer, sizeof(buffer) ) != 0 )
            goto fail;
    }

    size_t offset = 0;
    while( offset < output_size )
    {
        u32 currtime = gettime();

        mbedtls_sha3_starts( &ctx, MBEDTLS_SHA3_224 );
        mbedtls_sha3_update( &ctx, buffer, sizeof(buffer) );
        mbedtls_sha3_update( &ctx, (u8*)&currtime, sizeof(currtime) );
        mbedtls_sha3_finish( &ctx, buffer, sizeof(buffer) );

        size_t n = output_size - offset;
        if( n > sizeof(buffer) )
            n = sizeof(buffer);

        memcpy( output + offset, buffer, n );
        offset += n;
    }

    *estimate_bits = output_size * 8;
    return PSA_SUCCESS;

fail:
    return PSA_ERROR_INSUFFICIENT_ENTROPY;
}

mbedtls_ms_time_t mbedtls_ms_time(void)
{
        return ticks_to_millisecs(gettime());
}

#if defined(WII_RESET_PRIVATE_IDENTS)
#undef MBEDTLS_DECLARE_PRIVATE_IDENTIFIERS
#endif
#if defined(WII_RESET_PRIVATE_ACCESS)
#undef MBEDTLS_ALLOW_PRIVATE_ACCESS
#endif

#endif /* GEKKO */
