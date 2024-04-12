
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
							main.c
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
							kernel_main
 *======================================================================*/
PUBLIC int kernel_main()
{
	disp_str("-----\"kernel_main\" begins-----\n");

	TASK *p_task = task_table;
	PROCESS *p_proc = proc_table;
	char *p_task_stack = task_stack + STACK_SIZE_TOTAL;
	u16 selector_ldt = SELECTOR_LDT_FIRST;
	int i;
	for (i = 0; i < NR_TASKS; i++)
	{
		strcpy(p_proc->p_name, p_task->name); // name of the process
		p_proc->pid = i;					  // pid

		p_proc->ldt_sel = selector_ldt;

		memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],
			   sizeof(DESCRIPTOR));
		p_proc->ldts[0].attr1 = DA_C | PRIVILEGE_TASK << 5;
		memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],
			   sizeof(DESCRIPTOR));
		p_proc->ldts[1].attr1 = DA_DRW | PRIVILEGE_TASK << 5;
		p_proc->regs.cs = ((8 * 0) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.ds = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.es = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.fs = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.ss = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.gs = (SELECTOR_KERNEL_GS & SA_RPL_MASK) | RPL_TASK;

		p_proc->regs.eip = (u32)p_task->initial_eip;
		p_proc->regs.esp = (u32)p_task_stack;
		p_proc->regs.eflags = 0x1202; /* IF=1, IOPL=1 */

		p_task_stack -= p_task->stacksize;
		p_proc++;
		p_task++;
		selector_ldt += 1 << 3;

		proc_table[i].time = -1;
		proc_table[i].isSourceValid = 1;
		proc_table[i].state = RESTING;
		proc_table[i].pid = i;
		proc_table[i].num = 0;
	}
	k_reenter = 0; // 中断处理程序开始时++ 结束时--
	ticks = 0;

	process_count = 0;
	p_proc_ready = proc_table;
	disp_pos = 0;
	for (int i = 0; i < 80 * 25; i++)
	{
		disp_str(" ");
	}
	disp_pos = 0;

	loop_count = 1;

	// initReadFirst();
	// initWriteFirst();
	initProduceAndConsume();

	/* 初始化 8253 PIT */
	out_byte(TIMER_MODE, RATE_GENERATOR);
	out_byte(TIMER0, (u8)(TIMER_FREQ / HZ)); // 时钟间隔是10 ms
	out_byte(TIMER0, (u8)((TIMER_FREQ / HZ) >> 8));

	put_irq_handler(CLOCK_IRQ, clock_handler); /* 设定时钟中断处理程序 */
	enable_irq(CLOCK_IRQ);					   /* 让8259A可以接收时钟中断 */

	restart(); // 进程调度 启动第一个进程的入口

	while (1)
	{
	}
}

/*======================================================================*
							   TestA 普通进程
 *======================================================================*/
void TestA()
{
	while (1)
	{
		switch (mode)
		{
		case READ_WRITE_READFIRST:
		case READ_WRITE_WRITEFIRST:
			if (loop_count <= 20)
			{
				disp_int(loop_count);
				myPrintf(": ", WHITE);

				for (int i = 1; i < NR_TASKS; i++)
				{
					switch ((proc_table + i)->state)
					{
					case READING:
						myPrintf("O ", GREEN);
						break;
					case WAIT_READING:
						myPrintf("X ", RED);
						break;
					case WRITING:
						myPrintf("O ", GREEN);
						break;
					case WAIT_WRITING:
						myPrintf("X ", RED);
						break;
					case RESTING:
						myPrintf("Z ", BLUE);
						break;
					default:
						break;
					}
					milli_delay(9);
				}
				myPrintf("\n", WHITE);
				loop_count++;
			}
			wait();
			break;
		case CONSUMER_PRODUCER:
			if (loop_count <= 20)
			{
				disp_int(loop_count);
				myPrintf(": ", WHITE);
				for (int i = 1; i < NR_TASKS; i++)
				{
					disp_int((proc_table + i)->num);
					myPrintf(" ", WHITE);
				}
				milli_delay(9);
				myPrintf("\n", WHITE);
				loop_count++;
			}
			wait();
			break;
		default:
			break;
		}
	}
}

/*======================================================================*
							   TestB R1 P1
 *======================================================================*/
void TestB()
{
	while (1)
	{
		switch (mode)
		{
		case READ_WRITE_READFIRST:
			readFirst_read(2);
			break;
		case READ_WRITE_WRITEFIRST:
			writeFirst_read(2);
			break;
		case CONSUMER_PRODUCER:
			produce(1);
			break;
		default:
			break;
		}
	}
}

/*======================================================================*
							   TestC R2 P2
 *======================================================================*/
void TestC()
{
	while (1)
	{
		switch (mode)
		{
		case READ_WRITE_READFIRST:
			readFirst_read(3);
			break;
		case READ_WRITE_WRITEFIRST:
			writeFirst_read(3);
			break;
		case CONSUMER_PRODUCER:
			produce(2);
			break;
		default:
			break;
		}
	}
}
/*======================================================================*
							   TestD R3 C1
 *======================================================================*/
void TestD()
{
	while (1)
	{
		switch (mode)
		{
		case READ_WRITE_READFIRST:
			readFirst_read(3);
			break;
		case READ_WRITE_WRITEFIRST:
			writeFirst_read(3);
			break;
		case CONSUMER_PRODUCER:
			consume(1);
			break;
		default:
			break;
		}
	}
}
/*======================================================================*
							   TestE W1 C2
 *======================================================================*/
void TestE()
{
	while (1)
	{
		switch (mode)
		{
		case READ_WRITE_READFIRST:
			readFirst_write(3);
			break;
		case READ_WRITE_WRITEFIRST:
			writeFirst_write(3);
			break;
		case CONSUMER_PRODUCER:
			consume(2);
			break;
		default:
			break;
		}
	}
}
/*======================================================================*
							   TestF W2 C3
 *======================================================================*/
void TestF()
{
	while (1)
	{
		switch (mode)
		{
		case READ_WRITE_READFIRST:
			readFirst_write(4);
			break;
		case READ_WRITE_WRITEFIRST:
			writeFirst_write(4);
			break;
		case CONSUMER_PRODUCER:
			consume(2);
			break;
		default:
			break;
		}
	}
}

// time:需要的时间片数
void readFirst_read(int time)
{
	p_proc_ready->state = WAIT_READING;

	P(&r_mutex);
	P(&r_count_mutex);

	if (r_count == 0)
	{
		P(&w_mutex);
	}

	r_count++;
	V(&r_count_mutex);

	p_proc_ready->state = READING;

	for (int i = 0; i < time; i++)
	{
		wait();
	}

	P(&r_count_mutex);
	r_count--;
	if (r_count == 0)
	{
		V(&w_mutex);
	}
	V(&r_count_mutex);
	V(&r_mutex);
	p_proc_ready->state = RESTING;

	for (int i = 0; i < REST_TIME; i++)
	{
		wait();
	}
}

void readFirst_write(int time)
{
	p_proc_ready->state = WAIT_WRITING;
	P(&w_mutex);
	p_proc_ready->state = WRITING;

	for (int i = 0; i < time; i++)
	{
		wait();
	}

	V(&w_mutex);
	p_proc_ready->state = RESTING;
	for (int i = 0; i < REST_TIME; i++)
	{
		wait();
	}
}

void writeFirst_read(int time)
{
	p_proc_ready->state = WAIT_READING;
	P(&r_mutex);
	P(&r_w_mutex);
	P(&r_count_mutex);
	if (r_count == 0)
	{
		P(&w_mutex);
	}
	r_count++;
	V(&r_count_mutex);
	V(&r_w_mutex);

	p_proc_ready->state = READING;
	for (int i = 0; i < time; i++)
	{
		wait();
	}

	P(&r_count_mutex);
	r_count--;
	if (r_count == 0)
	{
		V(&w_mutex);
	}

	V(&r_count_mutex);
	V(&r_mutex);

	p_proc_ready->state = RESTING;
	for (int i = 0; i < REST_TIME; i++)
	{
		wait();
	}
}

void writeFirst_write(int time)
{
	p_proc_ready->state = WAIT_WRITING;
	P(&w_count_mutex);
	if (w_count == 0)
	{
		P(&r_w_mutex);
	}
	w_count++;
	V(&w_count_mutex);
	P(&w_mutex);
	p_proc_ready->state = WRITING;

	for (int i = 0; i < time; i++)
	{
		wait();
	}

	P(&w_count_mutex);
	w_count--;
	if (w_count == 0)
	{
		V(&r_w_mutex);
	}
	V(&w_count_mutex);
	V(&w_mutex);
	p_proc_ready->state = RESTING;
	for (int i = 0; i < REST_TIME; i++)
	{
		wait();
	}
}

void initSemaphore(SEMAPHORE *semaphore, int value)
{
	semaphore->queue_head = semaphore->queue_tail = 0;
	semaphore->data = value;
}

void initReadFirst()
{
	initSemaphore(&r_mutex, READER_NUM);
	initSemaphore(&w_mutex, 1);
	initSemaphore(&r_count_mutex, 1);
	mode = READ_WRITE_READFIRST;
	r_count = 0;
}

void initWriteFirst()
{
	initSemaphore(&r_mutex, READER_NUM);
	initSemaphore(&w_mutex, 1);
	initSemaphore(&r_w_mutex, 1);
	initSemaphore(&w_count_mutex, 1);
	initSemaphore(&r_count_mutex, 1);

	mode = READ_WRITE_WRITEFIRST;
	r_count = 0;
	w_count = 0;
}

void initProduceAndConsume()
{
	mode = CONSUMER_PRODUCER;
	initSemaphore(&full_mutex, 0);
	initSemaphore(&empty_mutex, CAPACITY);
	initSemaphore(&P1_mutex, 0);
	initSemaphore(&P2_mutex, 0);
}

void consume(int num)
{
	if (num == 1)
	{
		P(&P1_mutex);
	}
	else if (num == 2)
	{
		P(&P2_mutex);
	}

	P(&full_mutex);
	V(&empty_mutex);
	p_proc_ready->num++;
	wait();
}

void produce(int num)
{
	P(&empty_mutex);
	if (num == 1)
	{
		V(&P1_mutex);
	}
	else if (num == 2)
	{
		V(&P2_mutex);
	}
	V(&full_mutex);
	p_proc_ready->num++;
	wait();
}
