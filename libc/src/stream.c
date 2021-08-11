#include "stream.h"

#include <stddef.h>
#include <stdlib.h>
#include <system.h>

struct stream* stream_open(std_file_t* file) {
	struct stream* stream = malloc(sizeof(struct stream));
	stream->offset = 0;
	if (file != NULL) {
		stream->type = STREAM_FILE;
		stream->length = file->size;
		stream->file = file;
	} else {
		stream->type = STREAM_MEMORY;
		stream->length = 0;
		stream->buffer = mmap(NULL, 1);
	}
	return stream;
}

void stream_close(struct stream* stream) {
	if (stream == NULL)
		return;
	switch (stream->type) {
		case STREAM_MEMORY:
			munmap(stream->buffer, 1);
			break;
		default:
			break;
	}
	free(stream);
}

static u64 stream_memory_write(struct stream* stream, const u8* buffer, u64 length) {
	if (stream->length == 0x1000)
		return 0;
	u64 remainder = 0x1000 - stream->length;
	u64 count = remainder < length ? remainder : length;
	for (u64 i = 0; i < count; i++)
		stream->buffer[(stream->offset + stream->length + i) & 0xFFF] = buffer[i];
	stream->length += count;
	return count;
}

static u64 stream_file_write(struct stream* stream, const u8* buffer, u64 length) {
	u64 written = std_file_write(stream->file, stream->length, buffer, length);
	stream->length += written;
	return written;
}

u64 stream_write(struct stream* stream, const void* buffer, u64 length) {
	if (stream == NULL)
		return 0;
	switch (stream->type) {
		case STREAM_MEMORY:
			return stream_memory_write(stream, (const u8*) buffer, length);
		case STREAM_FILE:
			return stream_file_write(stream, (const u8*) buffer, length);
		default:
			return 0;
	}
}

static u64 stream_memory_read(struct stream* stream, u8* buffer, u64 length) {
	if (stream->length == 0)
		return 0;
	u64 count = length < stream->length ? length : stream->length;
	for (u64 i = 0; i < count; i++)
		buffer[i] = stream->buffer[(stream->offset + i) & 0xFFF];
	stream->offset += count;
	stream->length -= count;
	return count;
}

static u64 stream_file_read(struct stream* stream, u8* buffer, u64 length) {
	u64 remaining = stream->length - stream->offset;
	u64 count = length < remaining ? length : remaining;
	u64 read = std_file_read(stream->file, stream->offset, buffer, count);
	stream->offset += read;
	return read;
}

u64 stream_read(struct stream* stream, void* buffer, u64 length) {
	if (stream == NULL)
		return 0;
	switch (stream->type) {
		case STREAM_MEMORY:
			return stream_memory_read(stream, (u8*) buffer, length);
		case STREAM_FILE:
			return stream_file_read(stream, (u8*) buffer, length);
		default:
			return 0;
	}
}
