/**
 *	Atlas Project - Data Acquisition Daemon (DAQ)
 *	
 *	Top-level Process Control
 *	Copyright (c) 2012-2013 J. Hipps
 *
 *	http://atlas.neoretro.net/
 *
 *  
 * ---
 *	author:		Jacob Hipps <jacob@neoretro.net>
 *	
 *	title:		Top-level Process Control
 *	desc: >
 *		CLI entry point (main) for atlas_daq. Also includes various
 *		debugging, logging, error & exception handling, and
 *		I/O control routines for sockets and files.
 *		
 *	src:
 *		lang:		C
 *		std:		C99
 *	srcdoc:
 *		segment:	atlas_daq
 *		path:		atlas_daq.c
 *		base:		atlas.neoretro.net/srcdoc/
 *		cdate:		07 Aug 2012
 *		mdate:		04 Nov 2013
 *		license:	MPLv2
 *	options:
 *		deps:		{ mysqlclient }
 *  ...
 */

#define _MAIN_FILE

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include "atlas_daq.h"

ATLAS_DB *global_db = NULL;
ATLAS_FTRACER tstack[128];
int tstack_dex = -1;

// data type to string array
char dtype_str[][6] = {"","int","float","str","int"};



void log_mysql(int llevel, char* srcname, int srcline, int event_id, char* fmt, ...) {
	char texbuf[2048];
	char escaper[2048];
	char qq[4096];

	va_list ap;
	va_start(ap, fmt);
	vsprintf(texbuf, fmt, ap);
	va_end(ap);
	time_t tt_clock;

	tt_clock = time(NULL);

	if(global_db && global_db->status == STATUS_READY && global_db->conx) {

		mysql_real_escape_string(global_db->conx, escaper, texbuf, strlen(texbuf));

		sprintf(qq,"INSERT INTO %s (event_id,srcname,srcline,loglevel,tstamp,logmsg) "
			   "VALUES(%i,\"%s\",%i,%i,%i,\"%s\") ",
			   	global_db->tables.atlas_log,
				event_id,srcname,srcline,llevel,tt_clock,escaper
		       );

		mysql_query(global_db->conx,qq);
	}
}

/*
 * logthis
 *	Logs info to console and/or file for troubleshooting/debugging.
 *	The zlog_* macros should be used instead of directly calling this
 *	functions. The macros automatically set log level, and fill in the
 *	source name and source line arguments.
 *	Args:
 *		llevel			Log level
 *		srcname*		Name of source file
 *		srcline			Line number where this function is called
 *		event_id		Special event id for tracking purposes
 *
 */
void logthis(int llevel, char* srcname, int srcline, int event_id, char* fmt, ...) {
	char texbuf[4096];

	va_list ap;
	va_start(ap, fmt);
	vsprintf(texbuf, fmt, ap);
	va_end(ap);

	if(global_config.loglevel >= llevel) {
		// Prefix loglevel, source file, and line number if enabled
		if(global_config.trace_enable) printf("<%i>[%s, Line %4i]  ",llevel,srcname,srcline);

		// Print the message
		printf(texbuf);

		// Log to db
		log_mysql(llevel,srcname,srcline,event_id,texbuf);

		// And log to file...
		if(global_config.logwrite) {
			// logfile write
		}
	}

}

float atlas_get_tmr_res(int clktype) {
	struct timespec ttx_rtc, ttx_raw;
	float tst_real, tst_raw;

/*
	switch(clktype) {
		case ATLAS_CLK_RTC:
			if(clock_getres(CLOCK_REALTIME,&ttx_rtc) == -1) {
				zlog_error("atlas_get_tmr_res(): clock_getres() failed!\n");
				return 0.0f;
			}

			tst_real = ttx_rtc.tv_sec + (ttx_rtc.tv_nsec / 1000000000.0f);

			return tst_real;
		case ATLAS_CLK_RAW:
			if(clock_getres(CLOCK_MONOTONIC_RAW, &ttx_raw) == -1) {
				zlog_error("atlas_get_tmr_res(): clock_gettime() failed!\n");
				return 0.0f;
			}

			tst_raw  = ttx_raw.tv_sec + (ttx_raw.tv_nsec / 1000000000.0f);

			return tst_raw;
		default:

			zlog_error("atlas_get_xtime(): Invalid clktype spec! [clktype = %i]\n",clktype);
			return 0.0f;
	}
*/
	return 0.0f;
}

float atlas_get_tmr_float(int clktype) {
	struct timespec ttx_rtc, ttx_raw;
	float tst_real, tst_raw;

/*
	switch(clktype) {
		case ATLAS_CLK_RTC:
			if(clock_gettime(CLOCK_REALTIME, &ttx_rtc) == -1) {
				zlog_error("atlas_get_usecs(): clock_gettime() failed!\n");
				return 0.0f;
			}

			tst_real = ttx_rtc.tv_sec + (ttx_rtc.tv_nsec / 1000000000.0f);

			return tst_real;

		case ATLAS_CLK_RAW:
			if(clock_gettime(CLOCK_MONOTONIC_RAW, &ttx_raw) == -1) {
				zlog_error("atlas_get_usecs(): clock_gettime() failed!\n");
				return 0.0f;
			}

			tst_raw  = ttx_raw.tv_sec + (ttx_raw.tv_nsec / 1000000000.0f);

			return tst_raw;

		default:

			zlog_error("atlas_get_xtime(): Invalid clktype spec! [clktype = %i]\n",clktype);
			return 0.0f;
	}
*/
	
	return 0.0f;
}

/*
typedef struct {
	int calldepth;
	char funcname[128];
	char filename[128];
	float enter_ts;
	float leave_ts;
	float runtime;
} ATLAS_FTRACER;
*/
void atlas_trace_push(char *xfunc, char *xfile, int xline) {
	/*
	char texbuf[4096];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(texbuf, fmt, ap);
	va_end(ap);
	*/

	tstack_dex++;

	strcpy(tstack[tstack_dex].funcname,xfunc);
	strcpy(tstack[tstack_dex].filename,xfile);
	tstack[tstack_dex].enter_ts = atlas_get_tmr_float(ATLAS_CLK_RTC);
	tstack[tstack_dex].leave_ts = 0.0f;
	tstack[tstack_dex].runtime = 0.0f;
	tstack[tstack_dex].calldepth = 1;

	return;
}

void atlas_trace_pop(char *xfunc, char *xfile, int xline) {
	/*
	char texbuf[4096];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(texbuf, fmt, ap);
	va_end(ap);
	*/

	tstack[tstack_dex].leave_ts = atlas_get_tmr_float(ATLAS_CLK_RTC);
	tstack[tstack_dex].runtime  = tstack[tstack_dex].leave_ts - tstack[tstack_dex].enter_ts;
	tstack[tstack_dex].calldepth = 1;

	tstack_dex--;

	#ifdef ATLS_DBG_FUNC_PROFILE
		zlog_debug("\t\t>> TTRACE [%s] %0.04fms",tstack[tstack_dex].funcname,tstack[tstack_dex].runtime * 1000.0f);
	#endif

	return;
}

///////////////////////////////////////////////////////////////////////////////
// Socket Functions
///////////////////////////////////////////////////////////////////////////////

int atlas_sock_connect(ATLAS_MCS* condata) {

	if((condata->sock_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		zlog_error("atlas_sock_connect(): Failed to create socket!\n");
		return 0;
	}

	zlog_debug("atlas_sock_connect(): Socket created OK!\n");

	condata->servaddr.sin_family = AF_INET;
	condata->servaddr.sin_addr.s_addr = inet_addr(condata->ip_addr);
	condata->servaddr.sin_port = htons(condata->port_num);

	if(connect(condata->sock_fd, (struct sockaddr*) &condata->servaddr, sizeof(condata->servaddr)) < 0) {
		zlog_error("atlas_sock_connect(): Failed to connect to %s:%u :(\n",condata->ip_addr,condata->port_num);
		return 0;
	}

	zlog_debug("atlas_sock_connect(): Successfully connected to %s:%u :)\n",condata->ip_addr,condata->port_num);
	return 1;
}

void atlas_sock_close(ATLAS_MCS* condata) {

	close(condata->sock_fd);
	zlog_debug("atlas_sock_close(): Connection closed.\n");
}

int atlas_sock_send(ATLAS_MCS* condata, char* txdata, int data_sz) {
	int tx_len;

	// Send data using send()
	if((tx_len = send(condata->sock_fd, txdata, data_sz, 0)) != data_sz) {
		zlog_error("atlas_sock_send(): Transmit size mismatch!\n");
	}

	return tx_len;
}

int atlas_sock_recv(ATLAS_MCS* condata, char* rxdata, int buf_sz) {
	int rx_len;

	// Wait for data, then capture with recv()
	if((rx_len = recv(condata->sock_fd, rxdata, buf_sz, 0)) < 1) {
		zlog_error("atlas_sock_recv(): Failed to receive data from target!\n");
		return 0;
	}

	return rx_len;
}


///////////////////////////////////////////////////////////////////////////////
// Data Conversion
///////////////////////////////////////////////////////////////////////////////

int p2int(void* dptr) {
	if(dptr == 0 || dptr == -1) return -1;
	int *zptr = (int*)dptr;
	int zint = (*zptr);
	return zint;
}

float p2float(void* dptr) {
	if(dptr == 0 || dptr == -1) return -1;
	float *zptr = (float*)dptr;
	float zfloat = (*zptr);
	return zfloat;
}

// Returns datatype string from index value
char* get_dtype_str(int dtypei) {
	if(dtypei < 1 || dtypei > DTYPE_MAXNUM) return dtype_str[0];
	else return dtype_str[dtypei];
}

void set_target_msg(ATLAS_TARGET* cur_target, char* fmt, ...) {
	char texbuf[512];
	va_list ap;
	va_start(ap,fmt);
	vsprintf(texbuf, fmt, ap);
	va_end(ap);

	zlog_debug("set_target_msg() called.\n");
	strcpy(cur_target->err_msg, texbuf);
	zlog_debug("set_target_msg() OK.\n");

}

int atlas_mysql_init(ATLAS_DB* target_db) {

	// set default port number if not set
	if(!target_db->portnum) target_db->portnum = 3306;

	zlog_debug("atlas_mysql_init: Connecting to mySQL server on %s:%i ...\n",target_db->hostname,target_db->portnum);

	// Initialize connection...
	target_db->conx = mysql_init(NULL);
	if(!target_db->conx) {
		zlog_error("atlas_mysql_init: Error during mysql_init() - Err %u - %s\n",mysql_errno(target_db->conx),mysql_error(target_db->conx));
		target_db->status = STATUS_INITFAIL;
		return target_db->status;
	} else {
		zlog_debug("atlas_mysql_init: mysql_init() OK!\n");
	}

	// Connect...
	if(!mysql_real_connect(target_db->conx,target_db->hostname,target_db->username,target_db->password,target_db->db,target_db->portnum, NULL /*socket*/, 0 /*client flags*/)) {
		zlog_error("atlas_mysql_init: mysql_real_connect() failed! Err %u - %s\n",mysql_errno(target_db->conx),mysql_errno(target_db->conx));
		target_db->status = STATUS_INITFAIL;
		return target_db->status;
	} else {
		zlog_debug("atlas_mysql_init: mysql_real_connect() OK!\n");
	}

	zlog_debug("atlas_mysql_init: mySQL connection established successfully!\n");
	target_db->status = STATUS_READY;
	target_db->last_error = 0;

	return target_db->status;
}

int session_share_setup(ATLAS_TARGET* child_t) {

	ATLAS_TARGET* parent_t = NULL;
	int tid = 0;

	// find the source_target parent
	for(int i = 0; i < atx_targets; i++) {
		if(atx_tgdex[i]->id == child_t->session_target) {
			parent_t = atx_tgdex[i];
			tid = i;
			break;
		}
	}

	zlog_debug("session_share_setup(): Parent id [%i] found at index [%i] - [%s]\n",child_t->session_target,tid,atx_tgdex[tid]->sname);

	// Check for no match
	if(!parent_t) {
		zlog_error("session_share_setup(): [%s] Unable to match session_target! Target disabled.\n",child_t->sname);
		child_t->status = STATUS_DISABLED;
		return -1;
	}

	// AB session sharing is not supported due to TuxEip
	if(parent_t->target_type < 4) {
		zlog_error("session_share_setup(): [%s] Session sharing is not supported for Allen-Bradley targets! Target disabled.\n",child_t->sname);
		child_t->status = STATUS_DISABLED;
		return -1;
	}

	// Ensure target types match
	if(parent_t->target_type != child_t->target_type) {
		zlog_error("session_share_setup(): [%s] Target and parent session target types do not match! Target disabled.\n",child_t->sname);
		child_t->status = STATUS_DISABLED;
		return -1;
	}

	// Parent must already be connected and initialized before this function is called!
	// Otherwise the mc_session pointer will be null and useless
	if(!parent_t->mc_session) {
		zlog_error("session_share_setup(): Target parent session has not been initialized! This target will be disabled.\n");
		child_t->status = STATUS_DISABLED;
		return -1;
	}

	// duplicate mc_session pointer and set parent pointer
	child_t->mc_session = parent_t->mc_session;
	child_t->parent = parent_t;

	zlog_debug("session_share_setup(): Successfully associated [%s] to parent connection [%s].\n",child_t->sname,parent_t->sname);
	return tid;
}

int get_target_list(ATLAS_DB* dbconx) {

	MYSQL_RES* resultx;
	MYSQL_ROW  rowx;
	ATLAS_TARGET* cur_target;
	int pathsz_t;
	char mquery[256];

	// ensure we've established a connection...
	if(dbconx->status != STATUS_READY) {
		zlog_error("get_target_list: mySQL connection not established! Cannot get target list.\n");
		return -1;
	}

	zlog_debug("get_target_list(): Querying database for target list and params...\n");

	// query...
	sprintf(mquery,"SELECT * FROM %s",dbconx->tables.target_list);
	mysql_query(dbconx->conx, mquery);
	resultx = mysql_store_result(dbconx->conx);
	if(!resultx) {
		zlog_error("get_target_list(): Query failed! [%s]\n",mysql_error(dbconx->conx));
		return -1;
	}


	// enumerate the targets...
	while((rowx = mysql_fetch_row(resultx))) {
		// allocate memory for new target data
		if((atx_tgdex[atx_targets] = malloc(sizeof(ATLAS_TARGET))) == NULL) {
			zlog_error("CRITICAL: get_target_list: Memory allocation error!\n");
			exit(1);
		}
		cur_target = atx_tgdex[atx_targets];

		zlog_debug(">> target[%i] - data addr = 0x%08X, sname = %s, target_type = %i <<\n",atx_targets,cur_target,rowx[6],atoi(rowx[2]));

		// populate target's struct
		cur_target->id = atoi(rowx[0]);			// id = id
		strcpy(cur_target->ip_addr, rowx[3]);		// ip_addr = ip_addr
		cur_target->target_type = atoi(rowx[2]);	// target_type = typex
		strcpy(cur_target->sname, rowx[6]);		// sname = sname
		strcpy(cur_target->descx, rowx[1]);		// descx = descx
		if(rowx[7]) cur_target->port_num = atoi(rowx[7]); // port_num = port_num
		else        cur_target->port_num = 0;
		if(rowx[8]) cur_target->flags = atoi(rowx[8]);    // flags = flags
		else        cur_target->flags = 0;
		if(rowx[9]) cur_target->session_target = atoi(rowx[9]);
		else        cur_target->session_target = 0;

		// parse path...
		if(rowx[4]) {
			pathsz_t = strlen(rowx[4]);
			if((cur_target->path = malloc(pathsz_t)) == NULL) {
				zlog_error("CRITICAL: Memory allocation failed!\n");
				exit(1);
			}

			for(int l = 0; l < pathsz_t; l++) {
				// convert ASCII char to unsigned char integer
				cur_target->path[l] = (unsigned char)(rowx[4][l] - 48);
			}
			cur_target->path_sz = pathsz_t;

			// Copy string verbatim to path_str
			strcpy(cur_target->path_str,rowx[4]);
		} else {
			zlog_debug(">> target path is null.\n");
			cur_target->path = NULL;
			cur_target->path_sz = 0;
			cur_target->path_str[0] = 0x00;
		}
		
		// zero session data pointers...
		cur_target->eip_session = NULL;
		cur_target->eip_con = NULL;
		cur_target->mc_session = NULL;

		// other param defaults...
		cur_target->status = STATUS_NOTREADY;
		cur_target->port_num_active = cur_target->port_num;
		cur_target->rpi = 5000;
		cur_target->conn_id = 0;
		cur_target->conn_serial = 0;
		cur_target->last_update = 0;
		cur_target->connect_count = 0;
		cur_target->retry_count = 0;
		cur_target->parent = NULL;
		set_target_msg(cur_target,"OK");

		atx_targets++;
	}

	mysql_free_result(resultx);

}


char* atlas_gen_sqlargs(ATLAS_DB* curdb, ATLAS_TAG* curtag, char* outstr, int argtype) {
	char escaper[512];

	if(argtype == GENARG_INSERT) {
		if(curtag->dtypei == DTYPE_RET_INT || curtag->dtypei == DTYPE_RET_BOOL) {
			sprintf(outstr,"%i,NULL,NULL",curtag->v_int);
		} else if(curtag->dtypei == DTYPE_RET_FLOAT) {
			sprintf(outstr,"NULL,%f,NULL",curtag->v_float);
		} else {
			mysql_real_escape_string(curdb->conx, escaper, curtag->v_str, strlen(curtag->v_str));
			sprintf(outstr,"NULL,NULL,\"%s\"",escaper);
		}
	} else if(argtype == GENARG_UPDATE) {
		if(curtag->dtypei == DTYPE_RET_INT || curtag->dtypei == DTYPE_RET_BOOL) {
			sprintf(outstr,"v_int = %i",curtag->v_int);
		} else if(curtag->dtypei == DTYPE_RET_FLOAT) {
			sprintf(outstr,"v_float = %f",curtag->v_float);
		} else {
			mysql_real_escape_string(curdb->conx, escaper, curtag->v_str, strlen(curtag->v_str));
			sprintf(outstr,"v_str = \"%s\"",escaper);
		}
	}

	return outstr;
}

int atlas_readtag(ATLAS_TARGET* cur_target, ATLAS_TAG* curtag) {

	char *daq_val;

	// acquire using appropriate target driver
	switch(cur_target->target_type) {
		case TARGET_LGX:
		case TARGET_SLC:
		case TARGET_PLC:
			daq_val = eip_readtag(curtag->tagname, cur_target, NULL);
			break;
		case TARGET_MC:
			daq_val = mc_readword(curtag->tagname, cur_target);
			break;
	}

	// detect errors from protocol tag/register reads
	if(daq_val == -1) {
		zlog_debug("* atlas_readtag(): Driver returned -1 indiciating read failure!\n");
		return -1;
	} else {
		if(curtag->dtypei == DTYPE_RET_INT || curtag->dtypei == DTYPE_RET_BOOL) {
			curtag->v_int = p2int(daq_val);
			zlog_debug("\t>> v_int = %i\n",curtag->v_int);
		} else if(curtag->dtypei == DTYPE_RET_FLOAT) {
			curtag->v_float = p2float(daq_val);
			zlog_debug("\t>> v_float = %f\n",curtag->v_float);
		} else {
			strcpy(curtag->v_str, daq_val);
			zlog_debug("\t>> v_str = \"%s\"\n",curtag->v_str);
		}
	}

	if(eip_readerr && cur_target->target_type != TARGET_MC) {
		zlog_debug("\t>>> EIP read error detected!\n");
		return -1;
	}

	return 0;
}

int get_target_tags(ATLAS_DB* cur_db, ATLAS_TARGET* cur_target) {
	MYSQL_RES* resultx;
	MYSQL_ROW  rowx;
	ATLAS_TAG  curtag;
	char qq[512];
	char datasetter[280];
	char datasetsu[280];
	char escaper[300];
	//char dtx[8];
	time_t tstampx;
	int tdelta;

	// Ensure mySQL connection is OK
	if(cur_db->status != STATUS_READY) {
		zlog_error("get_target_tags: mySQL connection not established! Cannot get target list.\n");
		return -1;
	}

	tstampx = time(NULL);
	tdelta = tstampx - cur_target->last_update;
	if(!cur_target->last_update) {
		tdelta = -1;
		zlog_debug("get_target_tags: Performing initial polling round for target [%s]...\n",cur_target->sname);
	} else {
		zlog_debug("get_target_tags: Begin new target update round for [%s]. Last update = %i [%i seconds ago].\n",cur_target->sname,cur_target->last_update,tdelta);
	}

	// Query taglist for cur_target...
	sprintf(qq,"SELECT * FROM %s WHERE target_id = %i",cur_db->tables.tag_list, cur_target->id);
	if(mysql_query(cur_db->conx,qq)) {
		// change db status to NOTREADY
		cur_db->status = STATUS_NOTREADY;
		cur_db->last_error = mysql_errno(cur_db->conx);
		// log the error
		zlog_error("get_target_tags: Query failed! %i - %s [%s]\n",cur_db->last_error,mysql_error(cur_db->conx),qq);
		return -1;
	}
	resultx = mysql_store_result(cur_db->conx);

	// enumerate the tags...
	while((rowx = mysql_fetch_row(resultx))) {

		// populate tag info from database...
		curtag.id = atoi(rowx[0]);
		curtag.target_id = cur_target->id;
		if(rowx[2]) strcpy(curtag.tagname,rowx[2]);
		else curtag.tagname[0] = 0;

		// determine datatype from enum and convert to localized enum...
		strcpy(curtag.dtype,rowx[8]);
		if(!strcmp("int",rowx[8])) curtag.dtypei = DTYPE_RET_INT;
		else if(!strcmp("float",rowx[8])) curtag.dtypei = DTYPE_RET_FLOAT;
		else curtag.dtypei = DTYPE_RET_STR;

		// now, retrieve the tag's value from the target device...
		zlog_debug("get_target_tags: Retrieving tag [%s] from target device [%s]...\n",curtag.tagname,cur_target->sname);

		// check to see if it's disabled
		if(cur_target->status == STATUS_DISABLED) {
			zlog_debug("get_target_tags: Target disabled. Skipping.\n");
			set_target_msg(cur_target,"Disabled");
			continue;
		}

		// read tag using driver
		atlas_readtag(cur_target, &curtag);

		// get current time for timestamp
		tstampx = time(NULL);

		// now, write to the realtime table. update if it exists, otherwise create it...
		zlog_debug(">> Writing to '%s' table...\n", cur_db->tables.tag_realtime);
		sprintf(qq,"INSERT INTO %s (tag_id,target_id,dtype,v_int,v_float,v_str,tupdate,flags) "
					  "VALUES(%i,    %i,      \'%s\',%s                 ,%d     , 0) "
					  "ON DUPLICATE KEY UPDATE %s, tupdate = %d",
			cur_db->tables.tag_realtime,
			curtag.id, cur_target->id, get_dtype_str(curtag.dtypei), atlas_gen_sqlargs(cur_db, &curtag, datasetsu, GENARG_INSERT), tstampx, atlas_gen_sqlargs(cur_db, &curtag, datasetter, GENARG_UPDATE), tstampx);
		if(mysql_query(cur_db->conx,qq)) {
			zlog_error("get_target_tags: Query failed! %i - %s [%s]\n",mysql_errno(cur_db->conx),mysql_error(cur_db->conx),qq);
			return -1;
		}

		// next, add new entry to history table...
		zlog_debug(">> Writing to '%s' table...\n", cur_db->tables.tag_history);
		sprintf(qq,"INSERT INTO %s (tag_id,target_id,dtype,v_int,v_float,v_str,tupdate) "
					 "VALUES(%i,    %i,      \'%s\',%s                 ,%i) ",
			cur_db->tables.tag_history,
                        curtag.id, cur_target->id, get_dtype_str(curtag.dtypei), atlas_gen_sqlargs(cur_db, &curtag, datasetsu, GENARG_INSERT), tstampx);
                if(mysql_query(cur_db->conx,qq)) {
			// change db status to NOTREADY
			cur_db->status = STATUS_NOTREADY;
			cur_db->last_error = mysql_errno(cur_db->conx);
			// log the error msg
			zlog_error("get_target_tags: Query failed! %i - %s [%s]\n",mysql_errno(cur_db->conx),mysql_error(cur_db->conx),qq);
			return -1;
		}

		zlog_debug("get_target_tags: Update round for tag [%s -> %s] is complete. (timestamp = %i)\n\n",cur_target->sname,curtag.tagname,tstampx);

	}

	mysql_free_result(resultx);
	tstampx = time(NULL);
	zlog_debug("get_target_tags: Update round for target [%s] has completed successfully! (timestamp = %i)\n\n",cur_target->sname,tstampx);
	cur_target->last_update = tstampx;

	return 0;
}

// update status information in database...
int update_cstat(ATLAS_DB *cur_db, ATLAS_TARGET *cur_target) {
	char qq[2048];
	time_t tt_clock;

	zlog_debug("Updating connection status info...\n");

	tt_clock = time(NULL);
	sprintf(qq,"INSERT INTO %s (sname,descx,tupdate,st_msg,statx,supdate) "
		   "VALUES(\"%s\",\"%s\",%i,\"%s\",%i,%i) "
		   "ON DUPLICATE KEY UPDATE tupdate = %i, st_msg = \"%s\", statx = %i, supdate = %i",
		   	cur_db->tables.status,
			cur_target->sname, cur_target->descx, cur_target->last_update,
			cur_target->err_msg, cur_target->status, tt_clock, cur_target->last_update,
			cur_target->err_msg, cur_target->status, tt_clock
	       );

	mysql_query(cur_db->conx,qq);

	return 0;
}


void signal_exc(int sig) {

	if(sig == SIGHUP) {
		zlog_warn("** SIGHUP caught. Restarting...\n");
		// TODO: Implement this properly
	} else if(sig == SIGINT) {
		zlog_warn("** SIGINT caught. Shutting down...\n");
		atlas_shutdown(EFATAL_BREAK);
	} else if(sig == SIGTERM) {
		zlog_error("** EXCEPTION: SIGTERM caught. Goodbye.\n");
		atlas_shutdown(EFATAL_TERM);
	} else if(sig == SIGSEGV) {
		zlog_error("** EXCEPTION: SIGSEGV caught. Segmentation fault :(\n");
		exit(EFATAL_SEGFAULT);
	} else if(sig == SIGCHLD) {
		// do something useful here...
		zlog_error("** SIGCHLD caught. What to do?\n");
	} else {
		zlog_error("** CRITICAL: Unhandled exception! [sig = %i]\n",sig);
		exit(EFATAL_UNKNOWN);
	}
}

void atlas_shutdown(int errval) {
	int mysql_alive = 0;

	zlog_error("atlas_shutdown(): Shutting down. Disconnecting targets...\n");

	atlas_mgmt_fifo_close();

	if(global_db) {
		if(global_db->conx) {
			mysql_alive = 1;
		}
	}

	for(int tgi = 0; tgi < atx_targets; tgi++) {

		if(mysql_alive) {
			// Update status database
			set_target_msg(atx_tgdex[tgi],"atlas_shutdown(%u) [status = %u]",errval,atx_tgdex[tgi]->status);
			atx_tgdex[tgi]->status = STATUS_SHUTDOWN;
			update_cstat(global_db, atx_tgdex[tgi]);
		}

		if(!(atx_tgdex[tgi]->flags & TFLAG_CSESSION)) {
			// Call the stop function for the appropriate target driver
			switch(atx_tgdex[tgi]->target_type) {
				case TARGET_LGX:
				case TARGET_SLC:
				case TARGET_PLC:
					eip_stop(atx_tgdex[tgi]);
					break;
				case TARGET_MC:
					mc_stop(atx_tgdex[tgi]);
					break;
				default:
					break;
			}
		}

		// Free this target's memory
		free(atx_tgdex[tgi]);
	}

	if(mysql_alive) {
		zlog_error("atlas_shutdown(): Closing mySQL connections.\n");
		mysql_close(global_db->conx);
		global_db->status = STATUS_SHUTDOWN;
	}

	zlog_error("Farewell! :)\n\n");
	exit(errval);
}

int main(int argc, char** argv) {

	int ic_attempts;
	int ic_attempts_max = 3;
	int tgi;
	int fpid;
	int pgid;
	int forkmode = 0;
	int solo_driver = 0;
	
	ATLAS_DB daqdb = {
		"localhost", "atlas_daq", "atlas", "booboocat20", 3306, 0, 0, NULL,
		{ "status", "atlas_daq_log", "targets", "taglist", "history", "realtime",
		  "alarm_list", "alarm_history", "", "" }
	};

	// Global Config Default setup
	global_config.wait_interval = 10;

	// Initialize EIP error globals
	eip_readerr = 0;    // global error indicator
	eip_autorcx = 1;    // auto-reconnect upon failed/timed-out read
	eip_rcxattempt = 0; // reconnect attempt counter
	eip_maxattempts = 3;

	// Setup logging params
	global_config.trace_enable = 0;
	global_config.db_log   = 1;
	global_config.logwrite = 1;
	global_config.loglevel = 2;
	strcpy(global_config.logfile,"atlas_daq.log");

	// Setup Exception & Signal Handling
	signal(SIGTERM,signal_exc);
	signal(SIGHUP,signal_exc);
	signal(SIGINT,signal_exc);
	signal(SIGSEGV,signal_exc);
	signal(SIGCHLD,signal_exc);
	
	// Show start-up banner
	printf("Atlas Data Acquisition Daemon\n");
	printf("J. Hipps (jhipps@nichiha.com) - http://jhipps.org/\n");
	printf("Copyright (c) 2012-2013 Jacob Hipps/Nichiha USA, Inc., all rights reserved.\n");
	printf("Version %s (compiled %s %s)\n\n",ATLASDAQ_VERSION,__DATE__,__TIME__);

	// process command line params
	char* thisarg;
	for(int ci = 1; ci < argc; ci++) {
		thisarg = argv[ci];
		if(!thisarg) break;
		if(!thisarg[0]) break;

		if(!strcmp(thisarg,"--loglevel")) {
			if(argc <= ci+1) {
				zlog_error("error: loglevel requires argument!\n");
				exit(1);
			}
			global_config.loglevel = atoi(argv[ci+1]);
			if(global_config.loglevel < 1) global_config.loglevel = 1;
			ci++;
		} else if(!strcmp(thisarg,"--dboff")) {
			// Disable logging to mySQL
			global_config.db_log = 0;
		} else if(!strcmp(thisarg,"--dball")) {
			// Log all event for current loglevel to mySQL
			global_config.db_log = 9;
		} else if(!strcmp(thisarg,"--trace")) {
			global_config.trace_enable = 1;
		} else if(!strcmp(thisarg,"--solo")) {
			if(argc <= ci+1) {
				zlog_error("error: solo requires argument!\n");
				exit(1);
			}
			solo_driver = atoi(argv[ci+1]);
			if(solo_driver < 1) {
				zlog_error("error: invalid solo target spec!\n");
				exit(2);
			}
		}
	}
	
	// XXX-DEBUG FIXME
	melsec_read_wcd("comment_test.wcd");

	exit(1);

	// populate mySQL params
	// TODO - Retrieve mySQL connection params from a file..
	zlog_debug("mySQL client lib version: %s\n\n",mysql_get_client_info());
	atlas_mysql_init(&daqdb);
	global_db = &daqdb;	// copy to global_db pointer for shutdown purposes

	//atlas_mgmt_fifo_init("/home/jacob/atlas_fifo");

	zlog_event(1,"ATLAS DAQ. Version %s (compiled %s %s). Ready.",ATLASDAQ_VERSION,__DATE__,__TIME__);

	// Retrieve list of target devices from mySQL table ('targets')
	get_target_list(&daqdb);

	if(solo_driver) zlog_warn("[SOLO MODE ACTIVE] Target = %i",solo_driver);

	// Initialize the connection to the target devices
	for(tgi = 0; tgi < atx_targets; tgi++) {
		ic_attempts = 0;

		// set all other targets to "disabled" in solo mode
		if(solo_driver && tgi != (solo_driver - 1)) {
			atx_tgdex[tgi]->status = STATUS_DISABLED;
			continue;
		}

		zlog_debug("[%i]*  Initializing target driver. Segment addr = 0x%08X\n",tgi,atx_tgdex[tgi]);
		zlog_debug("[%i]** Driver target_type = %i\n",tgi,atx_tgdex[tgi]->target_type);

		// Check for and handle shared sessions
		if(atx_tgdex[tgi]->flags & TFLAG_CSESSION) {
			if(session_share_setup(atx_tgdex[tgi]) != -1) {
				zlog_debug("[%i]** Shared session setup completed successfully! Target ready.\n");
				atx_tgdex[tgi]->status = STATUS_READY;
			}
		} else {
			switch(atx_tgdex[tgi]->target_type) {
				case TARGET_LGX:
				case TARGET_SLC:
				case TARGET_PLC:
					while(atx_tgdex[tgi]->status != STATUS_READY && ic_attempts < ic_attempts_max) {
						eip_start(atx_tgdex[tgi]);
						ic_attempts++;
					}
					break;
				case TARGET_MC:
					// Mitsubishi Q - MC Protocol (ミツビシ MC プロトコル)
					while(atx_tgdex[tgi]->status != STATUS_READY && ic_attempts < ic_attempts_max) {
						mc_start(atx_tgdex[tgi]);
						ic_attempts++;
					}
					break;
				case TARGET_KEY:
					// TODO - Keyence EtherNet/IP Protocol.
					//        Should (maybe? hopefully!?) be able to use
					//        eip_start() and TuxEip library for Keyence stuff!
					break;
				default:
					zlog_error("ERROR: Unhandled target device type [%i]! This target will be disabled.\n",atx_tgdex[tgi]->target_type);
					atx_tgdex[tgi]->status = STATUS_DISABLED;
			}
		}

		if(atx_tgdex[tgi]->status == STATUS_READY) {
			if(atx_tgdex[tgi]->flags & TFLAG_CSESSION) zlog_info("[INIT] Successfully associated [%s] to parent connection [%s]\n",atx_tgdex[tgi]->sname,atx_tgdex[tgi]->parent->sname);
			else zlog_info("[INIT] Successfully connected to %s [%02i/%s] at %s\n",atx_tgdex[tgi]->descx,atx_tgdex[tgi]->id,atx_tgdex[tgi]->sname,atx_tgdex[tgi]->ip_addr);
		}
	}


	eip_enum_taglist(atx_tgdex[solo_driver]);
	atlas_shutdown(0);

	// Fork into background... (daemonize)
	/*
	if(forkmode) {
		fpid = fork();
		if(fpid < 0) {
			zlog_error("CRITICAL: Unable to fork into background!\n");
			exit(255);
		}
		if(fpid > 0) {
			zlog_info("Forked into background.\n");
			exit(0);
		}
		pgid = setsid();
	}
	*/

	// Main acquisition loop
	zlog_info("[INIT] Startup complete! Entering main acquisition loop. Cycle time is %i seconds.\n",global_config.wait_interval);
	while(1) {
		for(tgi = 0; tgi < atx_targets; tgi++) {
			get_target_tags(&daqdb, atx_tgdex[tgi]);	// update tags
			update_cstat(&daqdb, atx_tgdex[tgi]);		// update status
		}
		// check to ensure mySQL connection is still up...
		if(daqdb.status != STATUS_READY) {
			zlog_error("[mySQL] mySQL connection is NOT ready! Attempting to re-establish connectivity...\n");
			//mysql_close(&daqdb);
			if(atlas_mysql_init(&daqdb) == STATUS_READY) {
				zlog_error("[mySQL] Connection re-established OK! :)\n");
			}
		}
		zlog_debug("\n\n*** Cycle complete. Waiting %i seconds ***\n\n",global_config.wait_interval);
		sleep(global_config.wait_interval);
	}

	return 0;
}


