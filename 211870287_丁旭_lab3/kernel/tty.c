
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               tty.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

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

#define TTY_FIRST (tty_table)
#define TTY_END (tty_table + NR_CONSOLES)

PRIVATE void init_tty(TTY *p_tty);

PRIVATE void tty_do_read(TTY *p_tty);

PRIVATE void tty_do_write(TTY *p_tty);

PRIVATE void put_key(TTY *p_tty, u32 key);

// TODO
PRIVATE char search_text[SCREEN_SIZE]; // 存放搜索关键字
PRIVATE int key_len; // 关键词长度，在搜索模式下有效


/*======================================================================*
                           task_fresher
 *======================================================================*/
PUBLIC void task_fresher() { // TODO: 清空屏幕
	while (1) {
		milli_delay(200000); // 20 seconds
		for (TTY *p_tty = TTY_FIRST; p_tty < TTY_END; p_tty++) {
			while (p_tty->p_console->searchMode); // 搜索模式下不清空屏幕
			clean_screen(p_tty->p_console);
		}
	}
}

/*======================================================================*
                           task_tty
 *======================================================================*/
PUBLIC void task_tty() {
	init_keyboard();

	for (TTY *p_tty = TTY_FIRST; p_tty < TTY_END; p_tty++) {
		init_tty(p_tty);
	}
	key_len = 0; // TODO: 关键字长度
	select_console(0);
	while (1) {
		for (TTY *p_tty = TTY_FIRST; p_tty < TTY_END; p_tty++) {
			while (p_tty->p_console->cleaning); // TODO
			tty_do_read(p_tty);
			tty_do_write(p_tty);
		}
	}
}

/*======================================================================*
			   init_tty
 *======================================================================*/
PRIVATE void init_tty(TTY *p_tty) {
	p_tty->inbuf_count = 0;
	p_tty->p_inbuf_head = p_tty->p_inbuf_tail = p_tty->in_buf;

	init_screen(p_tty);
}

/*======================================================================*
				in_process
 *======================================================================*/
PUBLIC void in_process(TTY *p_tty, u32 key) {
	char output[2] = {'\0', '\0'};
	int raw_code = key & MASK_RAW; // raw key value = code passed to tty & MASK_RAW

	CONSOLE *p_con = p_tty->p_console; // TODO
	if (raw_code == ESC) {
		if (p_con->searchMode) {
			end_search(p_con, key_len);
			p_con->searchMode = 0;
		} else {
			p_con->searchMode = 1;
		}

		key_len = 0;
		p_con->blocked = 0;
		return;
	}

	if (p_con->blocked == 0) { // TODO: 未屏蔽按键
		if (!(key & FLAG_EXT)) { // Normal function keys
			if ((key & FLAG_CTRL_L) || (key & FLAG_CTRL_R)) { // Ctrl
				output[0] = key;
				if (output[0] == 'z' || output[0] == 'Z') { // Ctrl + Z
					int backspaced = rollback(p_con); // 单步撤销
					if (p_con->searchMode) {
						key_len += backspaced;
					}
				}
			} else {
				put_key(p_tty, key);
			}
		} else {
			switch (raw_code) {
				case ENTER: {
					if (p_con->searchMode) { // 查找模式下
						p_con->blocked = 1;
						search(p_con, search_text, key_len);
					} else {
						put_key(p_tty, '\n');
					}
					break;
				}
				case BACKSPACE: {
					if (p_con->searchMode) { // 退格关键字
						if (key_len > 0) {
							put_key(p_tty, '\b');
							key_len--;
						}
					} else { // 退格文本
						put_key(p_tty, '\b');
					}
					break;
				}
				case TAB: {
					put_key(p_tty, '\t');
					break;
				}
				case UP: {
					if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)) {
						scroll_screen(p_con, SCR_DN);
					}
					break;
				}
				case DOWN: {
					if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)) {
						scroll_screen(p_con, SCR_UP);
					}
					break;
				}
					// 下面的代码是用来切换终端的，跟本次作业没有关系
				case F1:
				case F2:
				case F3:
				case F4:
				case F5:
				case F6:
				case F7:
				case F8:
				case F9:
				case F10:
				case F11:
				case F12:
					/* Alt + F1~F12 */
					if ((key & FLAG_ALT_L) || (key & FLAG_ALT_R)) {
						select_console(raw_code - F1);
					}
					break;
				default:
					break;
			}
		}
	}
}

/*======================================================================*
			      put_key
*======================================================================*/
PRIVATE void put_key(TTY *p_tty, u32 key) {
	if (p_tty->inbuf_count < TTY_IN_BYTES) {
		*(p_tty->p_inbuf_head) = key;
		p_tty->p_inbuf_head++;
		if (p_tty->p_inbuf_head == p_tty->in_buf + TTY_IN_BYTES) {
			p_tty->p_inbuf_head = p_tty->in_buf;
		}
		p_tty->inbuf_count++;
	}
}


/*======================================================================*
			      tty_do_read
 *======================================================================*/
PRIVATE void tty_do_read(TTY *p_tty) {
	if (is_current_console(p_tty->p_console)) {
		keyboard_read(p_tty);
	}
}


/*======================================================================*
			      tty_do_write
 *======================================================================*/
PRIVATE void tty_do_write(TTY *p_tty) {
	if (p_tty->inbuf_count) { // 有待处理的任务
		char ch = *(p_tty->p_inbuf_tail++); // 键盘任务应处理的键值
		if (p_tty->p_inbuf_tail == p_tty->in_buf + TTY_IN_BYTES) { // 到达缓冲区结尾
			p_tty->p_inbuf_tail = p_tty->in_buf;
		}
		p_tty->inbuf_count--;
		if (p_tty->p_console->searchMode && ch != '\b') { // TODO: 处于搜索模式且不是输入退格符
			search_text[key_len++] = ch;
		}
		out_char(p_tty->p_console, ch);
	}
}