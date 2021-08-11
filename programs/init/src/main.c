#include <stddef.h>
#include <system.h>

u64 main() {
	wait(exec("/bin/sh", NULL));
	return 0;
}
