#pragma once

#include <stdfile.h>
#include <stdint.h>

enum stream_type {
	STREAM_MEMORY, STREAM_FILE,
};

struct stream {
	enum stream_type type;
	u64 offset;
	u64 length;
	union {
		u8* buffer;
		std_file_t* file;
	};
};

struct stream* stream_open(std_file_t*);
void stream_close(struct stream*);
u64 stream_write(struct stream*, const void*, u64);
u64 stream_read(struct stream*, void*, u64);
