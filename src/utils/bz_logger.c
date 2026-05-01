
#include "./breezy/bz_logger.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>


// Determined by categories being of type "uint8_t"
#define BZ_LOG_NUM_CATEGORIES 256

// =================================================================================================
//  State
// -------------------------------------------------------------------------------------------------

static bool is_initialized = false;
static bool log_levels[BZ_LOG_NUM_CATEGORIES];


// =================================================================================================
//  Configuration
// -------------------------------------------------------------------------------------------------

void bz_log_initialize(const enum bz_log_level default_level)
{
	for (int x = 0; x < BZ_LOG_NUM_CATEGORIES; x++) {
		log_levels[x] = default_level;
	}
	is_initialized = true;
}

void bz_log_set_level(const uint8_t category, const enum bz_log_level level)
{
	log_levels[category] = level;
}


// =================================================================================================
//  Logging
// -------------------------------------------------------------------------------------------------

__attribute__((__format__(__printf__, 5, 0) ))
static void bz_log(const enum bz_log_level level, const uint8_t category, char *file, const int line, const char *message_fmt, va_list args)
{

	if (!is_initialized) {
		fprintf(stderr, "Cannot use bz_logger prior to calling bz_log_initialize()\n");
	}

	if (log_levels[category] > level) {
		return;
	}

	char message[512];
	const int retval = vsnprintf(message, sizeof(message), message_fmt, args);

	if (retval < 0) {
		fprintf(stderr, "[%s:%d] Failed to write formatted log message. Error code: %d\n",
			file, line, retval);
		return;
	}

	if (level >= BZ_LOG_ERROR) {
		fprintf(stderr, "[%s:%d] %s\n", file, line, message);
	} else {
		fprintf(stdout, "[%s:%d] %s\n", file, line, message);
	}
}

__attribute__((__format__(__printf__, 4, 5) ))
void bz_debug(const uint8_t category, char *file, const int line, const char *message_fmt, ...)
{
	va_list args;
	va_start(args, message_fmt);
	bz_log(BZ_LOG_DEBUG, category, file, line, message_fmt, args);
	va_end(args);
}

__attribute__((__format__(__printf__, 4, 5) ))
void bz_info(const uint8_t category, char *file, const int line, const char *message_fmt, ...)
{
	va_list args;
	va_start(args, message_fmt);
	bz_log(BZ_LOG_INFO, category, file, line, message_fmt, args);
	va_end(args);
}

__attribute__((__format__(__printf__, 4, 5) ))
void bz_warn(const uint8_t category, char *file, const int line, const char *message_fmt, ...)
{
	va_list args;
	va_start(args, message_fmt);
	bz_log(BZ_LOG_WARN, category, file, line, message_fmt, args);
	va_end(args);
}

__attribute__((__format__(__printf__, 4, 5) ))
void bz_error(const uint8_t category, char *file, const int line, const char *message_fmt, ...)
{
	va_list args;
	va_start(args, message_fmt);
	bz_log(BZ_LOG_ERROR, category, file, line, message_fmt, args);
	va_end(args);
}