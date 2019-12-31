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
#define STATINTERVAL	10
#define MAXEXECUTORS	20
//---------------------------------------------------------------------------
#define TRC_CORE	1
#define TRC_GC		2
#define TRC_DX		4
#define TRC_SETT	8
#define TRC_DB		16
#define TRC_DBTR	32
//---------------------------------------------------------------------------
#define LOGSTRSIZE	256
#define DBERROR_FIELD_LEN 256
#define DBSTR_PARAM_LEN   64
#define MAX_DEVNAME	64		// Maximum length of device name											*/
#define MAX_NUMSIZE	128
//---------------------------------------------------------------------------
#define VST_IDLE        0
#define VST_PLAY        1
//---------------------------------------------------------------------------
#define PST_IDLE        0
#define PST_DBREQ       1
#define PST_PLAY	2
#define PST_TERMINATION	3
//---------------------------------------------------------------------------
#define MAXCHANS	2000  // Potential maximum of configured channels
#define MAX_CALLS       2
//---------------------------------------------------------------------------
#define MAXPARAMSIZE    64
#define MAXPARAMS	15

#define PRM_CHANNELSCNT	0
#define PRM_TRACEMASK	1
#define PRM_SVRTFILTER	2
#define PRM_SENDCALLACK 3
#define PRM_SENDACM	4
#define PRM_LOCALIP	5
#define PRM_DBNAME	6
#define PRM_DBUSER	7
#define PRM_DBPASSWORD	8
#define PRM_WAVPATH1	9
#define PRM_WAVPATH2	10
#define PRM_PLAYDEFAULT	11
#define PRM_DEFAULTFRGM	12
#define PRM_PRIORITY	13
#define PRM_SERVICEKEY	14
//---------------------------------------------------------------------------
#define DBR_OK		0
#define DBR_ERROR	-1
//---------------------------------------------------------------------------
#define DEFAULT_TOTALCHANNELS	2
#define DEFAULT_TRACEMASK	255
#define DEFAULT_SEVERITYFILTER	0
#define DEFAULT_SENDCALLACK	0
#define DEFAULT_SENDACM		0
#define DEFAULT_PRIORITY	0
#define DEFAULT_SERVICEKEY	1024
#define DEFAULT_PLAYDEFAULT	0
//---------------------------------------------------------------------------
struct T_ParamDef {
	int ID;
	char Name[MAXPARAMSIZE];
};
const struct T_ParamDef Parameters[MAXPARAMS] = {
		{PRM_CHANNELSCNT,"ChannelsCount"},
		{PRM_TRACEMASK,  "TraceMask"},
		{PRM_SVRTFILTER, "SeverityFilter"},
		{PRM_SENDCALLACK,"SendCallAck"},
		{PRM_SENDACM,    "SendACM"},
		{PRM_LOCALIP,    "LocalIP"},
		{PRM_DBNAME,     "DBName"},
		{PRM_DBUSER,     "DBUser"},
		{PRM_DBPASSWORD, "DBPassword"},
		{PRM_WAVPATH1,   "WAVPath1"},
		{PRM_WAVPATH2,   "WAVPath2"},
		{PRM_PLAYDEFAULT,   "PlayDefault"},
		{PRM_DEFAULTFRGM,   "DefaultFragmentN"},
		{PRM_PRIORITY,   "Priority"},
		{PRM_SERVICEKEY,   "ServiceKey"}
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
	int blocked;    // Линия разблокирована/заблокирована
	int VReady;     // признак готовности голосового ресурса. Будет использватся перед WaitCall т.к. обнаружено что иногде на срабатывает GetRecourceH
	DX_IOTT iott;
	DX_XPB  xpb;
	DV_DIGIT  digbuf;
	DV_TPT tpt[3];
} T_CHAN_INFO;


int TraceMask;
int SeverityFilter;
int TotalChannels;  // Number of channels in program instance
int SendCallAck;   // Send Ringing or no
int SendACM;
int LocalIP;
char DBUser[MAXPARAMSIZE];
char DBPassword[MAXPARAMSIZE];
char DBName[MAXPARAMSIZE];
char WAVPath1[128];
char WAVPath2[128];
int  ServiceKey;
int  Priority;
int  PlayDefault;
//char DefaultFragment[64];
int DefaultFragmentN;


// Statistics
// TotalChannels take from global totalChannels variable
int stUnblocked;
int stUsed;
double stCallsPerSec;
int CurrentDBOpCnt;
int stAvgDBOpDur;
// auxilary statistics
int stCallCntPerInterval;
int stTotalDBTimePerInterval;
int stDBReqPerInterval;

T_CHAN_INFO ChannelInfo[MAXCHANS + 1];

void LogWrite( const char * );
void Log( int Src, int Channel, const char *msg, int Svrt );
void LogGC( int Channel, int event, const char *, int Svrt );
void LogDX( int Channel, int event, const char *, int Svrt );
void LogFunc( int Channel, const char *FuncName, int ret );
void init_srl_mode();
static void intr_hdlr( void );
void LoadSettings();
void InitLogFile();
void InitDialogicLibs();
void Deinit();
void process_event( void );
void InitChannels();
void RegNewCall( int LineNo, CRN crn, int State );
int GetSIPRdPN( GC_PARM_BLK	*paramblkp, std::string & RdPN, std::string & reason );
int  GetCallNdx( int LineNo, CRN crn );
int  GetIndexByVoice( int );
int  FindParam( char *ParamName );
void InitPlayFragment( int index );
void InitDisconnect( int index );
