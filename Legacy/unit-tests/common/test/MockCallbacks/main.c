#include "main.h"
#include <stdio.h>
#include "unity.h"
#define ERROR_STRING_MAX 50
static const char * error_format_str = "ERROR: File: %s, Line: %d";
void fake_handleError(char *file, int line) {
	char output_str[ERROR_STRING_MAX] = {0};
	snprintf(output_str, ERROR_STRING_MAX, error_format_str, file, line);
	TEST_FAIL_MESSAGE(output_str);
}

void Error_Handler(void)
{
	fake_handleError(__FILE__, __LINE__);
}

