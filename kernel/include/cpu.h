#pragma once

#include <stdint.h>

static inline void out8(u16 port, u8 data) {
	asm volatile ("outb %0, %1" : : "a" (data), "Nd" (port));
}

static inline void out16(u16 port, u16 data) {
	asm volatile ("outw %0, %1" : : "a" (data), "Nd" (port));
}

static inline void out32(u16 port, u32 data) {
	asm volatile ("outl %0, %1" : : "a" (data), "Nd" (port));
}

static inline void outs32(u16 port, u64 buffer, u32 count) {
	asm volatile ("cld; rep; outsl" : : "D" (buffer), "d" (port), "c" (count));
}

static inline u8 in8(u16 port) {
	u8 ret;
	asm volatile ("inb %1, %0" : "=a" (ret) : "Nd" (port));
	return ret;
}

static inline u16 in16(u16 port) {
	u16 ret;
	asm volatile ("inw %1, %0" : "=a" (ret) : "Nd" (port));
	return ret;
}

static inline u32 in32(u16 port) {
	u32 ret;
	asm volatile ("inl %1, %0" : "=a" (ret) : "Nd" (port));
	return ret;
}

static inline void ins32(u16 port, u64 buffer, u32 count) {
	asm volatile ("cld; rep; insl" : : "D" (buffer), "d" (port), "c" (count));
}
