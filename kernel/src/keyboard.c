#include "keyboard.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stream.h>

#include "cpu.h"
#include "isr.h"
#include "memory.h"

#define MOD_LCTRL 0
#define MOD_RCTRL 1
#define MOD_LSHIFT 2
#define MOD_RSHIFT 3
#define MOD_LALT 4
#define MOD_RALT 5
#define MOD_LSUPER 6
#define MOD_RSUPER 7
#define MOD_CAPS 8
#define MOD_COUNT 9

static const char keymap[] = {
	0  , 0  , '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=','\b','\t',
	'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', 0  , '[','\n', 0  , 'a', 's',
	'd', 'f', 'g', 'h', 'j', 'k', 'l', 0  , '~','\'', 0  , ']', 'z', 'x', 'c', 'v',
	'b', 'n', 'm', ',', '.', ';', 0  , '*', 0  , ' ', 0  , 0  , 0  , 0  , 0  , 0  ,
	0  , 0  , 0  , 0  , 0  , 0  , 0  , '7', '8', '9', '-', '4', '5', '6', '+', '1',
	'2', '3', '0', ',', 0  , 0  ,'\\', 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  ,
	0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  ,
	0  , 0  , 0  , '/', 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , '.', 0  ,
};

static const char keymap_hi[] = {
	0  , 0  , '!', '@', '#', '$', '%', 0  , '&', '*', '(', ')', '_', '+','\b','\t',
	'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '`', '{','\n', 0  , 'A', 'S',
	'D', 'F', 'G', 'H', 'J', 'K', 'L', 0  , '^', '"', 0  , '}', 'Z', 'X', 'C', 'V',
	'B', 'N', 'M', '<', '>', ':', 0  , '*', 0  , ' ', 0  , 0  , 0  , 0  , 0  , 0  ,
	0  , 0  , 0  , 0  , 0  , 0  , 0  , '7', '8', '9', '-', '4', '5', '6', '+', '1',
	'2', '3', '0', ',', 0  , 0  , '|', 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  ,
	0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  ,
	0  , 0  , 0  , '?', 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , '.', 0  ,
};

static bool mod[MOD_COUNT] = { false };

static void send_key(int key) {
	if (key < KEY_ESC) {
		char c = (char) key;
		stream_write(stdin, &c, 1);
	} else {
		char c = KEY_SEQ;
		stream_write(stdin, &c, 1);
		c = (char) (key - KEY_ESC);
		stream_write(stdin, &c, 1);
	}
}

void keyboard_isr() {
	static bool e0 = false;
	unsigned key = in8(0x60);
	if (key == 0xE0) { e0 = true; return; }
	else if (e0) { key |= 0xE000; e0 = false; }
	switch (key) {
		case 0x01: send_key(KEY_ESC); break;
		case 0x1D: mod[MOD_LCTRL] = true; break;
		case 0x2A: mod[MOD_LSHIFT] = true; break;
		case 0x36: mod[MOD_RSHIFT] = true; break;
		case 0x38: mod[MOD_LALT] = true; break;
		case 0x9D: mod[MOD_LCTRL] = false; break;
		case 0xAA: mod[MOD_LSHIFT] = false; break;
		case 0xB6: mod[MOD_RSHIFT] = false; break;
		case 0xB8: mod[MOD_LALT] = false; break;
		case 0xBA: mod[MOD_CAPS] = !mod[MOD_CAPS]; break;
		case 0xE01C: send_key('\n'); break;
		case 0xE01D: mod[MOD_RCTRL] = true; break;
		case 0xE035: send_key('/'); break;
		case 0xE038: mod[MOD_RALT] = true; break;
		case 0xE047: send_key(KEY_HOME); break;
		case 0xE048: send_key(KEY_UP); break;
		case 0xE049: send_key(KEY_PAGE_UP); break;
		case 0xE04B: send_key(KEY_LEFT); break;
		case 0xE04D: send_key(KEY_RIGHT); break;
		case 0xE04F: send_key(KEY_END); break;
		case 0xE050: send_key(KEY_DOWN); break;
		case 0xE051: send_key(KEY_PAGE_DOWN); break;
		case 0xE052: send_key(KEY_INSERT); break;
		case 0xE053: send_key(KEY_DELETE); break;
		case 0xE05B: mod[MOD_LSUPER] = true; break;
		case 0xE05C: mod[MOD_RSUPER] = true; break;
		case 0xE09D: mod[MOD_RCTRL] = false; break;
		case 0xE0B8: mod[MOD_RALT] = false; break;
		case 0xE0DB: mod[MOD_LSUPER] = false; break;
		case 0xE0DC: mod[MOD_RSUPER] = false; break;
		default:
			if (key >= sizeof(keymap))
				break;
			char c = mod[MOD_LSHIFT] || mod[MOD_RSHIFT] ? keymap_hi[key] : keymap[key];
			if (c == 0)
				break;
			if (mod[MOD_CAPS] && isalpha(c))
				c ^= 1 << 5;
			if (mod[MOD_LCTRL] || mod[MOD_RCTRL])
				c &= 0x9F;
			stream_write(stdin, &c, 1);
			break;
	}
}

void keyboard_init() {
	isr_set(0x21, keyboard_isr);
}
