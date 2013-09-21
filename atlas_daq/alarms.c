/*
	Atlas Project - Data Acquisition Daemon (DAQ)
	J. Hipps - http://jhipps.org/ (jhipps@nichiha.com)
	Nichiha USA, Inc.
	Copyright (c) 2013 Jacob Hipps/Nichiha USA, Inc.
	Version 0.04

	26 Jan 2013 - Started
*/

#include <sys/types.h>
#include <sys/socket.h>
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

ATLAS_ALARM* atlas_alarm_add(int* newdex) {
	int new_index;

	atx_alarms++;
	new_index = atx_alarms - 1;

	if((atx_aldex = realloc(atx_aldex, sizeof(void*) * atx_alarms)) == NULL) {
		zlog_error("atlas_alarm_add(): realloc() failed when attempting to increase alarm pool size!\n");
		atlas_shutdown(EFATAL_MEMORY);
		return NULL;
	}

	zlog_debug("atlas_alarm_add(): increased atx_aldex size OK!\n");

	if((atx_aldex[new_index] = malloc(sizeof(ATLAS_ALARM))) == NULL) {
		zlog_error("atlas_alarm_add(): malloc() failed when creating new alarm!\n");
		atlas_shutdown(EFATAL_MEMORY);
		return NULL;
	}

	zlog_debug("atlas_alarm_add(): created new alarm allocation OK! index = %i, atx_alarms = %i\n", new_index, atx_alarms);

	// zero the newly allocated alarm
	memset(atx_aldex[new_index],0,sizeof(ATLAS_ALARM));

	// set the new index in newdex, if set (otherwise ignore if pointer is null)
	if(newdex) {
		(*newdex) = new_index;
	}

	return atx_aldex[new_index];
}

int atlas_alarm_read(ATLAS_TARGET* cur_target, ATLAS_ALARM* cur_alarm) {
	char *daq_val;
	int daq_bool;

	ATLAS_TAG* curtag = &cur_alarm->tag;

	// acquire using appropriate target driver
	switch(cur_target->target_type) {
		case TARGET_LGX:
		case TARGET_SLC:
		case TARGET_PLC:
			daq_val = eip_readtag(curtag->tagname, cur_target, NULL);
			break;
		case TARGET_MC:
			if(curtag->dtypei == DTYPE_RET_BOOL) daq_bool = mc_readbit(curtag->tagname, cur_target);
			else daq_val = mc_readword(curtag->tagname, cur_target);

			break;
	}

	// detect errors from protocol tag/register reads
	if(daq_val == -1) {
		zlog_debug("* atlas_readtag(): Driver returned -1 indiciating read failure!\n");
		return -1;
	} else {
		if(curtag->dtypei == DTYPE_RET_INT) {
			curtag->v_int = p2int(daq_val);
			zlog_debug("\t>> v_int = %i\n",curtag->v_int);
		} else if(curtag->dtypei == DTYPE_RET_FLOAT) {
			curtag->v_float = p2float(daq_val);
			zlog_debug("\t>> v_float = %f\n",curtag->v_float);
		} else if(curtag->dtypei == DTYPE_RET_BOOL) {
			curtag->v_int = daq_bool;
			zlog_debug("\t>> v_int (BOOL) = %i\n",curtag->v_int);
		} else {
			strcpy(curtag->v_str, daq_val);
			zlog_debug("\t>> v_str = \"%s\"\n",curtag->v_str);
		}
	}

	if(eip_readerr && cur_target->target_type != TARGET_MC) {
		zlog_debug("\t>>> EIP read error detected!\n");
		return -1;
	}
}

/*
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
} ATLAS_TAG;


typedef struct {
	ATLAS_TAG tag;			// tag data
	int parent_id;			// parent id from database
	int parent_index;		// parent index in array
	int alarm_trig;			// value which triggers alarm
	int flags;			// flags
} ATLAS_ALARM;

mysql> describe alarm_list;
  +------------+--------------------+------+-----+---------+----------------+
  | Field      | Type               | Null | Key | Default | Extra          |
  +------------+--------------------+------+-----+---------+----------------+
0 | id         | bigint(20)         | NO   | PRI | NULL    | auto_increment |
1 | target_id  | int(11)            | NO   |     | NULL    |                |
2 | parent_id  | bigint(20)         | YES  |     | NULL    |                |
3 | tagname    | varchar(255)       | YES  |     | NULL    |                |
4 | alarm_trig | int(11)            | YES  |     | NULL    |                |
5 | desc_eng   | varchar(255)       | YES  |     | NULL    |                |
6 | desc_jap   | varchar(255)       | YES  |     | NULL    |                |
7 | talias     | varchar(255)       | YES  |     | NULL    |                |
8 | dtype      | enum('int','bool') | YES  |     | NULL    |                |
9 | flags      | bigint(20)         | YES  |     | NULL    |                |
  +------------+--------------------+------+-----+---------+----------------+

*/

int get_target_alarms(ATLAS_DB* cur_db, ATLAS_TARGET* cur_target) {
	MYSQL_RES* resultx;
	MYSQL_ROW  rowx;
	char qq[512];
	char datasetter[280];
	char datasetsu[280];
	char escaper[300];
	char dtx[8];
	time_t tstampx;
	int tdelta;
	ATLAS_ALARM cur_alarm;

	// break out the tag data seperately for easy access
	ATLAS_TAG* curtag = &cur_alarm.tag;

	// Ensure mySQL connection is OK
	if(cur_db->status != STATUS_READY) {
		zlog_error("get_target_alarms: mySQL connection not established! Cannot get target list.\n");
		return -1;
	}

	tstampx = time(NULL);
	tdelta = tstampx - cur_target->last_update;
	if(!cur_target->last_update) {
		tdelta = -1;
		zlog_debug("get_target_alarms: Performing initial polling round for target [%s]...\n",cur_target->sname);
	} else {
		zlog_debug("get_target_alarms: Begin new target update round for [%s]. Last update = %d [%i seconds ago].\n",cur_target->sname,cur_target->last_update,tdelta);
	}

	// Query alarm list for cur_target...
	sprintf(qq,"SELECT * FROM %s WHERE target_id = %i",cur_db->tables.alarm_list, cur_target->id);
	if(mysql_query(cur_db->conx,qq)) {
		// change db status to NOTREADY
		cur_db->status = STATUS_NOTREADY;
		cur_db->last_error = mysql_errno(cur_db->conx);
		// log the error
		zlog_error("get_target_alarms: Query failed! %i - %s [%s]\n",cur_db->last_error,mysql_error(cur_db->conx),qq);
		return -1;
	}
	resultx = mysql_store_result(cur_db->conx);

	// enumerate the tags...
	while((rowx = mysql_fetch_row(resultx))) {

		// populate tag info from database...
		curtag->id = atoi(rowx[0]);
		curtag->target_id = cur_target->id;
		if(rowx[3]) strcpy(curtag->tagname,rowx[3]);
		else curtag->tagname[0] = 0;

		// determine datatype from enum and convert to localized enum...
		strcpy(curtag->dtype,rowx[8]);
		if(!strcmp("int",rowx[8])) curtag->dtypei = DTYPE_RET_INT;
		else curtag->dtypei = DTYPE_RET_BOOL;

		// now, retrieve the tag's value from the target device...
		zlog_debug("get_target_alarms: Retrieving tag [%s] from target device [%s]...\n",curtag->tagname,cur_target->sname);

		// check to see if it's disabled
		if(cur_target->status == STATUS_DISABLED) {
			zlog_debug("get_target_alarms: Target disabled. Skipping.\n");
			set_target_msg(cur_target,"Disabled");
			continue;
		}

		// read tag using driver
		atlas_alarm_read(cur_target, &cur_alarm);

		// get current time for timestamp
		tstampx = time(NULL);

		// add new entry to history table
		zlog_debug(">> Writing to %s table...\n",cur_db->tables.alarm_history);
		sprintf(qq,"INSERT INTO %s (tag_id,target_id,dtype,v_int,v_float,v_str,tupdate) "
					 "VALUES(%i,    %i,      \'%s\',%s                 ,%d) ",
					    cur_db->tables.alarm_history,
                        curtag->id, cur_target->id, dtx, atlas_gen_sqlargs(cur_db, curtag, datasetsu, GENARG_INSERT), tstampx);
                if(mysql_query(cur_db->conx,qq)) {
			// change db status to NOTREADY
			cur_db->status = STATUS_NOTREADY;
			cur_db->last_error = mysql_errno(cur_db->conx);
			// log the error msg
			zlog_error("get_target_alarms: Query failed! %i - %s [%s]\n",mysql_errno(cur_db->conx),mysql_error(cur_db->conx),qq);
			return -1;
		}

		zlog_debug("get_target_alarms: Update round for tag [%s -> %s] is complete. (timestamp = %d)\n\n",cur_target->sname,curtag->tagname,tstampx);

	}

	mysql_free_result(resultx);
	tstampx = time(NULL);
	zlog_debug("get_target_alarms: Update round for target [%s] has completed successfully! (timestamp = %d)\n\n",cur_target->sname,tstampx);
	cur_target->last_update = (double)tstampx;

	return 0;
}
