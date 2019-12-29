/* OS Header Files */
#include <stdio.h>
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
#else
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

#include <algorithm>

#include "callflow.hpp"


static int		interrupted = NO;	/* Flag for user interrupted signals		 */
time_t			start_time;		// To note down the start time

int main( void )
{
	int ret;

	/* Note down the start time for the test */
	time( &start_time );
	Log( TRC_CORE, -1, "********** CRBT service started ***********", 0 );

	// Give control to Event Handler
/*
#ifdef _WIN32
	signal(SIGINT, (void (__cdecl*)(int)) intr_hdlr);
	signal(SIGTERM, (void (__cdecl*)(int)) intr_hdlr);
#else
	signal(SIGHUP, (void (*)()) intr_hdlr);
	signal(SIGQUIT, (void (*)()) intr_hdlr);
	signal(SIGINT, (void (*)()) intr_hdlr);
	signal(SIGTERM, (void (*)()) intr_hdlr);
#endif
*/
	init_srl_mode();									/* set SRL mode to ASYNC, polled */
	LoadSettings();
	InitDialogicLibs();
	InitChannels();

	// Forever loop where the main work is done - wait for an event or user requested exit
	Log( TRC_CORE, -1, "Entering Working State", 0 );
	for(;;) {
		ret = sr_waitevt( 500 );					// 1/2 second
		if(ret != -1) {					// i.e. not timeout
			process_event();
		}
		if(interrupted == YES) {				// flag set in intr_hdlr() 
			//
			// Check here if all channels already cleared and closed
			// And if so - can exit
			//
			return 0;
		}

	}
}

/****************************************************************
*			NAME: void init_srl_mode(void)
* DESCRIPTION: Inits SR mode
*		RETURNS: None
*	  CAUTIONS: NA
****************************************************************/
void init_srl_mode() {
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
	if(sr_setparm( SRL_DEVICE, SR_MODELTYPE, &mode ) == -1) {
		Log( TRC_CORE, -1, "Unable to set to SR_STASYNC | SR_POLLMODE", 4 );
		exit( 1 );
	}
	Log( TRC_CORE, -1, "SRL Model Set to SR_STASYNC | SR_POLLMODE", 0 );
#else
	if(sr_setparm( SRL_DEVICE, SR_MODEID, &mode ) == -1) {
		Log( TRC_CORE, -1, "Unable to set mode ID to SR_POLLMODE ", 4 );
		exit( 1 );
	}

	Log( TRC_CORE, -1, "SRL mode ID set to SR_POLLMODE", 0 );
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
static void intr_hdlr( void )
{
	Log( TRC_CORE, -1, " *******Received User Interrupted Signal *******", 2 );
	Deinit();
	interrupted = YES;
}
//---------------------------------------------------------------------------
void InitDialogicLibs() {
	Log( TRC_CORE, -1, "Init dialogic Libs", 0 );
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
	if(TotalChannels > 120) {
		virtBoard.total_max_calls = TotalChannels;
		virtBoard.h323_max_calls = 0;
		virtBoard.sip_max_calls = TotalChannels;
	}
	if(LocalIP != -1) {
		virtBoard.localIP.ip_ver = IPVER4;
		virtBoard.localIP.u_ipaddr.ipv4 = (unsigned int)LocalIP;//htonl(0x0ade0966);
	}
	virtBoard.sip_msginfo_mask = IP_SIP_MSGINFO_ENABLE;
	INIT_IPCCLIB_START_DATA( &cclibData, 1, &virtBoard );

	gcLibStart.cclib_list = ccLibStart;
	gcLibStart.num_cclibs = 4;

	/* Start GlobalCall */
	if(gc_Start( &gcLibStart ) != GC_SUCCESS) {
		sprintf( str, "gc_Start(startp = NULL) Failed" );
		Log( TRC_CORE, -1, str, 4 );
		exit( 1 );
	}

	// Default style of gc_Start
	/*
	if (gc_Start(NULL) != GC_SUCCESS) {
		sprintf(str, "gc_Start(startp = NULL) Failed");
		Log(TRC_CORE, -1, str, 4);
		exit(1);
	}
	*/
	if(gc_CCLibStatusEx( (char *)"GC_ALL_LIB", &cclib_status_all ) != GC_SUCCESS) {
		sprintf( str, "gc_CCLibStatusEx(GC_ALL_LIB, &cclib_status_all) Failed" );
		Log( TRC_CORE, -1, str, 4 );
		exit( 1 );
	}
	strcpy( str, " Call Control Library Status:" );
	Log( TRC_CORE, -1, str, 0 );
	for(i = 0; i < GC_TOTAL_CCLIBS; i++) {
		switch(cclib_status_all.cclib_state[i].state) {
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
		Log( TRC_CORE, -1, str, 0 );
	}
	strcpy( str, " ----------------------------" );
	Log( TRC_CORE, -1, str, 0 );
}
//---------------------------------------------------------------------------
void Deinit() {
	Log( TRC_CORE, -1, "Deinitialization", 0 );
	int i;
	for(i = 0; i < TotalChannels; i++) {
		if(gc_Close( ChannelInfo[i].hdLine ) != GC_SUCCESS)
			Log( TRC_CORE, i, "gc_Close() error", 2 );
	}
}
//---------------------------------------------------------------------------
void Log( int Src, int Line, const char *msg, int Svrt ) {
	if((TraceMask & Src) == 0) return;
	if(Svrt < SeverityFilter)  return;

	char str[LOGSTRSIZE];
	char sSrc[16];
	switch(Src) {
	case TRC_CORE: strcpy( sSrc, "CORE" ); break;
	case TRC_GC:   strcpy( sSrc, "GC  " ); break;
	case TRC_DX:   strcpy( sSrc, "DX  " ); break;
	case TRC_SETT: strcpy( sSrc, "SETT" ); break;
	default: sprintf( sSrc, "[  %02d]", Src );
	}
	if(Line >= 0)
		sprintf( str, "[%s] [%04d] [%d] %s", sSrc, Line, Svrt, msg );
	else
		sprintf( str, "[%s] [    ] [%d] %s", sSrc, Svrt, msg );
	LogWrite( str );
}
//---------------------------------------------------------------------------
void LogGC( int Line, int event, const char *msg, int Svrt ) {
	if((TraceMask & TRC_GC) == 0) return;
	if(Svrt < SeverityFilter)  return;
	char str[LOGSTRSIZE];
	char sEvt[GCEVENTNAMESIZE];
	strcpy( sEvt, GcEventNames[event - DT_GC] );
	if(sEvt[0] == 0) sprintf( sEvt, "0x%04x", event );
	if(Line >= 0)
		sprintf( str, "[GC  ] [%04d] [%d] %s %s", Line, Svrt, sEvt, msg );
	else
		sprintf( str, "[GC  ] [    ] [%d] %s %s", Svrt, sEvt, msg );
	LogWrite( str );
}
//---------------------------------------------------------------------------
void LogDX( int Line, int event, const char *msg, int Svrt ) {
	if((TraceMask & TRC_DX) == 0) return;
	if(Svrt < SeverityFilter)  return;
	char str[LOGSTRSIZE];
	char sEvt[GCEVENTNAMESIZE];
	strcpy( sEvt, DxEventNames[event - 0x81] );
	if(sEvt[0] == 0) sprintf( sEvt, "0x%04x", event );
	if(Line >= 0)
		sprintf( str, "[DX  ] [%04d] [%d] %s %s", Line, Svrt, sEvt, msg );
	else
		sprintf( str, "[DX  ] [    ] [%d] %s %s", Svrt, sEvt, msg );
	LogWrite( str );
}
//---------------------------------------------------------------------------
void LogFunc( int Channel, const char *FuncName, int ret ) {
	char str[LOGSTRSIZE];
	sprintf( str, "%s : %s", FuncName, (ret == GC_SUCCESS) ? "Ok" : "Error" );
	Log( TRC_CORE, Channel, str, (ret == GC_SUCCESS) ? 0 : 2 );
}
//---------------------------------------------------------------------------
void LogWrite( const char *msg ) {
	time_t timer = time( NULL );
	struct tm *tblock;
	tblock = localtime( &timer );
	printf( "%04d/%02d/%02d %02d:%02d:%02d %s\n", tblock->tm_year + 1900, tblock->tm_mon, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec, msg );
	//  printf("%s\n",msg);
}
//---------------------------------------------------------------------------
int FindParam( char *ParamName ) {
	int i;
	for(i = 0; i < MAXPARAMS; i++)
		if(strcmp( ParamName, Parameters[i].Name ) == 0)
			return Parameters[i].ID;
	return -1;
}
//---------------------------------------------------------------------------
void LoadSettings() {
	Log( TRC_SETT, -1, "Load Settings", 0 );
	TotalChannels = DEFAULT_TOTALCHANNELS;
	TraceMask = DEFAULT_TRACEMASK;
	SeverityFilter = DEFAULT_SEVERITYFILTER;

	char logstr[LOGSTRSIZE];
	char str[MAXPARAMSIZE];
	char ParamName[MAXPARAMSIZE];
	char ParamValue[MAXPARAMSIZE];
	char sParam[MAXPARAMSIZE];
	int sl;
	int n, np, nv;
	FILE *f = fopen( "params.ini", "r" );
	if(f == NULL) {
		Log( TRC_SETT, -1, "Cannot find 'params.ini'", 4 );
		exit( 1 );
	}
	while(!feof( f )) {
		if(fgets( str, MAXPARAMSIZE, f ) == NULL) break;
		sl = strlen( str );
		if(sl < 3) continue;
		n = 0;
		np = 0;
		while((str[n] != '=') && (n < sl)) {
			ParamName[np++] = str[n++];
			ParamName[np] = 0;
		}
		if(n >= sl) continue; // String does not have part '=Value'
		n++;  // Skip symbol '='
		nv = 0;
		while((n < sl) && (str[n] != 10) && (str[n] != 13)) {
			ParamValue[nv++] = str[n++];
			ParamValue[nv] = 0;
		}
		if((strlen( ParamName ) < 1) || (strlen( ParamValue ) < 1)) continue;
		// At this point we have ParanName/ParamValue pair
		int PN = FindParam( ParamName );
		if(PN == -1) {
			sprintf( logstr, "Unknown parameter definition: %s=%s", ParamName, ParamValue );
			Log( TRC_SETT, -1, logstr, 1 );
		}
		else {
			switch(PN) {
			case PRM_CHANNELSCNT:
				if(sscanf( ParamValue, "%d", &TotalChannels ) == 1) {
					sprintf( logstr, "Set TotalChannels = %d", TotalChannels );
					Log( TRC_SETT, -1, logstr, 0 );
				}
				else {
					TotalChannels = DEFAULT_TOTALCHANNELS;
					sprintf( logstr, "Wrong parameter set: %s=%s", ParamName, ParamValue );
					Log( TRC_SETT, -1, logstr, 2 );
				}
				break;
			case PRM_TRACEMASK:
				if(sscanf( ParamValue, "%d", &TraceMask ) == 1) {
					sprintf( logstr, "Set TraceMask = %d", TraceMask );
					Log( TRC_SETT, -1, logstr, 0 );
				}
				else {
					TraceMask = DEFAULT_TRACEMASK;
					sprintf( logstr, "Wrong parameter set: %s=%s", ParamName, ParamValue );
					Log( TRC_SETT, -1, logstr, 2 );
				}
				break;
			case PRM_SVRTFILTER:
				if(sscanf( ParamValue, "%d", &SeverityFilter ) == 1) {
					sprintf( logstr, "Set SeverityFilter = %d", SeverityFilter );
					Log( TRC_SETT, -1, logstr, 0 );
				}
				else {
					SeverityFilter = DEFAULT_SEVERITYFILTER;
					sprintf( logstr, "Wrong parameter set: %s=%s", ParamName, ParamValue );
					Log( TRC_SETT, -1, logstr, 2 );
				}
				break;
			case PRM_SENDCALLACK:
				if(sscanf( ParamValue, "%d", &SendCallAck ) == 1) {
					sprintf( logstr, "Set SendCallAck = %d", SendCallAck );
					Log( TRC_SETT, -1, logstr, 0 );
				}
				else {
					SendCallAck = DEFAULT_SENDCALLACK;
					sprintf( logstr, "Wrong parameter set: %s=%s", ParamName, ParamValue );
					Log( TRC_SETT, -1, logstr, 2 );
				}
				break;
			case PRM_SENDACM:
				if(sscanf( ParamValue, "%d", &SendACM ) == 1) {
					sprintf( logstr, "Set SendACM = %d", SendACM );
					Log( TRC_SETT, -1, logstr, 0 );
				}
				else {
					SendCallAck = DEFAULT_SENDACM;
					sprintf( logstr, "Wrong parameter set: %s=%s", ParamName, ParamValue );
					Log( TRC_SETT, -1, logstr, 2 );
				}
				break;
			case PRM_LOCALIP:
				if(sscanf( ParamValue, "%s", sParam ) == 1) {
					LocalIP = inet_addr( sParam );
					if(LocalIP != -1) {
						sprintf( logstr, "LocalIP = %s", sParam );
						Log( TRC_SETT, -1, logstr, 0 );
					}
					else {
						sprintf( logstr, "Wrong LocalIP settings : [%s]", sParam );
						Log( TRC_SETT, -1, logstr, 0 );
					}
				}
				else {
					LocalIP = -1;
					sprintf( logstr, "Wrong parameter set: %s=%s", ParamName, ParamValue );
					Log( TRC_SETT, -1, logstr, 2 );
				}
				break;
			default:
				sprintf( logstr, "Unimplemented initialization: %s=%s", ParamName, ParamValue );
				Log( TRC_SETT, -1, logstr, 2 );
			}
		}
	}
	fclose( f );
}
//---------------------------------------------------------------------------
void InitChannels() {
	LogWrite( "Init Channels" );
	int i, callndx;
	int ret;
	char str[LOGSTRSIZE];
	char FullDevName[MAX_DEVNAME];

	for(i = 0; i < TotalChannels; i++) {
		ChannelInfo[i].blocked = 1;
		for(callndx = 0; callndx < MAX_CALLS; callndx++) {
			ChannelInfo[i].Calls[callndx].crn = 0;
			ChannelInfo[i].Calls[callndx].SState = GCST_NULL;
		}
		sprintf( FullDevName, ":P_SIP:N_iptB1T%d:M_ipmB1C%d:V_dxxxB%dC%d", i + 1, i + 1, (i / 4) + 1, (i % 4) + 1 );
		//sprintf(str, "[%s] is about to open", FullDevName);
		//Log(str);
		ret = gc_OpenEx( &(ChannelInfo[i].hdLine), FullDevName, EV_ASYNC, (void *)i );
		sprintf( str, "gc_OpenEx(\"%s\") = %d", FullDevName, ret );
		Log( TRC_CORE, i, str, 2 );
		if(ret == GC_SUCCESS) {
			ChannelInfo[i].iott.io_fhandle = -1;
		}
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
	GC_PARM_DATAP curParm = NULL;

	long		state;

	if(gc_GetMetaEvent( &metaevent ) != GC_SUCCESS) {
		// serious problem - should never fail
		Log( TRC_CORE, -1, "gc_GetMetaEvent() failed", 3 );
		return;
	}

	if((metaevent.flags & GCME_GC_EVENT) != 0) {
		// Global Call Event
		index = (int)metaevent.usrattr;
		evttype = metaevent.evttype;
		TempCRN = metaevent.crn;
		CallNdx = GetCallNdx( index, TempCRN );
		switch(evttype) {
		case GCEV_OPENEX:
			LogGC( index, evttype, "Channel is opened", 0 );
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
			break;
		case GCEV_UNBLOCKED:
			LogGC( index, evttype, "Channel is unblocked", 0 );
			ChannelInfo[index].blocked = 0;
			ret = gc_WaitCall( ChannelInfo[index].hdLine, NULL, NULL, 0, EV_ASYNC );
			LogFunc( index, "gc_WaitCall()", ret );
			break;
		case GCEV_OPENEX_FAIL:
			LogGC( index, evttype, "Failed to open channel", 2 );
			break;
		case GCEV_OFFERED:
			ChannelInfo[index].PState = PST_IDLE;
			RegNewCall( index, TempCRN, GCST_OFFERED );
			if(gc_GetCallInfo( TempCRN, DESTINATION_ADDRESS, st ) != -1)
				strncpy( ChannelInfo[index].CdPN, st, MAX_NUMSIZE ); else ChannelInfo[index].CdPN[0] = 0;
			if(gc_GetCallInfo( TempCRN, ORIGINATION_ADDRESS, st ) != -1)
				strncpy( ChannelInfo[index].CgPN, st, MAX_NUMSIZE ); else ChannelInfo[index].CgPN[0] = 0;
			ChannelInfo[index].RdPN[0] = 0;


			parmblkp = (GC_PARM_BLKP)metaevent.extevtdatap;

			// Get Diversion number (copy from CCAF)
			while(((curParm = gc_util_next_parm( parmblkp, curParm )) != NULL))
			{
				//printf("Extracting next param: code 0x%x\n", curParm->parm_ID);
				switch(curParm->parm_ID)
				{
				case IPPARM_DIVERSION_URI:
				{
					//printf("IPPARM_DIVERSION_URI found\n");
					char szCnum[GC_ADDRSIZE];
					memcpy( szCnum, curParm->value_buf, curParm->value_size );
					szCnum[curParm->value_size] = 0;
					//printf("Diversion URI: [%s]\n", szCnum);

					int firstPos = std::find( szCnum, szCnum + curParm->value_size, ':' ) - szCnum;
					int lastPos = std::find( szCnum + firstPos + 1, szCnum + curParm->value_size, ';' ) - szCnum;
					if((firstPos < curParm->value_size) && (lastPos < curParm->value_size))
					{
						szCnum[lastPos] = 0;
						//printf("CNum: [%s]\n", szCnum + firstPos + 1);
						strncpy( ChannelInfo[index].RdPN, szCnum + firstPos + 1, MAX_NUMSIZE );
					}

					break;
				}
				}
			}
			gc_util_delete_parm_blk( parmblkp );


			sprintf( str, "CgPN:[%s] CdPN:[%s] RdPN:[%s]", ChannelInfo[index].CgPN, ChannelInfo[index].CdPN, ChannelInfo[index].RdPN );
			LogGC( index, evttype, str, 0 );
			if(SendCallAck > 0) {
				GC_CALLACK_BLK callack; /* type & number of digits to collect */
				memset( &callack, 0, sizeof( callack ) );
				if(gc_CallAck( TempCRN, &callack, EV_ASYNC ) != GC_SUCCESS) {
					gc_ErrorInfo( &gc_error_info );
					sprintf( str, "Error gc_CallAck() %s", gc_error_info.ccMsg );
					Log( TRC_CORE, index, str, 2 );
				}
				else {
					LogFunc( index, "gc_CallAck", 0 );
				}
			}
			else {
				if(SendACM > 0) {
					ret = gc_AcceptCall( TempCRN, 0, EV_ASYNC );
					LogFunc( index, "gc_AcceptCall()", ret );
				}
				else {
					ret = gc_AnswerCall( TempCRN, 0, EV_ASYNC );
					LogFunc( index, "gc_AnswerCall()", ret );
				}
			}
			break;
		case GCEV_CALLPROC:
			LogGC( index, evttype, "", 0 );
			if(SendACM > 0) {
				ret = gc_AcceptCall( TempCRN, 0, EV_ASYNC );
				LogFunc( index, "gc_AcceptCall()", ret );
			}
			else {
				ret = gc_AnswerCall( TempCRN, 0, EV_ASYNC );
				LogFunc( index, "gc_AnswerCall()", ret );
			}
			break;
		case GCEV_ACCEPT:
			LogGC( index, evttype, "", 0 );
			if(CallNdx != -1)
				ChannelInfo[index].Calls[CallNdx].SState = GCST_ACCEPTED;
			else Log( TRC_GC, index, "CallIndex for CRN not found", 3 );
			ret = gc_AnswerCall( TempCRN, 0, EV_ASYNC );
			LogFunc( index, "gc_AnswerCall()", ret );
			break;
		case GCEV_ANSWERED:
			LogGC( index, evttype, "", 0 );
			if(CallNdx != -1)
				ChannelInfo[index].Calls[CallNdx].SState = GCST_CONNECTED;
			else Log( TRC_GC, index, "CallIndex for CRN not found", 3 );
			// Drop Call
			/*
							ret = gc_DropCall(TempCRN,GC_NORMAL_CLEARING,EV_ASYNC);
			LogFunc(index, "gc_DropCall()", ret);
			*/
			// End Drop Call
			// VOX PLAY
			/*
			if ((ChannelInfo[index].iott.io_fhandle = open("broadcast.vox",O_RDONLY)) != -1) {
				ChannelInfo[index].iott.io_type=IO_EOT|IO_DEV;
				ChannelInfo[index].iott.io_offset=(unsigned long)0;
				ChannelInfo[index].iott.io_length=-1;
				ret = dx_play(ChannelInfo[index].hdVoice, &ChannelInfo[index].iott, NULL, RM_SR8|EV_ASYNC);
				LogFunc(index, "dx_play()", ret);
				if (ret != GC_SUCCESS) {
					close(ChannelInfo[index].iott.io_fhandle);
									ChannelInfo[index].PState = PST_TERMINATION;
									ret = gc_DropCall(TempCRN,GC_NORMAL_CLEARING,EV_ASYNC);
					LogFunc(index, "gc_DropCall()", ret);
				} else {
					ChannelInfo[index].PState = PST_PLAY;
					ChannelInfo[index].VState = VST_PLAY;
				}
			} else {
			  Log(TRC_DX, index, "Play Error: unable open file", 2);
							  ChannelInfo[index].PState = PST_TERMINATION;
							  ret = gc_DropCall(TempCRN,GC_NORMAL_CLEARING,EV_ASYNC);
			  LogFunc(index, "gc_DropCall()", ret);
			}

			// End VOX Play
			*/
			// WAV PLAY

			if((ChannelInfo[index].iott.io_fhandle = open( "9999.wav", O_RDONLY )) != -1) {
				ChannelInfo[index].iott.io_type = IO_EOT | IO_DEV;
				ChannelInfo[index].iott.io_offset = (unsigned long)0;
				ChannelInfo[index].iott.io_length = -1;
				ChannelInfo[index].xpb.wFileFormat = FILE_FORMAT_WAVE;
				ret = dx_playiottdata( ChannelInfo[index].hdVoice, &ChannelInfo[index].iott, NULL, &ChannelInfo[index].xpb, EV_ASYNC );
				LogFunc( index, "dx_playiottdata()", ret );
				if(ret != GC_SUCCESS) {
					close( ChannelInfo[index].iott.io_fhandle );
					ChannelInfo[index].PState = PST_TERMINATION;
					ret = gc_DropCall( TempCRN, GC_NORMAL_CLEARING, EV_ASYNC );
					LogFunc( index, "gc_DropCall()", ret );
				}
				else {
					ChannelInfo[index].PState = PST_PLAY;
					ChannelInfo[index].VState = VST_PLAY;
				}
			}
			else {
				Log( TRC_DX, index, "Play Error: unable open file", 2 );
				ChannelInfo[index].PState = PST_TERMINATION;
				//ret = gc_DropCall(TempCRN,GC_NORMAL_CLEARING,EV_ASYNC);
				ret = gc_DropCall( TempCRN, GC_NORMAL_CLEARING, EV_ASYNC );
				LogFunc( index, "gc_DropCall()", ret );
			}

			// End WAV Play
			// Get Digit
			/*
							ChannelInfo[index].tpt[0].tp_type   = IO_CONT;
							ChannelInfo[index].tpt[0].tp_termno = DX_MAXDTMF;
							ChannelInfo[index].tpt[0].tp_length = 1;
							ChannelInfo[index].tpt[0].tp_flags  = TF_MAXDTMF;
							ChannelInfo[index].tpt[1].tp_type = IO_EOT;
							ChannelInfo[index].tpt[1].tp_termno = DX_MAXTIME;
							ChannelInfo[index].tpt[1].tp_length = 50;
							ChannelInfo[index].tpt[1].tp_flags = TF_MAXTIME;
				Log(TRC_DX, index, "Digit collection. To stop press '#' Timeout = 5 sec", 0);
			dx_clrdigbuf(ChannelInfo[index].hdVoice);
							ret = dx_getdig(ChannelInfo[index].hdVoice, ChannelInfo[index].tpt, &(ChannelInfo[index].digbuf), EV_ASYNC);
			LogFunc(index, "dx_getdig()", ret);
			*/
			// End Get Digit
			break;
		case GCEV_DISCONNECTED:
			ChannelInfo[index].PState = PST_TERMINATION;
			if(CallNdx == -1) {  // Should not be such case
				LogGC( index, evttype, "CallIndex for CRN not found", 3 );
				ret = gc_DropCall( TempCRN, GC_NORMAL_CLEARING, EV_ASYNC );
				LogFunc( index, "gc_DropCall()", ret );
			}
			else {
				switch(ChannelInfo[index].Calls[CallNdx].SState) {
				case GCST_CONNECTED: // Network Terminated
					LogGC( index, evttype, "Network terminated", 0 );
					ChannelInfo[index].Calls[CallNdx].SState = GCST_DISCONNECTED;
					ret = gc_DropCall( TempCRN, GC_NORMAL_CLEARING, EV_ASYNC );
					LogFunc( index, "gc_DropCall()", ret );
					break;
				case GCST_DIALING:   // Network rejects || Glare
					ChannelInfo[index].Calls[CallNdx].SState = GCST_DISCONNECTED;
					if(CallNdx == 0)
						LogGC( index, evttype, "Network rejected outgoing call. USER BUSY", 0 );
					else
						LogGC( index, evttype, "Network rejected outgoing call. GLARE", 0 );
					ret = gc_DropCall( TempCRN, GC_NORMAL_CLEARING, EV_ASYNC );
					LogFunc( index, "gc_DropCall()", ret );
					break;
				case GCST_DISCONNECTED:  // Simul. discconnect
					LogGC( index, evttype, "Simultaneous disconnect", 0 );
					break;
				default:
					LogGC( index, evttype, "Disconnect at unexpected SState", 1 );
					ChannelInfo[index].Calls[CallNdx].SState = GCST_DISCONNECTED;
					ret = gc_DropCall( TempCRN, GC_NORMAL_CLEARING, EV_ASYNC );
					LogFunc( index, "gc_DropCall()", ret );
				}
			}
			break;
		case GCEV_DROPCALL:
			LogGC( index, evttype, "", 0 );
			state = ATDX_STATE( ChannelInfo[index].hdVoice );
			if((state == CS_PLAY) || (state == CS_RECD) || (state == CS_GTDIG)) {
				dx_stopch( ChannelInfo[index].hdVoice, EV_ASYNC );
			}
			if(CallNdx == -1) {
				Log( TRC_GC, index, "CallIndex for CRN not found", 3 );
			}
			else {
				ChannelInfo[index].Calls[CallNdx].SState = GCST_IDLE;
			}
			ret = gc_ReleaseCallEx( TempCRN, EV_ASYNC );
			LogFunc( index, "gc_ReleaseCallEx()", ret );
			break;
		case GCEV_RELEASECALL:
			LogGC( index, evttype, "", 0 );
			if(CallNdx == -1) {
				Log( TRC_GC, index, "CallIndex for CRN not found", 3 );
			}
			else {
				if(ChannelInfo[index].Calls[CallNdx].SState == GCST_IDLE) {
					ChannelInfo[index].Calls[CallNdx].SState = GCST_NULL;
					ChannelInfo[index].Calls[CallNdx].crn = 0;
				}
				else {
					Log( TRC_GC, index, "got event at unexpected SState", 3 );
				}
			}
			break;
		case GCEV_RESETLINEDEV:
			LogGC( index, evttype, "", 0 );
			for(i = 0; i < MAX_CALLS; i++) {
				ChannelInfo[index].Calls[i].crn = 0;
				ChannelInfo[index].Calls[i].SState = GCST_NULL;
			}
			ret = gc_WaitCall( ChannelInfo[index].hdLine, NULL, NULL, 0, EV_ASYNC );
			LogFunc( index, "gc_WaitCall()", ret );
			break;
		case GCEV_TASKFAIL:
			LogGC( index, evttype, "", 0 );
			ChannelInfo[index].blocked = 0;
			ret = gc_ResetLineDev( ChannelInfo[index].hdLine, EV_ASYNC );
			LogFunc( index, "gc_ResetLineDev()", ret );
			break;
		default:
			LogGC( index, evttype, "Unsupported event", 2 );
		}
	}
	else {
		// Other Event
		// Suppose that here will be dxxx event. But if later another event source be added 
		//   need to clarify is it required to distinguish these sources (and process events in different ways)
		hdDev = metaevent.evtdev;
		evttype = metaevent.evttype;
		index = GetIndexByVoice( hdDev );
		switch(evttype) {
		case TDX_PLAY:
			LogDX( index, evttype, "", 0 );
			if(index == -1) {
				Log( TRC_CORE, -1, "Linedev not found", 3 );
			}
			else {
				switch(ChannelInfo[index].PState) {
				case PST_PLAY:
					ret = dx_play( ChannelInfo[index].hdVoice, &ChannelInfo[index].iott, NULL, RM_SR8 | EV_ASYNC );
					LogFunc( index, "dx_play()", ret );
					if(ret != GC_SUCCESS) {
						close( ChannelInfo[index].iott.io_fhandle );
						ret = gc_DropCall( ChannelInfo[index].Calls[0].crn, GC_NORMAL_CLEARING, EV_ASYNC );
						LogFunc( index, "gc_DropCall()", ret );
					}
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
			LogDX( index, evttype, "", 0 );
			if(index == -1) {
				Log( TRC_CORE, -1, "Linedev not found", 3 );
			}
			else {
				sprintf( str, "GetDigit return [%c]", ChannelInfo[index].digbuf.dg_value[0] );
				Log( TRC_DX, index, str, 0 );
			}
			break;
		case TDX_CST:
			LogDX( index, evttype, "", 0 );
			cstp = (DX_CST *)sr_getevtdatap();
			switch(cstp->cst_event) {
			case DE_DIGITS:
				sprintf( str, "Digit detected [%c]", (char)(cstp->cst_data) );
				Log( TRC_DX, index, str, 0 );
				break;
			}
			break;
		default:
			LogDX( index, evttype, "Unsupported event", 2 );
		}
	}
}
//---------------------------------------------------------------------------
void RegNewCall( int LineNo, CRN crn, int State ) {
	int i;
	// Вытолкнуть имеющиеся вызовы на позицию вглубь
	for(i = MAX_CALLS - 1; i > 0; i--) {
		ChannelInfo[LineNo].Calls[i].crn = ChannelInfo[LineNo].Calls[i - 1].crn;
		ChannelInfo[LineNo].Calls[i].SState = ChannelInfo[LineNo].Calls[i - 1].SState;
	}
	// Новый вызов записать в нулевую позицию
	ChannelInfo[LineNo].Calls[0].crn = crn;
	ChannelInfo[LineNo].Calls[0].SState = State;
}
//---------------------------------------------------------------------------
int GetCallNdx( int LineNo, CRN crn ) {
	int i;
	for(i = 0; i < MAX_CALLS; i++)
		if(ChannelInfo[LineNo].Calls[i].crn == crn) return i;
	return -1;
}
//---------------------------------------------------------------------------
int GetIndexByVoice( int dev ) {
	int i;
	for(i = 0; i < TotalChannels; i++)
		if(ChannelInfo[i].hdVoice == dev)
			return i;
	return -1;
}
//---------------------------------------------------------------------------
