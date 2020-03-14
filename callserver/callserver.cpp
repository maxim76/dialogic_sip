/* OS Header Files */
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <sys/timeb.h>
#include <fcntl.h>
/* OS Socket API Headers */
#ifdef _WIN32
#include <winsock2.h>
#include <io.h>
#include <direct.h>
#else
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#endif

#include <algorithm>
#include <unordered_map>

#include "callserver.hpp"
#include "udp_request.hpp"


#ifdef WIN32
#pragma comment(lib, "libsrlmt.lib")
#pragma comment(lib, "libgc.lib")
#pragma comment(lib, "LIBDXXMT.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable : 4996)
#endif

static int		interrupted = NO;	/* Flag for user interrupted signals		 */
time_t			start_time;		// To note down the start time
FILE *fLog;
FILE *fErrorLog;
FILE *fStat;

int main( void )
{
	int ret;
	int i;
	int fCanClose = 0;
	char str[LOGSTRSIZE];

	time( &start_time );
	InitLogFile();
	Log( TRC_CORE, TRC_INFO, -1, "********** IVR server started ***********" );

	// Give control to Event Handler

#ifdef _WIN32
	signal( SIGINT, (void( __cdecl* )(int)) intr_hdlr );
	signal( SIGTERM, (void( __cdecl* )(int)) intr_hdlr );
#else
	signal( SIGHUP,	 intr_hdlr );
	signal( SIGQUIT, intr_hdlr );
	signal( SIGINT,  intr_hdlr );
	signal( SIGTERM, intr_hdlr );
#endif

	init_srl_mode();									// set SRL mode to ASYNC, polled
	LoadSettings();

	InitDialogicLibs();
	InitChannels();
	InitNetwork();

	// Forever loop where the main work is done - wait for an event or user requested exit
	Log( TRC_CORE, TRC_INFO, -1, "Entering Working State" );
	for(;;)
	{
		ret = sr_waitevt( 500 );					// 1/2 second
		if(ret != -1)
		{
			process_event();
		}
		if(interrupted == YES)	// flag set in intr_hdlr() 
		{
			fCanClose = 1;
			for(i = 0; i < TotalChannels; i++)
			{
				switch(ChannelInfo[i].PState)
				{
				case PST_NULL:
				case PST_IDLE:
					ret = gc_Close( ChannelInfo[i].hdLine );
					LogFunc( i, "gc_Close()", ret );
					ChannelInfo[i].PState = PST_SHUTDOWN;
				}

				if(ChannelInfo[i].PState != PST_SHUTDOWN)
				{
					sprintf( str, "Waiting for channel [%d], state %d", i, ChannelInfo[i].PState );
					Log( TRC_CORE, TRC_DUMP, -1, str );
					fCanClose = 0;
				}
			}
			if(fCanClose)
			{
				Log( TRC_CORE, TRC_WARNING, -1, "Application Stopped" );
				fclose( fLog );
				fclose( fErrorLog );
				fclose( fStat );
				return 0;
			}
		}
	}
}

/****************************************************************
*			NAME: void init_srl_mode(void)
* DESCRIPTION: Inits SR mode
*		RETURNS: None
*	  CAUTIONS: NA
****************************************************************/
void init_srl_mode()
{
	/*
	SRL MODEL TYPE - Single Threaded Async, event handler will be called in the Main thread
	*/
#ifdef _WIN32
	int	mode = SR_STASYNC | SR_POLLMODE;
#else
	int	mode = SR_POLLMODE;
#endif                                                                     

	/* Set SRL mode */
#ifdef _WIN32
	if(sr_setparm( SRL_DEVICE, SR_MODELTYPE, &mode ) == -1)
	{
		Log( TRC_CORE, TRC_CRITICAL, -1, "Unable to set to SR_STASYNC | SR_POLLMODE" );
		exit( 1 );
	}
	Log( TRC_CORE, TRC_DUMP, -1, "SRL Model Set to SR_STASYNC | SR_POLLMODE" );
#else
	if(sr_setparm( SRL_DEVICE, SR_MODEID, &mode ) == -1)
	{
		Log( TRC_CORE, TRC_CRITICAL, -1, "Unable to set mode ID to SR_POLLMODE " );
		exit( 1 );
	}

	Log( TRC_CORE, TRC_DUMP , -1, "SRL mode ID set to SR_POLLMODE" );
#endif
}
/***************************************************************************
 *			NAME: void intr_hdlr()
 *DESCRIPTION: Handler called when one of the following signals is
 *					received: SIGHUP, SIGINT, SIGQUIT, SIGTERM.
 *					This function stops I/O activity on all channels and
 *					closes all the channels.
 *		  INPUT: None
 *		RETURNS: None.
  ***************************************************************************/
static void intr_hdlr( int receivedSignal )
{
	Log( TRC_CORE, TRC_WARNING, -1, " *******Received User Interrupted Signal *******" );
	Deinit();
	interrupted = YES;
}
//---------------------------------------------------------------------------
void InitLogFile()
{
	const char * logDir = "./Logs/";
	// Create directory for log files if it does not exist
	int res;
#ifdef _WIN32
	res = _mkdir( logDir );
#else
	res = mkdir( logDir, 0733 );
#endif
	if ((res != 0) && (errno != EEXIST))
	{
		printf( "Cannot create log dir. Error: %d (%s)\n", errno, strerror(errno) );
		exit( 1 );
	}

	char sFN[64];
	struct tm *tblock;
	tblock = localtime( &start_time );

	// Full log file
	sprintf( sFN, "%s/ivrserv_%04d%02d%02d_%02d%02d%02d.txt", logDir, tblock->tm_year + 1900, tblock->tm_mon, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec );
	if((fLog = fopen( sFN, "w" )) == NULL)
	{
		printf( "Cannot create log file. Termination.\n" );
		exit( 1 );
	}

	// Errors only
	sprintf( sFN, "%s/errors_%04d%02d%02d_%02d%02d%02d.txt", logDir, tblock->tm_year + 1900, tblock->tm_mon, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec );
	if((fErrorLog = fopen( sFN, "w" )) == NULL)
	{
		printf( "Cannot create error log file. Termination.\n" );
		exit( 1 );
	}

	// Statistics
	sprintf( sFN, "%s/stat_%04d%02d%02d_%02d%02d%02d.txt", logDir, tblock->tm_year + 1900, tblock->tm_mon, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec );
	if((fStat = fopen( sFN, "w" )) == NULL)
	{
		printf( "Cannot create statistics file. Termination.\n" );
		exit( 1 );
	}

}
//---------------------------------------------------------------------------
void InitDialogicLibs()
{
	Log( TRC_CORE, TRC_DUMP, -1, "Init dialogic Libs" );
	int i;
	char str[LOGSTRSIZE];
	GC_CCLIB_STATUSALL	cclib_status_all;

	// User-configured style of gc_Start
	IPCCLIB_START_DATA cclibData;
	IP_VIRTBOARD virtBoard;
	GC_START_STRUCT gcLibStart;

	CCLIB_START_STRUCT ccLibStart[] = {
		{(char *)"GC_SS7_LIB",NULL},
		{(char *)"GC_DM3CC_LIB",NULL},
		{(char *)"GC_IPM_LIB",NULL},
		{(char *)"GC_H3R_LIB",&cclibData}
	};

	INIT_IP_VIRTBOARD( &virtBoard );
	if(TotalChannels > 120)
	{
		virtBoard.total_max_calls = TotalChannels;
		virtBoard.h323_max_calls = 0;
		virtBoard.sip_max_calls = TotalChannels;
	}
	if(LocalIP != -1)
	{
		virtBoard.localIP.ip_ver = IPVER4;
		virtBoard.localIP.u_ipaddr.ipv4 = (unsigned int)LocalIP;//htonl(0x0ade0966);
	}
	virtBoard.sip_msginfo_mask = IP_SIP_MSGINFO_ENABLE;
	INIT_IPCCLIB_START_DATA( &cclibData, 1, &virtBoard );

	gcLibStart.cclib_list = ccLibStart;
	gcLibStart.num_cclibs = 4;

	if(gc_Start( &gcLibStart ) != GC_SUCCESS)
	{
		sprintf( str, "gc_Start() Failed" );
		Log( TRC_CORE, TRC_CRITICAL, -1, str );
		exit( 1 );
	}

	if(gc_CCLibStatusEx( (char *)"GC_ALL_LIB", &cclib_status_all ) != GC_SUCCESS)
	{
		sprintf( str, "gc_CCLibStatusEx(GC_ALL_LIB, &cclib_status_all) Failed" );
		Log( TRC_CORE, TRC_CRITICAL, -1, str );
		exit( 1 );
	}
	strcpy( str, " Call Control Library Status:" );
	Log( TRC_CORE, TRC_DUMP, -1, str );
	for(i = 0; i < GC_TOTAL_CCLIBS; i++)
	{
		switch(cclib_status_all.cclib_state[i].state)
		{
		case GC_CCLIB_CONFIGURED:
			sprintf( str, "		  %s - configured", cclib_status_all.cclib_state[i].name );
			break;

		case GC_CCLIB_AVAILABLE:
			sprintf( str, "		  %s - available", cclib_status_all.cclib_state[i].name );
			break;

		case GC_CCLIB_FAILED:
			sprintf( str, "		  %s - is not available for use", cclib_status_all.cclib_state[i].name );
			break;

		default:
			sprintf( str, "		  %s - unknown CCLIB status", cclib_status_all.cclib_state[i].name );
			break;


		}
		Log( TRC_CORE, TRC_DUMP, -1, str );
	}
	strcpy( str, " ----------------------------" );
	Log( TRC_CORE, TRC_DUMP, -1, str );
}
//---------------------------------------------------------------------------
void Deinit()
{
	Log( TRC_CORE, TRC_INFO, -1, "Deinitialization" );
	int i;
	int ret;
	for(i = 0; i<TotalChannels; i++)
	{
		switch(ChannelInfo[i].PState)
		{
		case PST_CALLING:
			ChannelInfo[i].PState = PST_RELEASING;
			ret = gc_DropCall( ChannelInfo[i].Calls[0].crn, GC_NORMAL_CLEARING, EV_ASYNC );
			LogFunc( i, "gc_DropCall()", ret );
			break;
		case PST_PLAY:
			ret = dx_stopch( ChannelInfo[i].hdVoice, EV_ASYNC );
			LogFunc( i, "dx_stopch()", ret );
			ChannelInfo[i].PState = PST_RELEASING;
			ret = gc_DropCall( ChannelInfo[i].Calls[0].crn, GC_NORMAL_CLEARING, EV_ASYNC );
			LogFunc( i, "gc_DropCall()", ret );
			break;
			/*
		case PST_TERMINATION:
			ChannelInfo[i].PState = PST_INTERRUPTING;
			break;
		default:   // NULL, IDLE
			ChannelInfo[i].PState = PST_INTERRUPTED;
			ret = gc_Close( ChannelInfo[i].hdLine );
			LogFunc( i, "gc_Close()", ret );
			*/
		}
	}
}
//---------------------------------------------------------------------------
void Log( int Src, int Svrt, int Line, const char *format, ... )
{
	char str[LOGSTRSIZE];	// Full log string
	char msg[LOGSTRSIZE];	// Message part

	// Form log source part
	std::string logSource = "";
	switch(Src)
	{
	case TRC_CORE: logSource = "CORE"; break;
	case TRC_GC:   logSource = "GC  "; break;
	case TRC_DX:   logSource = "DX  "; break;
	case TRC_SETT: logSource = "SETT"; break;
	}

	// Form verbal severity description
	std::string severityDescr = "";
	switch(Svrt)
	{
	case TRC_DUMP:     severityDescr = "DUMP    "; break;
	case TRC_INFO:     severityDescr = "INFO    "; break;
	case TRC_WARNING:  severityDescr = "WARNING "; break;
	case TRC_ERROR:    severityDescr = "ERROR   "; break;
	case TRC_CRITICAL: severityDescr = "CRITICAL"; break;
	}

	// Form message
	va_list args;
	va_start( args, format );
	vsprintf( msg, format, args );
	va_end( args );

	if(Line >= 0)
		snprintf( str, LOGSTRSIZE, "[%s] %s [%04d] %s", logSource.c_str(), severityDescr.c_str(), Line, msg );
	else
		snprintf( str, LOGSTRSIZE, "[%s] %s [    ] %s", logSource.c_str(), severityDescr.c_str(), msg );

	LogWrite( str, Svrt );
}
//---------------------------------------------------------------------------
void LogGC( int Svrt, int Line, int event, const char *msg )
{
	char str[LOGSTRSIZE];
	char sEvt[GCEVENTNAMESIZE];
	strncpy( sEvt, GcEventNames[event - DT_GC], GCEVENTNAMESIZE );
	if(sEvt[0] == 0) snprintf( sEvt, GCEVENTNAMESIZE, "0x%04x", event );
	snprintf(str, LOGSTRSIZE, "%s %s", sEvt, msg );
	Log( TRC_GC, Svrt, Line, str );
}
//---------------------------------------------------------------------------
void LogDX( int Svrt, int Line, int event, const char *msg )
{
	char str[LOGSTRSIZE];
	char sEvt[GCEVENTNAMESIZE];
	strncpy( sEvt, DxEventNames[event - 0x81], GCEVENTNAMESIZE );
	if(sEvt[0] == 0) snprintf( sEvt, GCEVENTNAMESIZE, "0x%04x", event );
	snprintf( str, LOGSTRSIZE, "%s %s", sEvt, msg );
	Log( TRC_DX, Svrt, Line, str );
}
//---------------------------------------------------------------------------
void LogFunc( int Line, const char *FuncName, int ret )
{
	char str[LOGSTRSIZE];
	sprintf( str, "%s : %s", FuncName, (ret == GC_SUCCESS) ? "Ok" : "Error" );
	Log( TRC_CORE, (ret == GC_SUCCESS) ? 0 : 2, Line, str );
}
//---------------------------------------------------------------------------
void LogWrite( const char *msg, int Svrt )
{
	/*
	// TODO: Implement filtering (non-error log only) if required
	if((TraceMask & Src) == 0) return;
	if(Svrt < SeverityFilter)  return;
	*/

	time_t timer = time( NULL );
	struct tm *tblock;
	tblock = localtime( &timer );
	printf( "%04d/%02d/%02d %02d:%02d:%02d %s\n", tblock->tm_year + 1900, tblock->tm_mon, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec, msg );
	fprintf( fLog, "%04d/%02d/%02d %02d:%02d:%02d %s\n", tblock->tm_year + 1900, tblock->tm_mon, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec, msg );
	fflush( fLog );

	if(Svrt >= DEFAULT_ERRLOG_FILTER)
	{
		fprintf( fErrorLog, "%04d/%02d/%02d %02d:%02d:%02d %s\n", tblock->tm_year + 1900, tblock->tm_mon, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec, msg );
		fflush( fErrorLog );
	}
}
//---------------------------------------------------------------------------
void LogGC_INFO( int index, GC_INFO *a_Info )
{
	Log( TRC_GC, TRC_DUMP, index, "LogGC_INFO() : gcValue = 0x%x\n", a_Info->gcValue );
	Log( TRC_GC, TRC_DUMP, index, "LogGC_INFO() : gcMsg = %s\n", a_Info->gcMsg );
	Log( TRC_GC, TRC_DUMP, index, "LogGC_INFO() : ccLibId = %d\n", a_Info->ccLibId );
	Log( TRC_GC, TRC_DUMP, index, "LogGC_INFO() : ccLibName = %s\n", a_Info->ccLibName );
	Log( TRC_GC, TRC_DUMP, index, "LogGC_INFO() : ccValue = 0x%x\n", a_Info->ccValue );
	Log( TRC_GC, TRC_DUMP, index, "LogGC_INFO() : ccMsg = %s\n", a_Info->ccMsg );
	Log( TRC_GC, TRC_DUMP, index, "LogGC_INFO() : additionalInfo = %s\n", a_Info->additionalInfo );
}
//---------------------------------------------------------------------------
int FindParam( char *ParamName )
{
	int paramCount = sizeof(Parameters)/sizeof(T_ParamDef);
	for(int i = 0; i < paramCount; i++)
		if(strcmp( ParamName, Parameters[i].Name ) == 0)
			return Parameters[i].ID;
	return -1;
}
//---------------------------------------------------------------------------
void LoadSettings()
{
	Log( TRC_SETT, TRC_DUMP, -1, "Load Settings" );
	TotalChannels = DEFAULT_CHANNELSCNT;
	TraceMask = DEFAULT_TRACEMASK;
	SeverityFilter = DEFAULT_SEVERITYFILTER;
	SendCallAck = DEFAULT_SENDCALLACK;
	SendACM = DEFAULT_SENDACM;
	strncpy( defaultFragment, DEFAULT_FRAGMENT, MAXPARAMSIZE );
	strncpy( scpIP, DEFAULT_SCPIP, MAXPARAMSIZE );
	scpPort = DEFAULT_SCPPORT;

	char logstr[LOGSTRSIZE];
	char str[MAXPARAMSIZE];
	char ParamName[MAXPARAMSIZE];
	char ParamValue[MAXPARAMSIZE];
	int sl;
	int n, np, nv;
	FILE *f = fopen( "params.ini", "r" );
	if(f == NULL)
	{
		Log( TRC_SETT, TRC_CRITICAL, -1, "Cannot find 'params.ini'" );
		exit( 1 );
	}
	while(!feof( f ))
	{
		if(fgets( str, MAXPARAMSIZE, f ) == NULL) break;
		sl = strlen( str );
		if(sl < 3) continue;
		// Skip comments
		if((str[0]==';')||(str[0]=='#')) continue;
		n = 0;
		np = 0;
		while((str[n] != '=') && (n < sl))
		{
			ParamName[np++] = str[n++];
			ParamName[np] = 0;
		}
		if(n >= sl) continue; // String does not have part '=Value'
		n++;  // Skip symbol '='
		nv = 0;
		while((n < sl) && (str[n] != 10) && (str[n] != 13))
		{
			ParamValue[nv++] = str[n++];
			ParamValue[nv] = 0;
		}
		if((strlen( ParamName ) < 1) || (strlen( ParamValue ) < 1)) continue;
		// At this point we have ParanName/ParamValue pair
		int PN = FindParam( ParamName );
		if(PN == -1)
		{
			sprintf( logstr, "Unknown parameter definition: %s=%s", ParamName, ParamValue );
			Log( TRC_SETT, TRC_ERROR, -1, logstr );
		}
		else
		{
			switch(PN)
			{
			case PRM_CHANNELSCNT:
				if(sscanf( ParamValue, "%d", &TotalChannels ) == 1)
				{
					sprintf( logstr, "Set TotalChannels = %d", TotalChannels );
					Log( TRC_SETT, TRC_DUMP, -1, logstr );
				}
				else
				{
					TotalChannels = DEFAULT_CHANNELSCNT;
					sprintf( logstr, "Wrong parameter set: %s=%s", ParamName, ParamValue );
					Log( TRC_SETT, TRC_ERROR, -1, logstr );
				}
				break;
			case PRM_FIRSTCHANNEL:
				if(sscanf( ParamValue, "%d", &FirstChannel ) == 1)
				{
					sprintf( logstr, "Set FirstChannel = %d", FirstChannel );
					Log( TRC_SETT, TRC_DUMP, -1, logstr );
				}
				else
				{
					FirstChannel = DEFAULT_FIRSTCHANNEL;
					sprintf( logstr, "Wrong parameter set: %s=%s", ParamName, ParamValue );
					Log( TRC_SETT, TRC_ERROR, -1, logstr );
				}
				break;
			case PRM_TRACEMASK:
				if(sscanf( ParamValue, "%d", &TraceMask ) == 1)
				{
					sprintf( logstr, "Set TraceMask = %d", TraceMask );
					Log( TRC_SETT, TRC_DUMP, -1, logstr );
				}
				else
				{
					TraceMask = DEFAULT_TRACEMASK;
					sprintf( logstr, "Wrong parameter set: %s=%s", ParamName, ParamValue );
					Log( TRC_SETT, TRC_ERROR, -1, logstr );
				}
				break;
			case PRM_SVRTFILTER:
				if(sscanf( ParamValue, "%d", &SeverityFilter ) == 1)
				{
					sprintf( logstr, "Set SeverityFilter = %d", SeverityFilter );
					Log( TRC_SETT, TRC_DUMP, -1, logstr );
				}
				else
				{
					SeverityFilter = DEFAULT_SEVERITYFILTER;
					sprintf( logstr, "Wrong parameter set: %s=%s", ParamName, ParamValue );
					Log( TRC_SETT, TRC_ERROR, -1, logstr );
				}
				break;
			case PRM_SENDCALLACK:
				if(sscanf( ParamValue, "%d", &SendCallAck ) == 1)
				{
					sprintf( logstr, "Set SendCallAck = %d", SendCallAck );
					Log( TRC_SETT, TRC_DUMP, -1, logstr );
				}
				else
				{
					SendCallAck = DEFAULT_SENDCALLACK;
					sprintf( logstr, "Wrong parameter set: %s=%s", ParamName, ParamValue );
					Log( TRC_SETT, TRC_ERROR, -1, logstr );
				}
				break;
			case PRM_SENDACM:
				if(sscanf( ParamValue, "%d", &SendACM ) == 1)
				{
					sprintf( logstr, "Set SendACM = %d", SendACM );
					Log( TRC_SETT, TRC_DUMP, -1, logstr );
				}
				else
				{
					SendACM = DEFAULT_SENDACM;
					sprintf( logstr, "Wrong parameter set: %s=%s", ParamName, ParamValue );
					Log( TRC_SETT, TRC_ERROR, -1, logstr );
				}
				break;
			case PRM_LOCALIP:
				if(sscanf( ParamValue, "%s", sLocalIP ) == 1)
				{
					LocalIP = inet_addr( sLocalIP );
					if(LocalIP != -1)
					{
						sprintf( logstr, "LocalIP = %s", sLocalIP );
						Log( TRC_SETT, TRC_DUMP, -1, logstr );
					}
					else
					{
						sprintf( logstr, "Wrong LocalIP settings : [%s]", sLocalIP );
						Log( TRC_SETT, TRC_ERROR, -1, logstr );
					}
				}
				else
				{
					LocalIP = -1;
					sprintf( logstr, "Wrong parameter set: %s=%s", ParamName, ParamValue );
					Log( TRC_SETT, TRC_ERROR, -1, logstr );
				}
				break;
			case PRM_MSCIP:
				if(sscanf( ParamValue, "%s", MSCIP ) == 1)
				{
					sprintf( logstr, "Set MSCIP = '%s'", MSCIP );
					Log( TRC_SETT, TRC_DUMP, -1, logstr );
				}
				else
				{
					sprintf( logstr, "Wrong parameter set: %s=%s", ParamName, ParamValue );
					Log( TRC_SETT, TRC_ERROR, -1, logstr );
				}
				break;
			case PRM_CDPN:
				if(sscanf( ParamValue, "%s", CdPN ) == 1)
				{
					sprintf( logstr, "Set CdPN = '%s'", CdPN );
					Log( TRC_SETT, TRC_DUMP, -1, logstr );
				}
				else
				{
					sprintf( logstr, "Wrong parameter set: %s=%s", ParamName, ParamValue );
					Log( TRC_SETT, TRC_ERROR, -1, logstr );
				}
				break;
			case PRM_CGPN:
				if(sscanf( ParamValue, "%s", CgPN ) == 1)
				{
					sprintf( logstr, "Set CgPN = '%s'", CgPN );
					Log( TRC_SETT, TRC_DUMP, -1, logstr );
				}
				else
				{
					sprintf( logstr, "Wrong parameter set: %s=%s", ParamName, ParamValue );
					Log( TRC_SETT, TRC_ERROR, -1, logstr );
				}
				break;
			case PRM_FRAGMENT:
				if(sscanf( ParamValue, "%s", defaultFragment ) == 1)
				{
					sprintf( logstr, "Set Default Fragment = '%s'", defaultFragment );
					Log( TRC_SETT, TRC_DUMP, -1, logstr );
				}
				else
				{
					strncpy( defaultFragment, DEFAULT_FRAGMENT, MAXPARAMSIZE );
					sprintf( logstr, "Wrong parameter set: %s=%s", ParamName, ParamValue );
					Log( TRC_SETT, TRC_ERROR, -1, logstr );
				}
				break;
			case PRM_MODE:
				if(sscanf( ParamValue, "%d", &Mode ) == 1)
				{
					sprintf( logstr, "Set Mode = %d", Mode );
					Log( TRC_SETT, TRC_DUMP, -1, logstr );
				}
				else
				{
					Mode = DEFAULT_MODE;
					sprintf( logstr, "Wrong parameter set: %s=%s", ParamName, ParamValue );
					Log( TRC_SETT, TRC_ERROR, -1, logstr );
				}
				break;
			case PRM_SCPIP:
				if(sscanf( ParamValue, "%s", scpIP ) == 1)
				{
					sprintf( logstr, "Set SCP IP = '%s'", scpIP );
					Log( TRC_SETT, TRC_DUMP, -1, logstr );
				}
				else
				{
					strncpy( scpIP, DEFAULT_SCPIP, MAXPARAMSIZE );
					sprintf( logstr, "Wrong parameter set: %s=%s", ParamName, ParamValue );
					Log( TRC_SETT, TRC_ERROR, -1, logstr );
				}
				break;
			case PRM_SCPPORT:
				if(sscanf( ParamValue, "%d", &scpPort ) == 1)
				{
					sprintf( logstr, "Set SCP Port = %d", scpPort );
					Log( TRC_SETT, TRC_DUMP, -1, logstr );
				}
				else
				{
					scpPort = DEFAULT_SCPPORT;
					sprintf( logstr, "Wrong parameter set: %s=%s", ParamName, ParamValue );
					Log( TRC_SETT, TRC_ERROR, -1, logstr );
				}
				break;
			default:
				sprintf( logstr, "Unimplemented initialization: %s=%s", ParamName, ParamValue );
				Log( TRC_SETT, TRC_ERROR, -1, logstr );
			}
		}
	}
	fclose( f );
}
//---------------------------------------------------------------------------
void InitChannels()
{
	Log( TRC_CORE, TRC_INFO, -1, "Init Channels" );
	int i, callndx;
	int ret;
	char str[LOGSTRSIZE];
	char FullDevName[MAX_DEVNAME];
	/*
	char sDst[MAX_NUMSIZE];
	char sSrc[MAX_NUMSIZE];
	GCLIB_MAKECALL_BLK	*gclib_makecallp;

	sprintf( sDst, "%s@%s", CdPN, MSCIP );
	sprintf( sSrc, "%s@%s", CgPN, sLocalIP );
	*/
	int absoluteChannelIndex;

	for(i = 0; i<TotalChannels; i++)
	{
		/*
		gclib_makecallp = (GCLIB_MAKECALL_BLKP)malloc( sizeof( GCLIB_MAKECALL_BLK ) );
		ChannelInfo[i].makecallblk.cclib = NULL;
		ChannelInfo[i].makecallblk.gclib = gclib_makecallp;
		gclib_makecallp->ext_datap = NULL;

		strcpy( gclib_makecallp->destination.address, sDst );
		gclib_makecallp->destination.address_type = GCADDRTYPE_TRANSPARENT;
		strcpy( gclib_makecallp->origination.address, sSrc );
		gclib_makecallp->origination.address_type = GCADDRTYPE_TRANSPARENT;
		*/
		ChannelInfo[i].blocked = 1;
		ChannelInfo[i].VReady = 0;
		ChannelInfo[i].N = i;
		ChannelInfo[i].PState = PST_NULL;
		for(callndx = 0; callndx < MAX_CALLS; callndx++)
		{
			ChannelInfo[i].Calls[callndx].crn = 0;
			ChannelInfo[i].Calls[callndx].SState = GCST_NULL;
		}
		absoluteChannelIndex = FirstChannel + i;
		sprintf( FullDevName, ":P_SIP:N_iptB1T%d:M_ipmB1C%d:V_dxxxB%dC%d",
			absoluteChannelIndex + 1,
			absoluteChannelIndex + 1,
			(absoluteChannelIndex / 4) + 1,
			(absoluteChannelIndex % 4) + 1 );
		ret = gc_OpenEx( &(ChannelInfo[i].hdLine), FullDevName, EV_ASYNC, (void *)i );
		sprintf( str, "gc_OpenEx(\"%s\") = %d", FullDevName, ret );
		LogFunc( i, str, ret );
		if(ret == GC_SUCCESS)
		{
			ChannelInfo[i].iott.io_fhandle = -1;
		}
	}
}
//---------------------------------------------------------------------------
void InitNetwork()
{
#ifdef WIN32
	// Initialize Windows socket library
	WSADATA wsaData;
	if(WSAStartup( MAKEWORD( 1, 1 ), &wsaData ) != 0)
	{
		fprintf( stderr, "WSAStartup() failed\n" );
		exit(-1);
	}
#endif

	UDPRequest request( scpIP, scpPort );
	if(!request.ready())
	{
		fprintf( stderr, "UDPRequest initialization failed\n" );
		exit( -1 );
	}
}
/****************************************************************
*			NAME: process_event(void)
* DESCRIPTION: Function to handle the GlobalCall Events
*		RETURNS: NA
*	  CAUTIONS: none
****************************************************************/
void process_event( void )
{
	METAEVENT	metaevent;
	char		str[LOGSTRSIZE];
	int		index;
	int		ret;
	int		i;

	int		evttype;
	CRN		TempCRN;
	char		st[MAX_NUMSIZE];
	GC_INFO		gc_error_info;
	int		CallNdx;
	int		hdDev;
	DX_CST		*cstp;
	GC_PARM_BLK	*parmblkp = NULL;  // gc_util_insert_parm_val creates data struct in memory itself
	//GC_PARM_DATAP curParm = NULL;

	long		state;

	if(gc_GetMetaEvent( &metaevent ) != GC_SUCCESS)
	{
		// serious problem - should never fail
		Log( TRC_CORE, TRC_ERROR, -1, "gc_GetMetaEvent() failed" );
		return;
	}

	if((metaevent.flags & GCME_GC_EVENT) != 0)
	{
		// Global Call Event
		index = (int)metaevent.usrattr;
		evttype = metaevent.evttype;
		TempCRN = metaevent.crn;
		CallNdx = GetCallNdx( index, TempCRN );
		switch(evttype)
		{
		case GCEV_OPENEX:
			LogGC( TRC_INFO, index, evttype, "Channel is opened" );
			/*
			ret = gc_GetResourceH( ChannelInfo[index].hdLine, &(ChannelInfo[index].hdVoice), GC_VOICEDEVICE );
			LogFunc( index, "gc_GetResourceH()", ret );
			ret = gc_SetAlarmNotifyAll( ChannelInfo[index].hdLine, ALARM_SOURCE_ID_NETWORK_ID, ALARM_NOTIFY );
			LogFunc( index, "gc_SetAlarmNotifyAll()", ret );
			gc_util_insert_parm_val( &parmblkp, IPSET_DTMF, IPPARM_SUPPORT_DTMF_BITMASK, sizeof( char ), IP_DTMF_TYPE_RFC_2833 );
			ret = gc_SetUserInfo( GCTGT_GCLIB_CHAN, ChannelInfo[index].hdLine, parmblkp, GC_ALLCALLS );
			LogFunc( index, "gc_SetUserInfo(Set DTMF mode)", ret );
			gc_util_delete_parm_blk( parmblkp );
			ret = dx_setevtmsk( ChannelInfo[index].hdVoice, DM_DIGITS );
			LogFunc( index, "dx_setevtmsk()", ret );
			*/
			break;
		case GCEV_UNBLOCKED:
			LogGC( TRC_INFO, index, evttype, "Channel is unblocked" );

			ret = gc_GetResourceH( ChannelInfo[index].hdLine, &(ChannelInfo[index].hdVoice), GC_VOICEDEVICE );
			LogFunc( index, "gc_GetResourceH()", ret );
			if(ret == GC_SUCCESS)
			{
				ChannelInfo[index].VReady = 1;
				stUnblocked++;
			}
			else
			{
			}
			ret = gc_SetAlarmNotifyAll( ChannelInfo[index].hdLine, ALARM_SOURCE_ID_NETWORK_ID, ALARM_NOTIFY );
			LogFunc( index, "gc_SetAlarmNotifyAll()", ret );
			gc_util_insert_parm_val( &parmblkp, IPSET_DTMF, IPPARM_SUPPORT_DTMF_BITMASK, sizeof( char ), IP_DTMF_TYPE_RFC_2833 );
			ret = gc_SetUserInfo( GCTGT_GCLIB_CHAN, ChannelInfo[index].hdLine, parmblkp, GC_ALLCALLS );
			LogFunc( index, "gc_SetUserInfo(Set DTMF mode)", ret );
			gc_util_delete_parm_blk( parmblkp );
			ret = dx_setevtmsk( ChannelInfo[index].hdVoice, DM_DIGITS );
			LogFunc( index, "dx_setevtmsk()", ret );

			ChannelInfo[index].blocked = 0;
			if(ChannelInfo[index].VReady)
			{
				ret = gc_WaitCall( ChannelInfo[index].hdLine, NULL, NULL, 0, EV_ASYNC );
				LogFunc( index, "gc_WaitCall()", ret );
			}
			else
			{
				Log( TRC_CORE, TRC_WARNING, index, "VReady = 0. Do not call gc_WaitCall() now" );
			}
			break;
		case GCEV_OPENEX_FAIL:
			LogGC( TRC_ERROR, index, evttype, "Failed to open channel" );
			break;
		case GCEV_OFFERED:
		{
			stUsed++;
			stCallCntPerInterval++;
			ChannelInfo[index].PState = PST_IDLE;
			RegNewCall( index, TempCRN, GCST_OFFERED );
			if(gc_GetCallInfo( TempCRN, DESTINATION_ADDRESS, st ) != -1)
				strncpy( ChannelInfo[index].CdPN, st, MAX_NUMSIZE ); else ChannelInfo[index].CdPN[0] = 0;
			if(gc_GetCallInfo( TempCRN, ORIGINATION_ADDRESS, st ) != -1)
				strncpy( ChannelInfo[index].CgPN, st, MAX_NUMSIZE ); else ChannelInfo[index].CgPN[0] = 0;
			std::string RdPN;
			std::string reason;
			int reasonCodeIP;
			if(GetSIPRdPN( (GC_PARM_BLKP)metaevent.extevtdatap, RdPN, reason, &reasonCodeIP ))
			{
				std::string fullRdPNInfo = RdPN + " (reason=" + reason + ")";
				strncpy( ChannelInfo[index].RdPN, fullRdPNInfo.c_str(), MAX_NUMSIZE );
				ChannelInfo[index].reasonCode = reasonCodeIP;
			}
			else
			{
				ChannelInfo[index].RdPN[0] = 0;
				ChannelInfo[index].reasonCode = 0;
			}

			sprintf( str, "CgPN:[%s] CdPN:[%s] RdPN:[%s] (reason=%d)", ChannelInfo[index].CgPN, ChannelInfo[index].CdPN, ChannelInfo[index].RdPN, ChannelInfo[index].reasonCode );
			LogGC( TRC_INFO, index, evttype, str );
			int reasonCodeDialogic;
			switch(Mode)
			{
			case 1:		// Missed call
				switch(reasonCodeIP)
				{
				case 480:
					reasonCodeDialogic = IPEC_SIPReasonStatus480TemporarilyUnavailable;
					Log(TRC_CORE, TRC_INFO, index, "MissedCall: dropping with reason 480TemporarilyUnavailable");
					break;
				case 486:
					reasonCodeDialogic = IPEC_SIPReasonStatus486BusyHere;
					Log( TRC_CORE, TRC_INFO, index, "MissedCall: dropping with reason 486BusyHere" );
					break;
				default:
					Log( TRC_CORE, TRC_INFO, index, "MissedCall: no redirection reason. Dropping with reason NORMAL_CLEARING" );
					reasonCodeDialogic = GC_NORMAL_CLEARING;
				}
				InitDisconnect(index, reasonCodeDialogic );
				break;
			default:	// other modes, that requires connection
				if(SendCallAck > 0)
				{
					GC_CALLACK_BLK callack; /* type & number of digits to collect */
					memset( &callack, 0, sizeof( callack ) );
					if(gc_CallAck( TempCRN, &callack, EV_ASYNC ) != GC_SUCCESS)
					{
						gc_ErrorInfo( &gc_error_info );
						sprintf( str, "Error gc_CallAck() %s", gc_error_info.ccMsg );
						Log( TRC_CORE, TRC_ERROR, index, str );
					}
					else
					{
						LogFunc( index, "gc_CallAck", 0 );
					}
				}
				else
				{
					if(SendACM > 0)
					{
						ret = gc_AcceptCall( TempCRN, 0, EV_ASYNC );
						LogFunc( index, "gc_AcceptCall()", ret );
					}
					else
					{
						ret = gc_AnswerCall( TempCRN, 0, EV_ASYNC );
						LogFunc( index, "gc_AnswerCall()", ret );
					}
				}
			}
		}
		break;
		case GCEV_CALLPROC:
			LogGC( TRC_INFO, index, evttype, "" );
			if(SendACM > 0)
			{
				ret = gc_AcceptCall( TempCRN, 0, EV_ASYNC );
				LogFunc( index, "gc_AcceptCall()", ret );
			}
			else
			{
				ret = gc_AnswerCall( TempCRN, 0, EV_ASYNC );
				LogFunc( index, "gc_AnswerCall()", ret );
			}
			break;
		case GCEV_ACCEPT:
			LogGC( TRC_INFO, index, evttype, "" );
			if(CallNdx != -1)
				ChannelInfo[index].Calls[CallNdx].SState = GCST_ACCEPTED;
			else Log( TRC_GC, TRC_ERROR, index, "CallIndex for CRN not found" );
			ret = gc_AnswerCall( TempCRN, 0, EV_ASYNC );
			LogFunc( index, "gc_AnswerCall()", ret );
			break;

		case GCEV_ANSWERED:
			LogGC( TRC_INFO, index, evttype, "" );
			if(CallNdx != -1)
			{
				ChannelInfo[index].Calls[CallNdx].SState = GCST_CONNECTED;
				switch(Mode)
				{
				case 0:
					Log( TRC_CORE, TRC_INFO, index, "Mode : 0 (Autoresponder). Default fragment will be played" );
					if(!InitPlayFragment( index, defaultFragment ))
					{
						ret = gc_DropCall( TempCRN, GC_NORMAL_CLEARING, EV_ASYNC );
						LogFunc( index, "gc_DropCall()", ret );
					}
					break;
				default:
					Log( TRC_CORE, TRC_ERROR, index, "Misconfiguration: unsupported Mode" );
				}
			}
			else
			{
				Log( TRC_GC, TRC_ERROR, index, "CallIndex for CRN not found" );
				ret = gc_DropCall( TempCRN, GC_NORMAL_CLEARING, EV_ASYNC );
				LogFunc( index, "gc_DropCall()", ret );
			}
			break;

		case GCEV_CONNECTED:
			LogGC( TRC_INFO, index, evttype, "" );
			if(CallNdx != -1)
				ChannelInfo[index].Calls[CallNdx].SState = GCST_CONNECTED;
			else
				Log( TRC_GC, TRC_ERROR, index, "CallIndex for CRN not found" );
			// TODO: Init play file, or do other task
			ret = gc_DropCall( TempCRN, GC_NORMAL_CLEARING, EV_ASYNC );
			break;

		case GCEV_DISCONNECTED:
			ChannelInfo[index].PState = PST_RELEASING;
			if(CallNdx == -1)
			{  // Should not be such case
				LogGC( TRC_ERROR, index, evttype, "CallIndex for CRN not found" );
				ret = gc_DropCall( TempCRN, GC_NORMAL_CLEARING, EV_ASYNC );
				LogFunc( index, "gc_DropCall()", ret );
			}
			else
			{
				switch(ChannelInfo[index].Calls[CallNdx].SState)
				{
				case GCST_CONNECTED: // Network Terminated
					LogGC( TRC_INFO, index, evttype, "Network terminated" );
					ChannelInfo[index].Calls[CallNdx].SState = GCST_DISCONNECTED;
					ret = gc_DropCall( TempCRN, GC_NORMAL_CLEARING, EV_ASYNC );
					LogFunc( index, "gc_DropCall()", ret );
					break;
				case GCST_DIALING:   // Network rejects || Glare
					ChannelInfo[index].Calls[CallNdx].SState = GCST_DISCONNECTED;
					if(CallNdx == 0)
						LogGC( TRC_WARNING, index, evttype, "Network rejected outgoing call. USER BUSY" );
					else
						LogGC( TRC_WARNING, index, evttype, "Network rejected outgoing call. GLARE" );
					ret = gc_DropCall( TempCRN, GC_NORMAL_CLEARING, EV_ASYNC );
					LogFunc( index, "gc_DropCall()", ret );
					break;
				case GCST_DISCONNECTED:  // Simul. discconnect
					LogGC( TRC_WARNING, index, evttype, "Simultaneous disconnect" );
					break;
				default:
					sprintf( str, "Disconnect at unexpected SState %d", ChannelInfo[index].Calls[CallNdx].SState );
					LogGC( TRC_ERROR, index, evttype, str );
					ChannelInfo[index].Calls[CallNdx].SState = GCST_DISCONNECTED;
					ret = gc_DropCall( TempCRN, GC_NORMAL_CLEARING, EV_ASYNC );
					LogFunc( index, "gc_DropCall()", ret );
				}
			}
			break;
		case GCEV_DROPCALL:
			LogGC( TRC_INFO, index, evttype, "" );
			state = ATDX_STATE( ChannelInfo[index].hdVoice );
			if((state == CS_PLAY) || (state == CS_RECD) || (state == CS_GTDIG))
			{
				dx_stopch( ChannelInfo[index].hdVoice, EV_ASYNC );
			}
			if(CallNdx == -1)
			{
				Log( TRC_GC, TRC_ERROR, index, "CallIndex for CRN not found" );
			}
			else
			{
				ChannelInfo[index].Calls[CallNdx].SState = GCST_IDLE;
			}
			ret = gc_ReleaseCallEx( TempCRN, EV_ASYNC );
			LogFunc( index, "gc_ReleaseCallEx()", ret );
			break;
		case GCEV_RELEASECALL:
			stUsed--;
			LogGC( TRC_INFO, index, evttype, "" );
			if(ChannelInfo[index].PState == PST_RELEASING)
			{
				ChannelInfo[index].PState = PST_IDLE;
			}
			//else
			//{
				if(CallNdx == -1)
				{
					Log( TRC_GC, TRC_ERROR, index, "CallIndex for CRN not found" );
				}
				else
				{
					if(ChannelInfo[index].Calls[CallNdx].SState == GCST_IDLE)
					{
						ChannelInfo[index].Calls[CallNdx].SState = GCST_NULL;
						ChannelInfo[index].Calls[CallNdx].crn = 0;
					}
					else
					{
						Log( TRC_GC, TRC_ERROR, index, "got event at unexpected SState" );
					}
				}
			//}
			break;
		case GCEV_RESETLINEDEV:
			LogGC( TRC_WARNING, index, evttype, "" );
			for(i = 0; i < MAX_CALLS; i++)
			{
				ChannelInfo[index].Calls[i].crn = 0;
				ChannelInfo[index].Calls[i].SState = GCST_NULL;
			}
			ret = gc_WaitCall( ChannelInfo[index].hdLine, NULL, NULL, 0, EV_ASYNC );
			LogFunc( index, "gc_WaitCall()", ret );
			break;
		case GCEV_TASKFAIL:
			LogGC( TRC_ERROR, index, evttype, "" );
			ChannelInfo[index].blocked = 0;
			ret = gc_ResultInfo( &metaevent, &gc_error_info );
			LogFunc( index, "gc_ResultInfo()", ret );
			if(ret == GC_SUCCESS)
			{
				LogGC_INFO( index, &gc_error_info );
			}
			ret = gc_ResetLineDev( ChannelInfo[index].hdLine, EV_ASYNC );
			LogFunc( index, "gc_ResetLineDev()", ret );
			break;
		case GCEV_PROCEEDING:
			LogGC( TRC_INFO, index, evttype, "" );
			break;
		case GCEV_ALERTING:
			LogGC( TRC_INFO, index, evttype, "" );
			break;
		default:
			LogGC( TRC_ERROR, index, evttype, "Unsupported event" );
		}
	}
	else
	{
		// Other Event
		// Suppose that here will be dxxx event. But if later another event source be added 
		//   need to clarify is it required to distinguish these sources (and process events in different ways)
		hdDev = metaevent.evtdev;
		evttype = metaevent.evttype;
		index = GetIndexByVoice( hdDev );
		switch(evttype)
		{
		case TDX_PLAY:
			LogDX( TRC_INFO, index, evttype, "" );
			if(index == -1)
			{
				Log( TRC_CORE, TRC_ERROR, -1, "Linedev not found" );
			}
			else
			{
				switch(ChannelInfo[index].PState)
				{
				case PST_PLAY:
					/*
					ret = dx_play( ChannelInfo[index].hdVoice, &ChannelInfo[index].iott, NULL, RM_SR8 | EV_ASYNC );
					LogFunc( index, "dx_play()", ret );
					if(ret != GC_SUCCESS)
					{
						close( ChannelInfo[index].iott.io_fhandle );
						ret = gc_DropCall( ChannelInfo[index].Calls[0].crn, GC_NORMAL_CLEARING, EV_ASYNC );
						LogFunc( index, "gc_DropCall()", ret );
					}
					*/
					InitDisconnect( index );
					break;
				default:
					ret = close( ChannelInfo[index].iott.io_fhandle );
					LogFunc( index, "close()", ret );
					ChannelInfo[index].iott.io_fhandle = -1;
					ChannelInfo[index].VState = VST_IDLE;
					//ChannelInfo[index].PState = PST_IDLE;
						//ret = gc_DropCall(ChannelInfo[index].Calls[0].crn,GC_NORMAL_CLEARING,EV_ASYNC);
					//LogFunc(index, "gc_DropCall()", ret);
				}
			}
			break;
		case TDX_GETDIG:
			LogDX( TRC_INFO, index, evttype, "" );
			if(index == -1)
			{
				Log( TRC_CORE, TRC_ERROR, -1, "Linedev not found" );
			}
			else
			{
				sprintf( str, "GetDigit return [%c]", ChannelInfo[index].digbuf.dg_value[0] );
				Log( TRC_DX, TRC_INFO, index, str );
			}
			break;
		case TDX_CST:
			LogDX( TRC_INFO, index, evttype, "" );
			cstp = (DX_CST *)sr_getevtdatap();
			switch(cstp->cst_event)
			{
			case DE_DIGITS:
				sprintf( str, "Digit detected [%c]", (char)(cstp->cst_data) );
				Log( TRC_DX, TRC_INFO, index, str );
				break;
			}
			break;
		default:
			LogDX( TRC_ERROR, index, evttype, "Unsupported event" );
		}
	}
}
//---------------------------------------------------------------------------
int GetSIPRdPN( GC_PARM_BLK	*paramblkp, std::string & RdPN, std::string & reason, int *reasonCode )
{
	std::unordered_map<std::string, int> sipReasons;
	sipReasons = {
		{"unavailable", 480},
		{"user-busy", 486},
	};
	bool result = false;
	reason = "";
	*reasonCode = 0;
	GC_PARM_DATAP curParm = NULL;
	while(((curParm = gc_util_next_parm( paramblkp, curParm )) != NULL))
	{
		switch(curParm->parm_ID)
		{
		case IPPARM_DIVERSION_URI:
			// read full param value first. It contains a lot of other attributes and can be very long
			// Example is: 
			// <sip:xxxxxxx@xxx.xxx.xxx.xxx;user=phone>;reason=user-busy;counter=1;privacy=off;screen=no
			std::string diversionURI( (char *)curParm->value_buf, (size_t)curParm->value_size );

			// Extract RdPN
			std::string rdpnStartTag( "sip:" );
			std::string rdpnEndTag( ";" );
			size_t beginRdPN = diversionURI.find( rdpnStartTag );
			size_t endRdPN = diversionURI.find( rdpnEndTag, beginRdPN );
			if((beginRdPN != std::string::npos) && (endRdPN != std::string::npos))
			{
				RdPN = diversionURI.substr( beginRdPN + rdpnStartTag.size(), endRdPN - beginRdPN - rdpnStartTag.size() );
				result = true;
			}

			// Extract Diversion reason
			std::string reasonStartTag( "reason=" );
			std::string reasonEndTag( ";" );
			size_t beginReason = diversionURI.find( reasonStartTag );
			size_t endReason = diversionURI.find( reasonEndTag, beginReason );
			if((beginReason != std::string::npos) && (endReason != std::string::npos))
			{
				reason = diversionURI.substr( beginReason + reasonStartTag.size(), endReason - beginReason - reasonStartTag.size() );
				// Convert string reason into numeric
				auto iter = sipReasons.find( reason );
				if(iter != sipReasons.end())
				{
					*reasonCode = iter->second;
				}
				else
				{
					*reasonCode = 0;
				}
			}
			break;
		}
	}
	gc_util_delete_parm_blk( paramblkp );
	return result;
}
//---------------------------------------------------------------------------
void RegNewCall( int LineNo, CRN crn, int State )
{
	int i;
	// Вытолкнуть имеющиеся вызовы на позицию вглубь
	for(i = MAX_CALLS - 1; i > 0; i--)
	{
		ChannelInfo[LineNo].Calls[i].crn = ChannelInfo[LineNo].Calls[i - 1].crn;
		ChannelInfo[LineNo].Calls[i].SState = ChannelInfo[LineNo].Calls[i - 1].SState;
	}
	// Новый вызов записать в нулевую позицию
	ChannelInfo[LineNo].Calls[0].crn = crn;
	ChannelInfo[LineNo].Calls[0].SState = State;
}
//---------------------------------------------------------------------------
int GetCallNdx( int LineNo, CRN crn )
{
	int i;
	for(i = 0; i < MAX_CALLS; i++)
		if(ChannelInfo[LineNo].Calls[i].crn == crn) return i;
	return -1;
}
//---------------------------------------------------------------------------
int GetIndexByVoice( int dev )
{
	int i;
	for(i = 0; i < TotalChannels; i++)
		if(ChannelInfo[i].hdVoice == dev)
			return i;
	return -1;
}
//---------------------------------------------------------------------------------
bool InitPlayFragment( int index, const char *filename )
{
	Log( TRC_CORE, TRC_INFO, index, "InitPlayFragment() : play '%s'", filename );
	int ret;
	if ((ChannelInfo[index].iott.io_fhandle = open( filename, O_RDONLY )) != -1)
	{
		ChannelInfo[index].iott.io_type = IO_EOT | IO_DEV;
		ChannelInfo[index].iott.io_offset = (unsigned long)0;
		ChannelInfo[index].iott.io_length = -1;
		ChannelInfo[index].xpb.wFileFormat = FILE_FORMAT_WAVE;
		ret = dx_playiottdata( ChannelInfo[index].hdVoice, &(ChannelInfo[index].iott), NULL, &(ChannelInfo[index].xpb), EV_ASYNC );
		LogFunc( index, "dx_playiottdata()", ret );
		if(ret != GC_SUCCESS)
		{
			close( ChannelInfo[index].iott.io_fhandle );
			return false;
		}
		else
		{
			ChannelInfo[index].PState = PST_PLAY;
			ChannelInfo[index].VState = VST_PLAY;
			return true;
		}
	}
	else
	{
		Log( TRC_DX, TRC_ERROR, index, "Play Error: unable open file" );
		return false;
	}
}
//---------------------------------------------------------------------------
void InitDisconnect( int N, int reason )
{
	int ret;
	ChannelInfo[N].PState = PST_RELEASING;
	ret = gc_DropCall( ChannelInfo[N].Calls[0].crn, reason, EV_ASYNC );
	LogFunc( N, "gc_DropCall()", ret );
}
//---------------------------------------------------------------------------------
void InitNewCall( int N )
{
	int ret;
	ret = gc_MakeCall( ChannelInfo[N].hdLine, &(ChannelInfo[N].Calls[0].crn), NULL, &(ChannelInfo[N].makecallblk), 15, EV_ASYNC );
	LogFunc( N, "gc_MakeCall()", ret );
	if(ret == GC_SUCCESS)
	{
		ChannelInfo[N].Calls[0].SState = GCST_DIALING;
		ChannelInfo[N].PState = PST_CALLING;
	}
	else
	{
		ChannelInfo[N].Calls[0].SState = GCST_NULL;
		ChannelInfo[N].PState = PST_IDLE;
	}
}
//---------------------------------------------------------------------------------
void writeStatistics()
{
	struct tm *tblock;
	tblock = localtime( &start_time );

	fprintf( fStat, "%04d/%02d/%02d %02d:%02d:%02d  %4d %4d %4d\n",
		tblock->tm_year + 1900, tblock->tm_mon, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec,
		TotalChannels, stUnblocked, stUsed );
	fflush( fStat );
}
//---------------------------------------------------------------------------------
