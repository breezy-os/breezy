#ifndef BZ_LOGGER_H
#define BZ_LOGGER_H
// #################################################################################################


#include <stdint.h>
#include <stdio.h>

enum bz_log_level { BZ_LOG_DEBUG, BZ_LOG_INFO, BZ_LOG_WARN, BZ_LOG_ERROR, BZ_LOG_OFF };

// -- Configuring the Logger --
void bz_log_initialize(enum bz_log_level default_level);
void bz_log_initialize_custom(enum bz_log_level default_level, FILE *out, FILE *err);
void bz_log_set_level(uint8_t category, enum bz_log_level level);

// -- Using the Logger --
void bz_debug(uint8_t category, char *file, int line, const char *message_fmt, ...);
void bz_info(uint8_t category, char *file, int line, const char *message_fmt, ...);
void bz_warn(uint8_t category, char *file, int line, const char *message_fmt, ...);
void bz_error(uint8_t category, char *file, int line, const char *message_fmt, ...);

// -- Categories for Breezy --
#define BZ_LOG_MAIN 0
#define BZ_LOG_GRAPHICS 1
#define BZ_LOG_INPUT 2
#define BZ_LOG_SEAT 3
#define BZ_LOG_LIST 4


// #################################################################################################
#endif