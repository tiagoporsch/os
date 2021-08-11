#pragma once

#include "isr.h"

#define assert(check) do { if (!(check)) panic("%s:%d: Assertion `%s` failed.\n", __func__, __LINE__, #check); } while (0)

_Noreturn void panic(const char*, ...);
_Noreturn void panic_isr(struct isr_stack);
_Noreturn void panic_isr_user(struct isr_stack s);
