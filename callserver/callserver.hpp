/* Dialogic Header Files */
#include <gcip.h>
#include <gclib.h>
#include <gcisdn.h>
#include <srllib.h>
#include <dxxxlib.h>
//---------------------------------------------------------------------------
#define YES					1
#define NO					0
//---------------------------------------------------------------------------
#define TRC_CORE	1
#define TRC_GC		2
#define TRC_DX		4
#define TRC_SETT	8

#define TRC_DUMP		0
#define TRC_INFO		1
#define TRC_WARNING		2
#define TRC_ERROR		3
#define TRC_CRITICAL	4
//---------------------------------------------------------------------------
#define LOGSTRSIZE	256
#define MAX_DEVNAME	64		// Maximum length of device name											*/
#define MAX_NUMSIZE	128
//---------------------------------------------------------------------------
#define VST_IDLE        0
#define VST_PLAY        1
//---------------------------------------------------------------------------
#define PST_NULL		0	// Immediately after channel is opened
#define PST_IDLE		1	// ready for call (waitcall is done)
#define PST_CALLING		2	// makecall is in progress
#define PST_PLAY		3	// dx_play is in progress
#define PST_RELEASING	4	// DropCall is in progress
#define PST_SHUTDOWN	5	// gc_Close() is done
//---------------------------------------------------------------------------
#define MAXCHANS	2000  // Potential maximum of configured channels
#define MAX_CALLS       2
//---------------------------------------------------------------------------
#define CONFIG_FILE		"callserver.cfg"
//---------------------------------------------------------------------------
#define MAXPARAMSIZE    128

#define PRM_CHANNELSCNT		0
#define PRM_FIRSTCHANNEL	1
#define PRM_TRACEMASK		2
#define PRM_SVRTFILTER		3
#define PRM_SENDCALLACK		4
#define PRM_SENDACM			5
#define PRM_LOCALIP			6
#define PRM_MSCIP			7
#define PRM_CDPN			8
#define PRM_CGPN			9
#define PRM_MODE			10
#define PRM_FRAGMENT		11
#define PRM_SCPIP			12
#define PRM_SCPPORT			13
#define PRM_IPTDEVICESCNT	14
//---------------------------------------------------------------------------
// Working modes
#define MODE_AUTORESPONDER	0
#define MODE_SSP			1
//---------------------------------------------------------------------------
#define DEFAULT_ERRLOG_FILTER	TRC_ERROR
#define DEFAULT_CHANNELSCNT		2
#define DEFAULT_FIRSTCHANNEL	0
#define DEFAULT_TRACEMASK		255
#define DEFAULT_SEVERITYFILTER	0
#define DEFAULT_SENDCALLACK		0
#define DEFAULT_SENDACM			0
#define DEFAULT_MODE			MODE_AUTORESPONDER
#define DEFAULT_FRAGMENT		"hello.wav"
#define DEFAULT_SCPIP			"127.0.0.1"
#define DEFAULT_SCPPORT			10000
#define DEFAULT_IPTDEVICESCNT	120
//---------------------------------------------------------------------------
struct T_ParamDef {
	int ID;
	char Name[MAXPARAMSIZE];
};
const struct T_ParamDef Parameters[] = {
		{PRM_CHANNELSCNT,"ChannelsCount"},
		{PRM_FIRSTCHANNEL,"FirstChannel" },
		{PRM_TRACEMASK,  "TraceMask"},
		{PRM_SVRTFILTER, "SeverityFilter"},
		{PRM_SENDCALLACK,"SendCallAck"},
		{PRM_SENDACM,    "SendACM"},
		{PRM_LOCALIP,    "LocalIP"},
		{PRM_MSCIP,      "MSCIP" },
		{PRM_CDPN,       "CdPN" },
		{PRM_CGPN,       "CgPN" },
		{PRM_MODE,			"Mode"},
		{ PRM_FRAGMENT,   "Fragment" },
		{PRM_SCPIP,			"ScpIP"},
		{PRM_SCPPORT,		"ScpPort"},
		{PRM_IPTDEVICESCNT,		"IPTDevicesCount" },
};
//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------
#define MAXDXEVENT	10
#define DXEVENTNAMESIZE	32
static const char DxEventNames[MAXDXEVENT][DXEVENTNAMESIZE] = { "TDX_PLAY","TDX_RECORD","TDX_GETDIGIT","TDX_DIAL","TDX_CALLP","TDX_CST","TDX_SETHOOK","TDX_WINK","X","TDX_PLAYTONE" };
//---------------------------------------------------------------------------
typedef struct {   /////// Дескриптор вызова
	CRN crn;
	int SState;
} T_CALL_INFO;

typedef struct {   /////// Дескриптор ресурса
	int N;   // хранит индекс массива. Нужно для случая когда приходит указатель на элемент массива и нужно узнать этот индекс
	int hdVoice;
	int hdDti;
	LINEDEV hdLine;
	int PState;   // Program state
	int VState;   // Voice resourse state
	T_CALL_INFO Calls[MAX_CALLS];
	char CgPN[MAX_NUMSIZE];
	char CdPN[MAX_NUMSIZE];
	char RdPN[MAX_NUMSIZE];
	int reasonCode;	// Причина переадресации
	int blocked;    // Линия разблокирована/заблокирована
	int VReady;     // признак готовности голосового ресурса. Будет использватся перед WaitCall т.к. обнаружено что иногде на срабатывает GetRecourceH
	DX_IOTT iott;
	DX_XPB  xpb;
	DV_DIGIT  digbuf;
	DV_TPT tpt[3];
	GC_MAKECALL_BLK makecallblk;
} T_CHAN_INFO;


int TraceMask;
int SeverityFilter;
int TotalChannels;  // Number of channels in program instance
int FirstChannel;
int IPTDevicesCnt;
int SendCallAck;   // Send Ringing or no
int SendACM;
int LocalIP;
char sLocalIP[MAXPARAMSIZE];
char MSCIP[MAXPARAMSIZE];
char CdPN[MAXPARAMSIZE];
char CgPN[MAXPARAMSIZE];
char defaultFragment[MAXPARAMSIZE];
int Mode;
char scpIP[MAXPARAMSIZE];
int scpPort;


// Statistics
// TotalChannels take from global totalChannels variable
int stUnblocked = 0;
int stUsed = 0;
int stCallCntPerInterval = 0;

T_CHAN_INFO ChannelInfo[MAXCHANS + 1];

void LogWrite( const char *, int Svrt );
void Log( int Src, int Svrt, int Line, const char *format, ... );
void LogGC( int Svrt, int Line, int event, const char * );
void LogDX( int Svrt, int Line, int event, const char * );
void LogFunc( int Line, const char *FuncName, int ret );
void init_srl_mode();
static void intr_hdlr( int receivedSignal );
void LoadSettings();
void InitLogFile();
void InitDialogicLibs();
void InitNetwork();
void Deinit();
void processGlobalCall();
void processNetwork();
bool processPacket(unsigned int channel, const char *data, size_t len);
bool checkAppExitCondition();
void process_event();
void InitChannels();
void RegNewCall( int LineNo, CRN crn, int State );
int  GetSIPRdPN( GC_PARM_BLK	*paramblkp, std::string & RdPN, std::string & reason, int * reasonCode );
int  GetCallNdx( int LineNo, CRN crn );
int  GetIndexByVoice( int );
int  FindParam( char *ParamName );
bool InitPlayFragment( int index, const char *filename );
void InitDisconnect( int index, int reason = GC_NORMAL_CLEARING );
void InitNewCall( int index );
int  reasonCodeIP2reasonCodeDialogic(unsigned int reasonCodeIP);
void writeStatistics();