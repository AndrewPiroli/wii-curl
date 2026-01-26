#if defined(GEKKO)
#include <ogc/system.h>
#include <ogc/lwp_watchdog.h>

struct _hr_time
{
	u64 start;
};
unsigned long mbedtls_timing_get_timer(struct mbedtls_timing_hr_time *val, int reset)
{
    struct _hr_time *t = (struct _hr_time *) val;

    if (reset) {
	t->start = gettime();
        return 0;
    } else {
	return diff_msec(t->start, gettime());
    }
}
#endif
