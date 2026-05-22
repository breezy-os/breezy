
// This gives us access to open_memstream() for an in-memory file-like buffer
#define _POSIX_C_SOURCE 200809L // NOLINT

#include <stdio.h>
#include <stdlib.h>

#include "unity/unity.h"
#include "fff/fff.h"
DEFINE_FFF_GLOBALS

#include "breezy/bz_logger.h"


// =================================================================================================
//  Set up / tear down / globals
// -------------------------------------------------------------------------------------------------

static FILE *mem = nullptr;
static char *buf = nullptr;

void setUp(void)
{
	size_t size;
	mem = open_memstream(&buf, &size);
}

void tearDown(void)
{
	(void) fclose(mem);
	free(buf);
	mem = nullptr;
	buf = nullptr;
}


// =================================================================================================
//  Test Functions
// -------------------------------------------------------------------------------------------------

void test_failed_to_initialize(void)
{
	// Not initialized, so nothing should be written to the buffer.
	bz_debug(BZ_LOG_MAIN, "file", 0, "This should fail.");
	(void) fflush(mem);
	TEST_ASSERT_EQUAL_STRING("", buf);
}

void test_default_log_level(void)
{
	// Setting "WARN" should only print warnings and errors.
	bz_log_initialize_custom(BZ_LOG_WARN, mem, mem);
	bz_debug(BZ_LOG_MAIN, "file", 0, "Debug statement");
	bz_info(BZ_LOG_MAIN, "file", 0, "Info statement");
	bz_warn(BZ_LOG_MAIN, "file", 0, "Warn statement");
	bz_error(BZ_LOG_MAIN, "file", 0, "Error statement");
	(void) fflush(mem);
	TEST_ASSERT_EQUAL_STRING(
		"WARN  [file:0]\tWarn statement\n"
		"[file:0]\tError statement\n" // From stderr
		"ERROR [file:0]\tError statement\n",
		buf);
}

void test_category_log_level(void)
{
	// Defaulting "WARN", but assigning "DEBUG" to DRM
	bz_log_initialize_custom(BZ_LOG_WARN, mem, mem);
	bz_log_set_level(BZ_LOG_GRAPHICS, BZ_LOG_DEBUG);

	// "WARN/ERROR" should always be printed. "DEBUG/INFO" only for DRM logs.
	bz_debug(BZ_LOG_MAIN, "file", 0, "Main Debug");
	bz_info(BZ_LOG_MAIN, "file", 0, "Main Info");
	bz_warn(BZ_LOG_MAIN, "file", 0, "Main Warn");
	bz_error(BZ_LOG_MAIN, "file", 0, "Main Error");
	bz_debug(BZ_LOG_GRAPHICS, "file", 0, "DRM Debug");
	bz_info(BZ_LOG_GRAPHICS, "file", 0, "DRM Info");
	bz_warn(BZ_LOG_GRAPHICS, "file", 0, "DRM Warn");
	bz_error(BZ_LOG_GRAPHICS, "file", 0, "DRM Error");
	(void) fflush(mem);

	TEST_ASSERT_EQUAL_STRING(
		"WARN  [file:0]\tMain Warn\n"
		"[file:0]\tMain Error\n" // From stderr
		"ERROR [file:0]\tMain Error\n"
		"DEBUG [file:0]\tDRM Debug\n"
		"INFO  [file:0]\tDRM Info\n"
		"WARN  [file:0]\tDRM Warn\n"
		"[file:0]\tDRM Error\n" // From stderr
		"ERROR [file:0]\tDRM Error\n",
		buf);
}

void test_off_log_level(void)
{
	// Defaulting "WARN", but assigning "OFF" to DRM
	bz_log_initialize_custom(BZ_LOG_WARN, mem, mem);
	bz_log_set_level(BZ_LOG_GRAPHICS, BZ_LOG_OFF);

	// "WARN/ERROR" should be printed for main, and nothing for DRM.
	bz_debug(BZ_LOG_MAIN, "file", 0, "Main Debug");
	bz_info(BZ_LOG_MAIN, "file", 0, "Main Info");
	bz_warn(BZ_LOG_MAIN, "file", 0, "Main Warn");
	bz_error(BZ_LOG_MAIN, "file", 0, "Main Error");
	bz_debug(BZ_LOG_GRAPHICS, "file", 0, "DRM Debug");
	bz_info(BZ_LOG_GRAPHICS, "file", 0, "DRM Info");
	bz_warn(BZ_LOG_GRAPHICS, "file", 0, "DRM Warn");
	bz_error(BZ_LOG_GRAPHICS, "file", 0, "DRM Error");
	(void) fflush(mem);

	TEST_ASSERT_EQUAL_STRING(
		"WARN  [file:0]\tMain Warn\n"
		"[file:0]\tMain Error\n" // From stderr
		"ERROR [file:0]\tMain Error\n",
		buf);
}

void test_custom_log_message_format(void)
{
	bz_log_initialize_custom(BZ_LOG_DEBUG, mem, mem);
	bz_debug(BZ_LOG_MAIN, "file", 0, "Number: %d, String: %s", 42, "hello");
	(void) fflush(mem);
	TEST_ASSERT_EQUAL_STRING("DEBUG [file:0]\tNumber: 42, String: hello\n", buf);
}


// =================================================================================================
//  Runner
// -------------------------------------------------------------------------------------------------

int main(void) {
	UNITY_BEGIN();
	RUN_TEST(test_failed_to_initialize);
	RUN_TEST(test_default_log_level);
	RUN_TEST(test_category_log_level);
	RUN_TEST(test_off_log_level);
	RUN_TEST(test_custom_log_message_format);
	return UNITY_END();
}