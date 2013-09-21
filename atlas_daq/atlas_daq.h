/*

	Atlas Project - Data Acquisition Daemon (DAQ)
	J. Hipps - http://jhipps.org/ (jhipps@nichiha.com)
	Nichiha USA, Inc.

	Global Header
	Copyright (c) 2012-2013 Jacob Hipps/Nichiha USA, Inc., all rights reserved.

*/

#include <mysql.h>
#include "../tuxeip/tuxeip/src/TuxEip.h"

// Compile-time Defines /////////////////////////////////////////////
#define ATLASDAQ_VERSION "0.04"

// Constants & Enum Defines
#define ATLS_DBG_FUNC_PROFILE		// log function runtimes to zlog_debug()

// AB target mapping matches the TuxEIP mapping
#define TARGET_NONE	0	// Unknown/Invalid/None
#define TARGET_LGX	3	// Lgx5k
#define TARGET_SLC	2	// SLC500 E/IP
#define TARGET_PLC	1	// PLC5 E/IP
#define TARGET_MC	4	// Mitsubishi - MC Protocol, Type 3E, Binary
#define TARGET_KEY	5	// Keyence E/IP

// Target flags
#define TFLAG_CSESSION	1 	// Share a connection session with another target

// Status
#define STATUS_NOTREADY	0	// Not Ready/Uninitialized
#define STATUS_READY	1	// Ready/Connected
#define STATUS_COMFAIL	2	// Connnection/Communications failed/error'd
#define STATUS_INITFAIL 3	// Initialization failed
#define STATUS_SHUTDOWN 254	// Shutdown
#define STATUS_DISABLED 255	// Disabled

// EIP data types
#define EIP_DTYPE_BOOL	193	// Bit/Boolean (BOOL)
#define EIP_DTYPE_ASC	194	// ASCII/Byte (SINT)
#define EIP_DTYPE_INT	196	// Integer (DINT)
#define EIP_DTYPE_FLP	202	// Float (REAL)

// Data types
#define DTYPE_RET_INT	1
#define DTYPE_RET_FLOAT	2
#define DTYPE_RET_STR	3
#define DTYPE_RET_BOOL	4
#define DTYPE_MAXNUM	4 	// max dtype index value

// Data class types
#define DCLASS_STATS	1
#define DCLASS_ALARMS	2
#define DCLASS_PARAMS	3


#define GENARG_INSERT	1
#define GENARG_UPDATE	2

// Alarm list flags
#define ALMFLAG_TRIG_EQU	1
#define ALMFLAG_TRIG_GT		2
#define ALMFLAG_TRIG_LT		4
#define ALMFLAG_TRIG_AND	8
#define ALMFLAG_ONETIME		16
#define ALMFLAG_ALWAYS_SCAN	64
#define ALMFLAG_TOPLEVEL	128

// Fixed limits
#define ATLAS_MAX_TARGETS	128	// Size of atx_tgdex[] array

// Program fatal errors
#define EFATAL_BREAK		1
#define EFATAL_MEMORY		10
#define EFATAL_TERM		200
#define EFATAL_SEGFAULT		201
#define EFATAL_UNKNOWN		255

// Management console command flags for parser (ATLAS_MGMT_CMD.flags)
#define MGMTC_NORMAL	0
#define MGMTC_NOARGS	1 		// No args
#define MGMTC_NOSPLIT	2 		// Don't split argument string
#define MGMTC_SLESCAPE	4		// Use backslash (\) for escaping (default = no escape char)
#define MGMTC_UCESCAPE	8		// Use caret (^) for escaping

// Timers
#define ATLAS_CLK_RTC	1 		// use RTC timer
#define ATLAS_CLK_RAW	2 		// use monotonic timer, raw offset value

// Typedefs /////////////////////////////////////////////////////////

// Primary Config typedef
typedef struct {
	int logwrite;
	char logfile[128];
	FILE *logptr;
	int loglevel;
	int db_log;
	int trace_enable;
	int wait_interval;
} GCONFIG;


// typedef for in_addr
typedef struct {
	unsigned long s_addr;
} ATLAS_IN_ADDR;

// typedef for sockaddr_in
typedef struct {
	short sin_family;
	unsigned short sin_port;
	ATLAS_IN_ADDR sin_addr;
	char sin_zero[8];
} ATLAS_SOCKADDR_IN;

// Atlas drivers' session/connection data
typedef struct {
	int sock_fd;
	ATLAS_SOCKADDR_IN servaddr;
	char ip_addr[64];
	int port_num;
} ATLAS_MCS;

// Target Device typedef (PLC connection info and upkeep ptrs)
typedef struct sATLAS_TARGET {
	int id;				// id number from database
	char ip_addr[64];		// Target/PLC hostname or IP Address
	int target_type;		// 3 = AB Logix/LGX5k, 2 = AB SLC/500, 1 = AB PLC/5, 4 = Mitsubishi Q-series E71 Ethernet
					// Keyence = 5, Unknown/Invalid = 0
	Eip_Session *eip_session;	// EIP Session pointer
	Eip_Connection *eip_con;	// EIP Connection pointer
	ATLAS_MCS *mc_session;		// MC/Mitsubishi Session & Connection pointer
	int conn_id;			// Connection ID
	int conn_serial;		// Connection Serial Number
	int rpi;			// Request Packet Interval (update frequency) in milliseconds
	unsigned char *path;		// EIP Routing Path (pointer to byte array, each byte is node number)
	int path_sz;			// Size of 'path' array
	char path_str[16];		// Path string, unchanged
	int status;
	char sname[32];			// short name
	time_t last_update;		// timestamp from last update
	char descx[128];		// long name/description
	char err_msg[512];		// error message
	int port_num;			// port number
	int port_num_active;		// active port number
	int connect_count;		// connect count
	int retry_count;		// connection retry count
	int session_target;		// target to share a connection session
	struct sATLAS_TARGET *parent;	// pointer to parent
	int flags;
} ATLAS_TARGET;


typedef struct {
	int id;				// id number from database
	int target_id;			// target id
	int target_index;		// target index
	char tagname[256];		// tagname
	char dtype[8];			// data type
	int dtypei;			// data type (local storage)
	int v_int;			// value: integer
	float v_float;			// value: float
	char v_str[256];		// value: string
	//ATLAS_TARGET* host;		// pointer to host target
} ATLAS_TAG;


typedef struct {
	ATLAS_TAG tag;			// tag data
	int parent_id;			// parent id from database
	int parent_index;		// parent index in array
	int alarm_trig;			// value which triggers alarm
	int flags;			// flags
} ATLAS_ALARM;

// Target Database typedef (mySQL connection info)
typedef struct {
	char hostname[64];	// mySQL hostname or IP addr
	char db[64];		// mySQL database name
	char username[64];	// mySQL login: usernamee
	char password[64];	// mySQL login: password
	int portnum;		// mySQL port number
	int status;		// status bits
	int last_error;		// last error number from mysql_errno()
	MYSQL* conx;		// connection pointer for libmysqlclient
	struct {			// list of table names
		char status[64];
		char atlas_log[64];
		char target_list[64];
		char tag_list[64];
		char tag_history[64];
		char tag_realtime[64];
		char alarm_list[64];
		char alarm_history[64];
		char param_list[64];
		char param_history[64];
	} tables;
} ATLAS_DB;

#define ATLASM_CCONV
typedef int (*ATLAS_MGMT_CALLBACK)(char* cargs, int argcnt);

typedef struct {
	char fifo_path[256];
	int  fifoHandle;
	int  status;
} ATLAS_FIFO;

typedef struct {
	char cmdtok[32];
	int flags;
	ATLAS_MGMT_CALLBACK ccback;
} ATLAS_MGMT_CMD;

typedef struct {
	int calldepth;
	char funcname[128];
	char filename[128];
	float enter_ts;
	float leave_ts;
	float runtime;
} ATLAS_FTRACER;

// Superglobal export definitions ///////////////////////////////////
#ifdef _MAIN_FILE
	#define ZEXPORT
#else
	#define ZEXPORT extern
#endif

// Logging macros ///////////////////////////////////////////////////
#define zlog_debug(a...)	logthis(9,__FILE__,__LINE__,0,a)
#define zlog_info(a...)		logthis(2,__FILE__,__LINE__,0,a)
#define zlog_warn(a...)		logthis(1,__FILE__,__LINE__,0,a)
#define zlog_error(a...)	logthis(0,__FILE__,__LINE__,0,a)
#define zlog_event(a,b...)	logthis(2,__FILE__,__LINE__,a,b)

// EIP Readtag macros ///////////////////////////////////////////////
#define eip_readtag_int(a,b)		(int)eip_readtag(a,b,NULL)
#define eip_readtag_float(a,b)		(float)eip_readtag(a,b,NULL)
#define eip_readtag_str(a,b)		(char*)eip_readtag(a,b,NULL)

// Function debug asserts ///////////////////////////////////////////
#define ATLS_ASSERT_NULLPTR(a,z)		if(!a) { zlog_error("**ASSERT FAIL: [%s] pointer is NULL! [@ %s/Line %i/%s] \n",a,__FILE__,__LINE__,__func__); return z; }
#define ATLS_ASSERT_NONPOS(a,z)			if(!a) { zlog_error("**ASSERT FAIL: [%s] is non-positive! [@ %s/Line %i/%s] \n",a,__FILE__,__LINE__,__func__); return z; }
#define ATLS_ASSERT_LESS(a,b,z)			if(a >= b) { zlog_error("**ASSERT FAIL: [%s < %i] false! [@ %s/Line %i/%s]\n",a,b,__FILE__,__LINE__,__func__); return z; }
#define ATLS_ASSERT_GRT(a,b,z)			if(a <= b) { zlog_error("**ASSERT FAIL: [%s > %i] false! [@ %s/Line %i/%s]\n",a,b,__FILE__,__LINE__,__func__); return z; }
#define ATLS_ASSERT_EQU(a,b,z)			if(a > b) { zlog_error("**ASSERT FAIL: [%s == %i] false! [@ %s/Line %i/%s]\n",a,b,__FILE__,__LINE__,__func__); return z; }

#define ATLS_DEBUG_LOGFUNC()			zlog_debug("\t >> PING [ %s() / %s / line %i ] <<\n",__func__,__FILE__,__LINE__)
#define ATLS_FENTER()					atlas_trace_push(__func__,__FILE__,__LINE__)
#define ATLS_FLEAVE()					atlas_trace_pop(__func__,__FILE__,__LINE__)

// Superglobal variables ////////////////////////////////////////////
ZEXPORT GCONFIG global_config;

ZEXPORT int atx_targets;
ZEXPORT int atx_alarms;
ZEXPORT ATLAS_TARGET* atx_tgdex[ATLAS_MAX_TARGETS];
ZEXPORT ATLAS_ALARM** atx_aldex;

ZEXPORT int eip_readerr;	// global error indicator
ZEXPORT int eip_autorcx;	// auto-reconnect upon failed/timed-out read
ZEXPORT int eip_rcxattempt;	// reconnect attempt counter
ZEXPORT int eip_maxattempts;

/////////////////////////////////////////////////////////////////////
// Function prototypes //////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

// Target Interface Drivers /////////////////////////////////////////

// E/IP Interface //

int eip_start(ATLAS_TARGET* cur_device);
int eip_stop(ATLAS_TARGET* cur_device);
void* eip_readtag(char* tagname, ATLAS_TARGET* cur_device, int* dtype_ret);
int eip_genpath(unsigned char** target_ptr, int num_nodes, ...);
int eip_enum_taglist(ATLAS_TARGET* cur_device);

// MC Interface //

int mc_start(ATLAS_TARGET* atag);
void mc_stop(ATLAS_TARGET* atag);
void* mc_readword(char* devname, ATLAS_TARGET* atag);
int mc_readbit(char *devname, ATLAS_TARGET* atag);
int mc_decode_device(char* devstr, unsigned char* dcode, int* dnum);
int mc_batch_read(char* devname, ATLAS_TARGET* atag, void* outbuf, unsigned short seq);

char* mc_get_dev_from_val(unsigned char val);
int melsec_read_wcd(char* fname);

// Database functions ///////////////////////////////////////////////

// mySQL //
int atlas_mysql_init(ATLAS_DB* target_db);
char* atlas_gen_sqlargs(ATLAS_DB* curdb, ATLAS_TAG* curtag, char* outstr, int argtype);

// Data Handling //
int atlas_readtag(ATLAS_TARGET* cur_target, ATLAS_TAG* curtag);
int get_target_list(ATLAS_DB* dbconx);
int get_target_tags(ATLAS_DB* cur_db, ATLAS_TARGET* cur_target);
int session_share_setup(ATLAS_TARGET* child_t);

// Status Tracking //
int update_cstat(ATLAS_DB *cur_db, ATLAS_TARGET *cur_target);


// Utility functions ////////////////////////////////////////////////

int p2int(void* dptr);
float p2float(void* dptr);


// Program Control //////////////////////////////////////////////////

void atlas_shutdown(int errval);


// Socket functions /////////////////////////////////////////////////

int atlas_sock_connect(ATLAS_MCS* condata);
void atlas_sock_close(ATLAS_MCS* condata);
int atlas_sock_send(ATLAS_MCS* condata, char* txdata, int data_sz);
int atlas_sock_recv(ATLAS_MCS* condata, char* rxdata, int buf_sz);


// Logging, Debug, & Exception Handling functions ///////////////////

void log_mysql(int llevel, char* srcname, int srcline, int event_id, char* fmt, ...);
void logthis(int llevel, char* srcname, int srcline, int event_id, char* fmt, ...);
void set_target_msg(ATLAS_TARGET* cur_target, char* fmt, ...);
void signal_exc(int sig);

// FIFO /////////////////////////////////////////////////////////////

int atlas_mgmt_fifo_init(char* pipe_path);
int atlas_mgmt_fifo_close();
int atlas_mgmt_fifo_listen();
int atlas_mgmt_fifo_tx(char* odat, int dsz);
int atlas_mgmt_parse(char* cbuff, int bufsz);
int atlas_mgmt_cmdluk(char* cmdchk);
void AMF_printf(char* fmt, ...);

// Management callbacks  ////////////////////////////////////////////
int mgmtcb_status(char* cargs, int argcnt);
int mgmtcb_reload(char* cargs, int argcnt);
int mgmtcb_tag_read(char* cargs, int argcnt);
int mgmtcb_tag_add(char* cargs, int argcnt);

