#pragma once
#include <cstdint>

#define TRC_CORE	1
#define TRC_GC		2
#define TRC_DX		4
#define TRC_SETT	8

#define TRC_DUMP		0
#define TRC_INFO		1
#define TRC_WARNING		2
#define TRC_ERROR		3
#define TRC_CRITICAL	4

#define DEFAULT_ERRLOG_FILTER	TRC_ERROR

#define MAXGCEVENT	0x100
#define GCEVENTNAMESIZE	32
static const char GcEventNames[MAXGCEVENT][GCEVENTNAMESIZE] = {
	/*0x00*/    "","GCEV_TASKFAIL","GCEV_ANSWERED","GCEV_CALLPROGRESS","GCEV_ACCEPT","GCEV_DROPCALL","GCEV_RESETLINEDEV","GCEV_CALLINFO",
	/*0x08*/    "GCEV_REQANI","GCEV_SETCHANSTATE","GCEV_FACILITY_ACK","GCEV_FACILITY_REJ","GCEV_MOREDIGITS","","GCEV_SETBILLING","",
	/*0x10*/    "","","","","","","","GCEV_OPENEX","GCEV_OPENFAIL","","","","","","","",
	/*0x20*/    "","GCEV_ALERTING","GCEV_CONNECTED","GCEV_ERROR","GCEV_OFFERED","","GCEV_DISCONNECTED","GCEV_PROCEEDING",
	/*0x28*/    "GCEV_PROGRESSING","GCEV_USRINFO","GCEV_FACILITY","GCEV_CONGESTION","","","GCEV_D_CHAN_STATUS","",
	/*0x30*/    "","","GCEV_BLOCKED","GCEV_UNBLOCKED","","","","",
	/*0x38*/    "","","","","","","","",
	/*0x40*/    "","","GCEV_HOLDCALL","","","","","","","","","","","","","",
	/*0x50*/    "","","","","","","","","","GCEV_RELEASECALL","","","","","","",
	/*0x60*/    "","","","","","","GCEV_CALLPROC","","","","","","","","","",
	/*0x70*/    "","","","","","","","","","","","","","","","",
	/*0x80*/    "","","","","","","","","","","","","","","","",
	/*0x90*/    "","","","","","","","","","","","","","","","",
	/*0xA0*/    "","","","","","","","","","","","","","","","",
	/*0xB0*/    "","","","","","","","","","","","","","","","",
	/*0xC0*/    "","","","","","","","","","","","","","","","",
	/*0xD0*/    "","","","","","","","","","","","","","","","",
	/*0xE0*/    "","","","","","","","","","","","","","","","",
	/*0xF0*/    "","","","","","","","","","","","","","","",""
};

#define MAXDXEVENT	10
#define DXEVENTNAMESIZE	32
static const char DxEventNames[MAXDXEVENT][DXEVENTNAMESIZE] = { "TDX_PLAY","TDX_RECORD","TDX_GETDIGIT","TDX_DIAL","TDX_CALLP","TDX_CST","TDX_SETHOOK","TDX_WINK","X","TDX_PLAYTONE" };


void InitLogFile(int event_code_offset);

void LogWrite(const char*, int Svrt);
void Log(int Src, int Svrt, int Line, const char* format, ...);
void LogGC(int Svrt, int Line, int event, const char*);
void LogDX(int Svrt, int Line, int event, const char*);
void LogFunc(int Line, const char* FuncName, bool is_success);
void LogStat(uint16_t total, uint16_t unblocked, uint16_t used);

void CloseLogFiles();