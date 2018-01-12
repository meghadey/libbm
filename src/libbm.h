#ifndef __LIB_LIBBM_H
#define __LIB_LIBBM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>

#define __LIBBM_MAX_COUNTERS 2

/* lib struct */
struct libbm_data
{
        int group;
        int fd[__LIBBM_MAX_COUNTERS];
        struct perf_event_attr *attr;
        pid_t pid;
        int cpu;
};

/* libbm_initialize
 *
 * This function initializes the library.
 *
 * int pid - pass in gettid()/getpid() value, -1 for current process
 * int cpu - pass in cpuid to track, -1 for any
 *
 * return - libbm_data structure for use in future library calls
 */
struct libbm_data *
libbm_initialize(int pid, int cpu, int num_events);

/* libbm_readcounter
 *
 * This function reads an individual counter.
 *
 * struct libbm_data* pd - library structure obtained from libbm_initialize()
 * int counter - pass in a constant value
 *
 * Proper method of printing value:
 *
 *
 *      #define __STDC_FORMAT_MACROS
 *      #include <inttypes.h>
 *      #include <stdio.h>
 *
 *      ...
 *
 *      fprintf(stdout, "%" PRIu64 "\n", value);
 *
 *
 * return - 64 bit counter value
 */
uint64_t
libbm_readcounter(struct libbm_data *pd, int counter);

/* libbm_enablecounter
 *
 * This function enables an individual counter.
 *
 * struct libbm_data* pd - library structure obtained from libbm_initialize()
 * int counter - pass in a constant value
 *
 * return - same semantics as ioctl 
 */
int
libbm_enablecounter(struct libbm_data *pd, int counter);

/* libbm_disablecounter
 *
 * This function disables an individual counter.
 *
 * struct libbm_data* pd - library structure obtained from libbm_initialize()
 * int counter - pass in a constant value
 *
 * return - same semantics as ioctl 
 */
int
libbm_disablecounter(struct libbm_data *pd, int counter);

/* libbm_close
 *
 * This function shuts down the library performing cleanup.
 * It should always be called when you're finished tracing to avoid memory
 * leaks.
 *
 * struct libbm_data* pd - library structure obtained from libbm_initialize()
 */
void
libbm_close(struct libbm_data *pd, int counter);

#ifdef __cplusplus
}
#endif
#endif /* __LIBROP_H */
