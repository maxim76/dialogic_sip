#include <cerrno>	// errno
#include <cstdarg>	// va_start/va_end
#include <cstdlib>	// exit
#include <cstring>	// strerror
#include <stdio.h>
#include <string>
#include <time.h>

// _mkdir header
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#include "logging.h"

FILE* fLog;
FILE* fErrorLog;
FILE* fStat;
int g_event_code_offset = 0;
time_t g_start_time;

#define LOGSTRSIZE	256

void InitLogFile(int event_code_offset)
{
	g_event_code_offset = event_code_offset;
	const char* logDir = "./Logs/";
	// Create directory for log files if it does not exist
	int res;
#ifdef _WIN32
	res = _mkdir(logDir);
#else
	res = mkdir(logDir, 0733);
#endif
	if ((res != 0) && (errno != EEXIST))
	{
		printf("Cannot create log dir. Error: %d (%s)\n", errno, strerror(errno));
		exit(1);
	}

	char sFN[64];
	struct tm* tblock;
	tblock = localtime(&g_start_time);

	// Full log file
	sprintf(sFN, "%s/ivrserv_%04d%02d%02d_%02d%02d%02d.txt", logDir, tblock->tm_year + 1900, tblock->tm_mon + 1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec);
	if ((fLog = fopen(sFN, "w")) == NULL)
	{
		printf("Cannot create log file. Termination.\n");
		exit(1);
	}

	// Errors only
	sprintf(sFN, "%s/errors_%04d%02d%02d_%02d%02d%02d.txt", logDir, tblock->tm_year + 1900, tblock->tm_mon + 1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec);
	if ((fErrorLog = fopen(sFN, "w")) == NULL)
	{
		printf("Cannot create error log file. Termination.\n");
		exit(1);
	}

	// Statistics. TODO: move to stat module/function
	sprintf(sFN, "%s/stat_%04d%02d%02d_%02d%02d%02d.txt", logDir, tblock->tm_year + 1900, tblock->tm_mon + 1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec);
	if ((fStat = fopen(sFN, "w")) == NULL)
	{
		printf("Cannot create statistics file. Termination.\n");
		exit(1);
	}
}

void Log(int Src, int Svrt, int Line, const char* format, ...)
{
	char str[LOGSTRSIZE];	// Full log string
	char msg[LOGSTRSIZE];	// Message part

	// Form log source part
	std::string logSource = "";
	switch (Src)
	{
	case TRC_CORE: logSource = "CORE"; break;
	case TRC_GC:   logSource = "GC  "; break;
	case TRC_DX:   logSource = "DX  "; break;
	case TRC_SETT: logSource = "SETT"; break;
	}

	// Form verbal severity description
	std::string severityDescr = "";
	switch (Svrt)
	{
	case TRC_DUMP:     severityDescr = "DUMP    "; break;
	case TRC_INFO:     severityDescr = "INFO    "; break;
	case TRC_WARNING:  severityDescr = "WARNING "; break;
	case TRC_ERROR:    severityDescr = "ERROR   "; break;
	case TRC_CRITICAL: severityDescr = "CRITICAL"; break;
	}

	// Form message
	va_list args;
	va_start(args, format);
	vsprintf(msg, format, args);
	va_end(args);

	if (Line >= 0)
		snprintf(str, LOGSTRSIZE, "[%s] %s [%04d] %s", logSource.c_str(), severityDescr.c_str(), Line, msg);
	else
		snprintf(str, LOGSTRSIZE, "[%s] %s [    ] %s", logSource.c_str(), severityDescr.c_str(), msg);

	LogWrite(str, Svrt);
}
//---------------------------------------------------------------------------
void LogGC(int Svrt, int Line, int event, const char* msg)
{
	char str[LOGSTRSIZE];
	char sEvt[GCEVENTNAMESIZE];
	strncpy(sEvt, GcEventNames[event - g_event_code_offset], GCEVENTNAMESIZE);
	if (sEvt[0] == 0) snprintf(sEvt, GCEVENTNAMESIZE, "0x%04x", event);
	snprintf(str, LOGSTRSIZE, "%s %s", sEvt, msg);
	Log(TRC_GC, Svrt, Line, str);
}
//---------------------------------------------------------------------------
void LogDX(int Svrt, int Line, int event, const char* msg)
{
	char str[LOGSTRSIZE];
	char sEvt[GCEVENTNAMESIZE];
	strncpy(sEvt, DxEventNames[event - 0x81], GCEVENTNAMESIZE);
	if (sEvt[0] == 0) snprintf(sEvt, GCEVENTNAMESIZE, "0x%04x", event);
	snprintf(str, LOGSTRSIZE, "%s %s", sEvt, msg);
	Log(TRC_DX, Svrt, Line, str);
}
//---------------------------------------------------------------------------
void LogFunc(int Line, const char* FuncName, bool is_success)
{
	char str[LOGSTRSIZE];
	sprintf(str, "%s : %s", FuncName, (is_success) ? "Ok" : "Error");
	Log(TRC_CORE, (is_success) ? 0 : 2, Line, str);
}
//---------------------------------------------------------------------------
void LogWrite(const char* msg, int Svrt)
{
	time_t timer = time(NULL);
	struct tm* tblock;
	tblock = localtime(&timer);
	printf("%04d/%02d/%02d %02d:%02d:%02d %s\n", tblock->tm_year + 1900, tblock->tm_mon + 1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec, msg);
	fprintf(fLog, "%04d/%02d/%02d %02d:%02d:%02d %s\n", tblock->tm_year + 1900, tblock->tm_mon + 1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec, msg);
	fflush(fLog);

	if (Svrt >= DEFAULT_ERRLOG_FILTER)
	{
		fprintf(fErrorLog, "%04d/%02d/%02d %02d:%02d:%02d %s\n", tblock->tm_year + 1900, tblock->tm_mon + 1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec, msg);
		fflush(fErrorLog);
	}
}
//---------------------------------------------------------------------------
void CloseLogFiles() {
	fclose(fLog);
	fclose(fErrorLog);
	fclose(fStat);
}
//---------------------------------------------------------------------------
void LogStat(uint16_t total, uint16_t unblocked, uint16_t used) {
	struct tm* tblock;
	tblock = localtime(&g_start_time);

	fprintf(fStat, "%04d/%02d/%02d %02d:%02d:%02d  %4d %4d %4d\n",
		tblock->tm_year + 1900, tblock->tm_mon + 1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,
		total, unblocked, used);
	fflush(fStat);
}
//---------------------------------------------------------------------------
