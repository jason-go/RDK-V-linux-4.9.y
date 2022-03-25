/*
 * log.c
 *
 *  Created on: Jun 25, 2010
 * 
 * Synopsys Inc.
 * SG DWC PT02
 */

#include "log.h"
#include "kernelcalls.h"

static verbose_t log_Verbose = VERBOSE_NOTICE;
static numeric_t log_Numeric = NUMERIC_HEX;
static unsigned log_VerboseWrite = 0;

static const char *desc[] = {
	"HDMI-ERR", //VERBOSE_ERRR = 0,
	"HDMI-DBG", //VERBOSE_DEBUG,
	"HDMI-WRN", //VERBOSE_WARN,
	"HDMI-NTC", //VERBOSE_NOTICE,
	"HDMI-TRC", //VERBOSE_TRACE
};

void log_SetVerbose(verbose_t verbose)
{
	log_Verbose = verbose;
}

void log_SetNumeric(numeric_t numeric)
{
	log_Numeric = numeric;
}

void log_SetVerboseWrite(unsigned state)
{
	log_VerboseWrite = state;
}

void log_PrintWrite(unsigned a, unsigned b)
{
	if (log_VerboseWrite == 1) {
		if (log_Numeric == NUMERIC_DEC)
			gx_printf("%d, %d\n", a, b);
		else
			gx_printf("0x%x, 0x%x\n", a, b);
	}
}

void log_Print0(verbose_t verbose, const char* functionName)
{
	if (verbose <= log_Verbose) {
		const char *desc_string = NULL;
		desc_string = desc[verbose];
		gx_printf("\n[%s] %s\n", desc_string, functionName);
	}
}

void log_Print1(verbose_t verbose, const char* functionName, const char* a)
{
	if (verbose <= log_Verbose) {
		const char *desc_string = NULL;
		desc_string = desc[verbose];
		gx_printf("\n[%s] %s: %s\n", desc_string, functionName, a);
	}
}

void log_Print2(verbose_t verbose, const char* functionName, const char* a, unsigned b)
{
	if (verbose <= log_Verbose) {
		if (log_Numeric == NUMERIC_DEC)
			gx_printf("\n[%s] %s: %s, %d\n", desc[verbose], functionName, a, b);
		else
			gx_printf("\n[%s] %s: %s, 0x%x\n", desc[verbose], functionName, a, b);
	}
}

void log_Print3(verbose_t verbose, const char* functionName, const char* a, unsigned b, unsigned c)
{
	if (verbose <= log_Verbose) {
		if (log_Numeric == NUMERIC_DEC)
			gx_printf("\n[%s] %s: %s, %d, %d\n", desc[verbose], functionName, a, b, c);
		else
			gx_printf("\n[%s] %s: %s, 0x%x, 0x%x\n", desc[verbose], functionName, a, b, c);
	}
}

void log_PrintInt(verbose_t verbose, const char* functionName, unsigned a)
{
	if (verbose <= log_Verbose) {
		if (log_Numeric == NUMERIC_DEC)
			gx_printf("[HDMI] %s: %d\n", functionName, a);
		else
			gx_printf("[HDMI] %s: 0x%x\n", functionName, a);
	}
}

void log_PrintInt2(verbose_t verbose, const char* functionName, unsigned a, unsigned b)
{
	if (verbose <= log_Verbose) {
		if (log_Numeric == NUMERIC_DEC)
			gx_printf("[HDMI] %s: %d, %d\n", functionName, a, b);
		else
			gx_printf("[HDMI] %s: 0x%x, 0x%x\n", functionName, a, b);
	}
}

