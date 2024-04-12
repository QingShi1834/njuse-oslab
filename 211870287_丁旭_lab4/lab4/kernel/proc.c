
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
							   proc.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
													Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proc.h"
#include "proto.h"
#include "string.h"

#include "global.h"

/*======================================================================*
							  schedule
 *======================================================================*/
PUBLIC void schedule()
{
	PROCESS *p;

	int flag = 1;

	while (flag)
	{
		process_count++;
		process_count %= NR_TASKS;

		p = proc_table + process_count;

		if (!p->isSourceValid)
		{
			continue;
		}

		if (p->time != -1)
		{
			if (p->time <= get_ticks())
			{
				p->time = -1;
			}
			else
			{
				continue;
			}
		}

		flag = 0;
		p_proc_ready = p;
	}
}

/*======================================================================*
						   sys_get_ticks
 *======================================================================*/
PUBLIC int sys_get_ticks()
{
	return ticks;
}

PUBLIC void sys_myPrintf(char *s, char color)
{
	disp_color_str(s, color);
	return;
}

PUBLIC void sys_P(SEMAPHORE *semaphore)
{
	if (semaphore->data <= 0)
	{
		p_proc_ready->isSourceValid = 0;
		semaphore->list[semaphore->queue_head] = p_proc_ready;
		semaphore->queue_head = (semaphore->queue_head + 1) % SEMAPHORE_WAIT_SIZE;
		wait();
	}
	semaphore->data--;
}
PUBLIC void sys_V(SEMAPHORE *semaphore)
{
	semaphore->data++;
	if (semaphore->queue_head != semaphore->queue_tail)
	{
		semaphore->list[semaphore->queue_tail]->isSourceValid = 1;
		semaphore->queue_tail = (semaphore->queue_tail + 1) % SEMAPHORE_WAIT_SIZE;
		
	}
}

PUBLIC void sys_sleep(int time)
{
	p_proc_ready->time = get_ticks() + time;
	wait();
}