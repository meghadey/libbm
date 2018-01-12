#include <assert.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stropts.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <linux/perf_event.h>
#include <poll.h>
#include "libbm.h"

/* perf_event_open syscall wrapper */
static long
sys_perf_event_open(struct perf_event_attr *hw_event,
		    pid_t pid, int cpu, int group_fd,
		    unsigned long flags)
{
	return syscall(__NR_perf_event_open, hw_event, pid, cpu,
		group_fd, flags);
}

__u64 get_type()
{
	FILE *fptr;
	char str[10];
	long type;

	fptr = fopen("/sys/devices/intel_bm/type", "r");

	if (fptr == NULL) {
		printf("Cannot open type file \n");
		exit(-1);
	}

	while (fscanf(fptr, "%s",str) != EOF) {
		type = strtoul(str,NULL,10);
	}

	fclose(fptr);

	return type;
}

__u64 get_guest_disable()
{
	unsigned int guest_disable;

	printf("Disable ROP events when operating at VMX non-root operation? (1-yes, 0-no)\n");

	if (!(scanf("%u", &guest_disable) == 1))
		printf("Failed to read guest_disable\n");

	if (guest_disable > 1) {
		printf("Value out of range. See README for acceptable values\n");
		exit(-1);
	}

	return guest_disable;
}

__u64 get_lbr_freeze()
{
	unsigned int lbr_freeze;

	printf("Enable LBR freeze on threshold trip? (1-yes, 0-no)\n");

	if (!(scanf("%u", &lbr_freeze) == 1))
		printf("Failed to read lbr_freeze\n");

	if (lbr_freeze > 1) {
		printf("Value out of range. See README for acceptable values\n");
		exit(-1);
	}

	return lbr_freeze;
}

__u64 get_window_size()
{
	unsigned int window_size;

	printf("Enter Window size (0 - 1023)\n");

	if (!(scanf("%u", &window_size) == 1))
		printf("Failed to read window_size\n");

	if (window_size > 1023) {
		printf("Value out of range. See README for acceptable values\n");
		exit(-1);
	}

	return window_size;
}

__u64 get_window_count_select()
{
	unsigned int window_count_select;

	printf("Window event count select: (0 - 3)\n");

	if (!(scanf("%u", &window_count_select) == 1))
		printf("Failed to read window_count_select\n");

	if (window_count_select > 3) {
		printf("Value out of range. See README for acceptable values\n");
		exit(-1);
	}

	return window_count_select;
}

__u64 get_count_and_mode()
{
	unsigned int count_and_mode;

	printf("Enable count AND mode? (1 or 0)\n");

	if (!(scanf("%u", &count_and_mode) == 1))
		printf("Failed to read count_and_mode\n");

	if (count_and_mode > 3) {
		printf("Value out of range. See README for acceptable values\n");
                exit(-1);
	}

	return count_and_mode;
}

__u64 get_threshold()
{
	unsigned int threshold;

	printf("Enter threshold (0 to 127)\n");

	if (!(scanf("%u", &threshold) == 1))
		printf("Failed to read threshold\n");

	if (threshold > 127) {
		printf("Value out of range. See README for acceptable values\n");
		exit(-1);
	}

	return threshold;
}

__u64 get_mispredict_event_count()
{
        int fd;
	unsigned int mispredict_event_count;

	printf("Enter mispredict_event_count:(0 or 1)\n");

	if (!(scanf("%u", &mispredict_event_count) == 1))
		printf("Failed to read mispredict_event_count\n");

	if (mispredict_event_count > 1) {
		printf("Value out of range. See README for acceptable values\n");
		exit(-1);
	}

	return mispredict_event_count;
}

__u64 get_event_type()
{
	unsigned int event_type;

	printf( "Enter a event type you want to monitor:\n");

	if (!(scanf("%d", &event_type) == 1))
		printf("Failed to read event_type.\n");

	if (event_type > 5) {
		printf("Value out of range. See README for acceptable values\n");
                exit(-1);;
        }

	return event_type;
}

__u64 get_bm_options(__u64 per_task_options)
{
        return per_task_options |
		(get_threshold() << 8) |
		(get_mispredict_event_count() << 15) |
		(get_event_type() << 1);
}

int get_num_events()
{
	unsigned int num_events;

	printf("Number of events to be monitored for this task(0,1 or 2):\n");

	if (!(scanf("%d", &num_events) == 1))
		printf("Failed to read num_events\n");

	if (num_events > 2) {
		printf("Maximum of 2 events supported per task\n");
		exit(-1);
	}

	if (!num_events)
		exit(0);

	return num_events;
}

/* thread safe, sets up a fd for profiling code to read from */
struct libbm_data * libbm_initialize(pid_t pid, int cpu, int num_events)
{
	int i, fd;
	struct libbm_data *pd;
	struct perf_event_attr *attr;
	 __u64 per_task_parameters;

	// We support only per task related events
	if(pid == -1)
		return (void *) (-EINVAL);

	attr = malloc(num_events * sizeof(struct perf_event_attr));;
	if(attr == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	pd = malloc(sizeof(struct libbm_data));
	if (pd == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	pd->group = -1;
	pd->pid = pid;
	pd->cpu = cpu;
	pd->attr = attr;

	memset(attr, 0, num_events * sizeof(struct perf_event_attr));

	if(num_events)
	{
		printf("Task parameters: (See README for details)\n");
                per_task_parameters = (get_guest_disable() << 35) |
                                        (get_lbr_freeze() << 34) |
                                        (get_window_size() << 40) |
                                        (get_window_count_select() << 56) |
                                        (get_count_and_mode() << 58);
        }

	for(i = 0; i < num_events; i++) {
		printf("Event %d/%d\n", i+1, num_events);
		attr[i].size = sizeof(struct perf_event_attr);
		attr[i].disabled = 1;         /* disable them now... */
		attr[i].exclude_kernel = 1;
		attr[i].config = get_bm_options(per_task_parameters);
		attr[i].type = get_type();
		attr[i].enable_on_exec = 1;

		pd->fd[i] = sys_perf_event_open(&attr[i], pid, cpu, -1, 0);
		if (pd->fd[i] < 0) {
			fprintf(stderr, "error at event %d/%d\n", i+1, num_events);
			perror("sys_perf_event_open");
			exit(EXIT_FAILURE);
		}
	}
	return pd;
}

int libbm_enablecounter(struct libbm_data *pd, int counter)
{
	if (pd->fd[counter] == -1)
		assert((pd->fd[counter] = sys_perf_event_open(&(pd->attr[counter]), pd->pid,
					pd->cpu, pd->group, 0)) != -1);

	return ioctl(pd->fd[counter], PERF_EVENT_IOC_ENABLE);
}

int libbm_disablecounter(struct libbm_data *pd, int counter)
{
	if (pd->fd[counter] == -1)
		return 0;

	return ioctl(pd->fd[counter], PERF_EVENT_IOC_DISABLE);
}

void libbm_close(struct libbm_data *pd, int counter)
{
	assert(pd->fd[counter] >= 0);
	close(pd->fd[counter]);
}

void libbm_finalize(struct libbm_data *pd)
{
	free(pd->attr);
	free(pd);
}
