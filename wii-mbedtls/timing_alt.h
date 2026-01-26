#pragma once

#include "mbedtls/private_access.h"

#include "mbedtls/build_info.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
// Regular implementation
//

/**
 * \brief          timer structure
 */
struct mbedtls_timing_hr_time {
    uint64_t MBEDTLS_PRIVATE(opaque)[4];
};

/**
 * \brief          Context for mbedtls_timing_set/get_delay()
 */
typedef struct mbedtls_timing_delay_context {
    struct mbedtls_timing_hr_time   MBEDTLS_PRIVATE(timer);
    uint32_t                        MBEDTLS_PRIVATE(int_ms);
    uint32_t                        MBEDTLS_PRIVATE(fin_ms);
} mbedtls_timing_delay_context;

#ifdef __cplusplus
}
#endif
