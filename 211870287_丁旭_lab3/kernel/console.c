
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			      console.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
						    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*
	回车键: 把光标移到第一列
	换行键: 把光标前进到下一行
*/


#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"

PRIVATE void set_cursor(unsigned int position);

PRIVATE void set_video_start_addr(u32 addr);

PRIVATE void flush(CONSOLE *p_con);

// TODO
// 存储换行前后信息
struct LF_Node {
	unsigned int prev;
	unsigned int post;
};

PRIVATE int LF_top;
PRIVATE struct LF_Node LF_stack[SCREEN_SIZE / SCREEN_WIDTH];

PRIVATE int operation_top;
PRIVATE char op_stack[SCREEN_SIZE]; // 操作栈

PRIVATE int is_tab(unsigned int cur) { // 连续四个字符
	u8 *start = (u8 * )(V_MEM_BASE + cur * 2);
	return start[0] == TAB_CHAR && start[2] == TAB_CHAR && start[4] == TAB_CHAR && start[6] == TAB_CHAR;
}

PRIVATE int matched(unsigned int cur, char *text, int len) {
	unsigned int cursor = cur;
	for (int i = 0; i < len; i++) {
		u8 *start = (u8 * )(V_MEM_BASE + cursor * 2);
		if (is_tab(cursor)) {
			if (text[i] != '\t')
				return 0;
			cursor += 4;
			continue;
		} else if (*start != text[i]) {
			return 0;
		}
		cursor++;
	}
	return cursor - cur;
}

PRIVATE void turn_red(u8 *start, int len) {
	for (int i = 0; i < len; i++) {
		*(start + i * 2) = RED_CHAR_COLOR;
	}
}

PUBLIC void search(CONSOLE *p_con, char *text, int len) {
	for (unsigned int i = p_con->original_addr; i < p_con->cursor - len; i++) {
		u8 *start = (u8 * )(V_MEM_BASE + i * 2) + 1;
//		if (*start == RED_CHAR_COLOR) break; // 遇到了被搜索过文字
		int matched_len = matched(i, text, len);
		if (matched_len) {
			turn_red(start, matched_len);
//			i += t - 1;
		}
	}
}

PUBLIC void end_search(CONSOLE *p_con, int key_len) {
	for (int i = 0; i < key_len; i++) {
		out_char(p_con, '\b');
	}
	for (int i = p_con->original_addr; i < p_con->cursor; i++) {
		u8 *start = (u8 * )(V_MEM_BASE + i * 2) + 1;
		*start = DEFAULT_CHAR_COLOR;
	}
}


/*======================================================================*
			   init_screen
 *======================================================================*/
PUBLIC void init_screen(TTY *p_tty) {
	int nr_tty = p_tty - tty_table;
	p_tty->p_console = console_table + nr_tty;

	int v_mem_size = V_MEM_SIZE >> 1;    /* 显存总大小 (in WORD) */

	int con_v_mem_size = v_mem_size / NR_CONSOLES;
	p_tty->p_console->original_addr = nr_tty * con_v_mem_size;
	p_tty->p_console->v_mem_limit = con_v_mem_size;
	p_tty->p_console->current_start_addr = p_tty->p_console->original_addr;

	/* 默认光标位置在最开始处 */
	p_tty->p_console->cursor = p_tty->p_console->original_addr;
	/* 默认非查找模式 */
	p_tty->p_console->searchMode = 0;
	p_tty->p_console->blocked = 0;
	p_tty->p_console->rolling = 0;
	p_tty->p_console->cleaning = 0;

	if (nr_tty == 0) {
		/* 第一个控制台沿用原来的光标位置 */
		p_tty->p_console->cursor = disp_pos / 2;
		disp_pos = 0;
	}
	clean_screen(p_tty->p_console); // TODO: 清除先前的输出信息
}

PUBLIC void clean_screen(CONSOLE *p_con) { // TODO
	u8 *p_vmem = (u8 * )(V_MEM_BASE);
	for (int i = p_con->original_addr; i < p_con->cursor; ++i) {
		*p_vmem++ = ' ';
		*p_vmem++ = DEFAULT_CHAR_COLOR;
	}
	p_con->cursor = p_con->current_start_addr = p_con->original_addr;
	LF_top = operation_top = -1;
	flush(p_con);
}


/*======================================================================*
			   is_current_console
*======================================================================*/
PUBLIC int is_current_console(CONSOLE *p_con) {
	return (p_con == &console_table[nr_current_console]);
}


/*======================================================================*
			   out_char
 *======================================================================*/
PUBLIC void out_char(CONSOLE *p_con, char ch) {
	u8 *p_vmem = (u8 * )(V_MEM_BASE + p_con->cursor * 2);
	switch (ch) {
		case '\n': {
			if (p_con->cursor < p_con->original_addr + p_con->v_mem_limit - SCREEN_WIDTH) {
				LF_stack[++LF_top].prev = p_con->cursor; // TODO: 记录换行开始位置
				p_con->cursor = p_con->original_addr +
				                SCREEN_WIDTH * ((p_con->cursor - p_con->original_addr) / SCREEN_WIDTH + 1);
				LF_stack[LF_top].post = p_con->cursor; // TODO: 记录换行结束位置
				if (!p_con->rolling) {
					op_stack[++operation_top] = '\b';
				}
			}
			break;
		}
		case '\b': { // TODO: 退格
			if (p_con->cursor > p_con->original_addr) { // 判断退格什么字符并存入栈
				char ch;
				if (is_tab(p_con->cursor - TAB_LEN)) { // 退格tab
					ch = '\t';
					for (int i = 1; i <= TAB_LEN; i++) {
						p_con->cursor--;
						*(p_vmem - 2 * i) = ' ';
						*(p_vmem - (2 * i) + 1) = DEFAULT_CHAR_COLOR;
					}
				} else if (p_con->cursor == LF_stack[LF_top].post) { // 退格 LF
					ch = '\n';
					p_con->cursor = LF_stack[LF_top].prev;
					LF_top--;
				} else { // 退格其他字符
					p_con->cursor--;
					ch = *(p_vmem - 2);
					*(p_vmem - 2) = ' ';
					*(p_vmem - 1) = DEFAULT_CHAR_COLOR;
				}
				if (!p_con->rolling) {
					op_stack[++operation_top] = ch;
				}
				if (p_con->searchMode) {

				}
			}
			break;
		}
		case '\t': { // TODO: Tab，输入四个空格，保存加入四个空格后的 p_con->cursor 的值
			if (p_con->cursor < p_con->original_addr + p_con->v_mem_limit - TAB_LEN) {
				for (int i = 0; i < TAB_LEN; i++) {
					*p_vmem++ = TAB_CHAR; // 使用 TAB_CHAR 而非 ' ' 以区分 '\t' 和 ' '
					*p_vmem++ = DEFAULT_CHAR_COLOR;
					p_con->cursor++;
				}
				if (!p_con->rolling) {
					op_stack[++operation_top] = '\b';
				}
			}
			break;
		}
		default: {
			u8 color = DEFAULT_CHAR_COLOR; // TODO: 默认黑底白字
			if (p_con->searchMode) { // TODO: 如果是查找模式，字体颜色要变红
				color = RED_CHAR_COLOR;
			}
			if (p_con->cursor < p_con->original_addr + p_con->v_mem_limit - 1) {
				*p_vmem++ = ch;
				*p_vmem++ = color;
				p_con->cursor++;
				if (!p_con->rolling) {
					op_stack[++operation_top] = '\b';
				}
			}
			break;
		}
	}

	while (p_con->cursor >= p_con->current_start_addr + SCREEN_SIZE) {
		scroll_screen(p_con, SCR_DN);
	}

	flush(p_con);
}

PUBLIC int rollback(CONSOLE *p_con) { // TODO: 先前已经将字符存入栈，所以这里直接调用 outchar 即可
	p_con->rolling = 1;
	int ret = 1; //
	if (operation_top >= 0) {
		if (op_stack[operation_top] == '\b')
			ret = -1; // 撤销后删除了字符
		out_char(p_con, op_stack[operation_top--]);
	}
	p_con->rolling = 0;
	return ret;
}

/*======================================================================*
                           flush
*======================================================================*/
PRIVATE void flush(CONSOLE *p_con) {
	set_cursor(p_con->cursor);
	set_video_start_addr(p_con->current_start_addr);
}

/*======================================================================*
			    set_cursor
 *======================================================================*/
PRIVATE void set_cursor(unsigned int position) {
	disable_int();
	out_byte(CRTC_ADDR_REG, CURSOR_H);
	out_byte(CRTC_DATA_REG, (position >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, CURSOR_L);
	out_byte(CRTC_DATA_REG, position & 0xFF);
	enable_int();
}

/*======================================================================*
			  set_video_start_addr
 *======================================================================*/
PRIVATE void set_video_start_addr(u32 addr) {
	disable_int();
	out_byte(CRTC_ADDR_REG, START_ADDR_H);
	out_byte(CRTC_DATA_REG, (addr >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, START_ADDR_L);
	out_byte(CRTC_DATA_REG, addr & 0xFF);
	enable_int();
}


/*======================================================================*
			   select_console
 *======================================================================*/
PUBLIC void select_console(int nr_console)    /* 0 ~ (NR_CONSOLES - 1) */
{
	if ((nr_console < 0) || (nr_console >= NR_CONSOLES)) {
		return;
	}

	nr_current_console = nr_console;

	set_cursor(console_table[nr_console].cursor);
	set_video_start_addr(console_table[nr_console].current_start_addr);
}

/*======================================================================*
			   scroll_screen
 *----------------------------------------------------------------------*
 滚屏.
 *----------------------------------------------------------------------*
 direction:
	SCR_UP	: 向上滚屏
	SCR_DN	: 向下滚屏
	其它	: 不做处理
 *======================================================================*/
PUBLIC void scroll_screen(CONSOLE *p_con, int direction) {
	if (direction == SCR_UP) {
		if (p_con->current_start_addr > p_con->original_addr) {
			p_con->current_start_addr -= SCREEN_WIDTH;
		}
	} else if (direction == SCR_DN) {
		if (p_con->current_start_addr + SCREEN_SIZE <
		    p_con->original_addr + p_con->v_mem_limit) {
			p_con->current_start_addr += SCREEN_WIDTH;
		}
	} else {
	}

	set_video_start_addr(p_con->current_start_addr);
	set_cursor(p_con->cursor);
}
