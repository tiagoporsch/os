#pragma once

#include "stdlib.h"

#define assert(check)\
	do {\
		if (!(check)) {\
			printf("%s:%d: Assertion `%s` failed.\n", __func__, __LINE__, #check);\
			exit(1);\
		}\
	} while (0)
