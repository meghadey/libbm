/******************************************************************************
 * libbm_example.c                                                           *
 *                                                                            *
 * This is an example program showing how to use libbm within C code.        *
 *                                                                            *
 * libbm interfaces with the kernel ROP(Return Oriented Programming) driver. *
 * Copyright (C) 2017 Megha Dey, Yu-Cheng Yu                                  *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software                *
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA              *
 * 02110-1301, USA.                                                           *
 ******************************************************************************/

#include <inttypes.h>           /* for PRIu64 definition */
#include <stdint.h>             /* for uint64_t and PRIu64 */
#include <stdio.h>              /* for printf family */
#include <stdlib.h>             /* for EXIT_SUCCESS definition */
#include <fcntl.h>
#include <poll.h>
#include "libbm.h"             /* standard libbm include */

int main(int argc, char *argv[])
{
	struct pollfd pfd[__LIBBM_MAX_COUNTERS];
	struct libbm_data *pd;
	int pipefd[2];
	int i, ret, num_events;
	char read_pipe, write_pipe = 1;

	num_events = get_num_events();

	if (pipe(pipefd) < 0) {
		perror("failed to create 'go' pipe");
		return -1;
	}

	pid_t pid = fork();

	if (pid == 0) {
		close(pipefd[1]);
		ret = read(pipefd[0], &read_pipe, 1);
		if (ret != 1)
			exit(ret);
		/* argv[1] is the app we want to monitor */
		execv(argv[1], (char **)argv);
		close(pipefd[0]);
		exit(-1);
	}

	/* init lib */
	pd = libbm_initialize(pid, -1, num_events);

	for(i = 0; i < num_events; i++)
		pfd[i] = (struct pollfd){pd->fd[i], POLLIN, 0};

	write(pipefd[1], &write_pipe, 1);

	/* wait for any of the file descriptors to become ready */
	ret = poll(pfd, num_events, -1);

	/* Disable Branch monitoring counters */
	for(i = 0; i < num_events; i++)
		libbm_disablecounter(pd, i);

	for(i = 0; i < num_events; i++)
		libbm_close(pd, i);

	libbm_finalize(pd);

	if (ret == -1) {
		perror ("poll");
		return 1;
	}
	/* poll returns 0 if timeout */
	if (!ret) {
		printf ("Timeout occured. No ROP attack.\n");
		return 0;
	}

	/* if any of the FDs becomes ready, ROP may have occured */
	if ((pfd[0].revents & POLLIN) || (pfd[1].revents & POLLIN))
		printf ("ROP attack detected from process with PID %d\n", pid);

	close(pipefd[0]);
	close(pipefd[1]);

	return EXIT_SUCCESS;
}
