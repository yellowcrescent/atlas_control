/*

	Atlas Project - Data Acquisition Daemon (DAQ)
	J. Hipps - http://jhipps.org/ (jhipps@nichiha.com)
	Nichiha USA, Inc.

	EtherNet/IP & Common Industrial Protocol (CIP) - TuxEIP Interface
        Copyright (c) 2012-2013 Jacob Hipps/Nichiha USA, Inc.

	TuxEip,	Copyright (c) 2006 Stephane Jeanne/TUXPLC

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include "atlas_daq.h"

extern CIP_UINT _OriginatorVendorID;
extern CIP_UDINT _OriginatorSerialNumber;
extern BYTE _Priority;
extern CIP_USINT _TimeOut_Ticks;
extern WORD _Parameters;
extern BYTE _Transport;
extern BYTE _TimeOutMultiplier;

#define ATLS_EIP_MRS_GET_INSTANCE_ATTRIBUTE_LIST	0x0055


typedef struct {
	unsigned int instance_id;
	unsigned short int stype;
	unsigned short int namestr_sz;
	char* namestr;
} EIP_SYMBOL_INSTANCE_DATA;

int eip_enum_taglist(ATLAS_TARGET* cur_device) {

	MR_Reply* mr_reply;
	int mr_reply_sz;
	char* resp_data;
	int resp_data_sz;
	int reply_status = 1;

	// request data:
	// num of attribs (word), attrib #2 - symbol type (word), attrib #1 - symbol len & name (word)
	char rq_data[6] = { 0x02, 0x00, 0x02, 0x00, 0x01, 0x00 };
	int rq_data_sz = 6;

	char path_data[6] = { 0x20, 0x6b, 0x25, 0x00, 0x00, 0x00 };
	unsigned short int* instance_id = (unsigned short int*)&path_data[4];
	int path_data_sz = 6;

	// Build request packet
	//while(reply_status) {
		zlog_debug("eip_enum_taglist(): Sending CIP/MR packet, service = 0x%04x, instance_id = 0x%04x\n",ATLS_EIP_MRS_GET_INSTANCE_ATTRIBUTE_LIST,(*instance_id));
		mr_reply = _SendMRRequest(cur_device->eip_session, ATLS_EIP_MRS_GET_INSTANCE_ATTRIBUTE_LIST, path_data, path_data_sz, rq_data, rq_data_sz, &mr_reply_sz);
		ATLS_DEBUG_LOGFUNC();
		resp_data = _GetMRData(mr_reply);
		ATLS_DEBUG_LOGFUNC();
		reply_status = mr_reply->General_Status;
		ATLS_DEBUG_LOGFUNC();
		zlog_debug("eip_enum_taglist(): Got response! Status = 0x%02x, data sz = %i\n",reply_status,resp_data_sz);
	//}

	return 0;
}

/*
 * eip_genpath
 * 	Allocates memory for, then builds, TuxEIP's 'path' argument.
 *	Args:
 *		target_ptr**		Pointer to address of path var
 *		num_nodes		Number of nodes in path list
 *		...			List of node numbers for path
 *	Returns:
 *		Number of nodes in list
 */

int eip_genpath(unsigned char** target_ptr, int num_nodes, ...) {
	unsigned char cur_arg;

	// allocate memory for the path info (one byte per hop/node)
	(*target_ptr) = malloc(num_nodes);

	// check to ensure we have a valid pointer...
	if((*target_ptr) == NULL) {
		zlog_error("CRITICAL: Memory allocation failed! Failed to allocate %d bytes during eip_genpath() call!\n\n",num_nodes);
		exit(1);
	}

	// run through var-arg list and copy node addresses to path memory area
	va_list ap;
	va_start(ap, num_nodes);
	// loop through and read in all of the node addresses to create the path
	for(int i = 0; i < num_nodes; i++) {
		cur_arg = va_arg(ap, unsigned char);
		target_ptr[i] = cur_arg;
	}
	va_end(ap);

	return num_nodes;
}

int eip_start(ATLAS_TARGET* cur_device) {

	int cur_type;
	int cur_id;
	int cur_serial;
	int cur_rpi;
	char* cur_path;
	int cur_path_sz;
	int session_reg;
	Eip_Session *cur_session;
	Eip_Connection *cur_connection;

	// Populate current device info from ATLAS_TARGET object...
	cur_type   = cur_device->target_type;
	cur_id     = cur_device->conn_id++;
	cur_serial = cur_device->conn_serial++;
	cur_rpi    = cur_device->rpi;
	cur_path   = cur_device->path;
	cur_path_sz = cur_device->path_sz;

	// increment retry count
	cur_device->retry_count++;

	// Begin EIP session (Connect to PLC's comms device at given address)
	if(cur_device->connect_count) {
		zlog_error("Destroying old connection...\n");
		eip_stop(cur_device);
	}
	cur_session = OpenSession(cur_device->ip_addr);	
	if(!cur_session) {
		zlog_error("[%s] OpenSession() failed when attempting to connect to %s!\n\n",cur_device->sname,cur_device->ip_addr);
		cur_device->status = STATUS_COMFAIL;
		return -1;
	} else {
		zlog_debug("OpenSession() succeeded!\n");
	}

	// Check & Set Timeout params...
	// By default, timeout=1000, we will set to 10000 (milliseconds) since we have a laggy network
	// (especially paintline being slow as hell)
	cur_session->timeout = 10000;
	zlog_debug("** eip_session->timeout = %i\n",cur_session->timeout);

	// Register EIP session
	session_reg = RegisterSession(cur_session);
	if(session_reg != Error) {
		zlog_debug("RegisterSession() succeeded!\n");
	} else {
		zlog_error("[%s] RegisterSession() failed! %s (%i : %i)\n\n",cur_device->sname,cip_err_msg, cip_errno, cip_ext_errno);
		cur_device->status = STATUS_COMFAIL;
		return -1;
	}

	// Perform connection to target device over specified path
	cur_connection = ConnectPLCOverCNET(cur_session, (Plc_Type)cur_type, cur_id, cur_serial, cur_rpi, cur_path, cur_path_sz);

	if(!cur_connection) {
		zlog_error("[%s] ConnectPLCOverCNET() failed!\n\n",cur_device->sname);
		cur_device->status = STATUS_COMFAIL;
		return -1;
	} else {
		zlog_debug("ConnectPLCOverCNET() succeeded!\n");
	}

	// Set device vars and status information...
	cur_device->eip_session = cur_session;
	cur_device->eip_con = cur_connection;
	cur_device->status = STATUS_READY;
	cur_device->connect_count++;
	cur_device->retry_count = 0;
	if(cur_device->connect_count > 1) set_target_msg(cur_device,"OK! (Reconnect count = %i)",cur_device->connect_count);

	zlog_debug("eip_start() complete! Connection ready.\n");
	return 0;
}

int eip_stop(ATLAS_TARGET* cur_device) {

	int res;

	zlog_debug("eip_stop(): Closing session...\n");

	if(cur_device->eip_con) {
		res = Forward_Close(cur_device->eip_con);
		zlog_debug("ForwardClose: %s\n",cip_err_msg);
	}

	if(cur_device->eip_session) {
		UnRegisterSession(cur_device->eip_session);
		zlog_debug("UnRegisterSession: %s\n",cip_err_msg);
	}

	cur_device->eip_con = NULL;
	cur_device->eip_session = NULL;
	cur_device->status = STATUS_NOTREADY;

	return 0;
}

void* eip_readtag(char* tagname, ATLAS_TARGET* cur_device, int* dtype_ret) {
	static char tempreg[32];
	static int  txnum = 0;

	static union {
		int  v_int;
		float v_float;
	} rval;

	float vfloat;
	int vint;
	LGX_Read *rdata;
	PLC_Read *pdata;

	if(cur_device->status != STATUS_READY) {
		zlog_error("eip_readtag(): Target device is not ready!\n");
		zlog_error("eip_readtag(): Attempting to re-establish connection...\n");
		if(!eip_start(cur_device)) {
			zlog_info("eip_readtag(): Connection re-established OK!\n");
		} else {
			zlog_error("eip_readtag(): Connection failed. Will try next time.\n");
			eip_readerr = 1;
			return -1;
		}
	}

	// Retrieve tag value from target
	zlog_debug("eip_readtag: Reading \"%s\"...\n",tagname);
	if(cur_device->target_type == TARGET_LGX) {
		rdata = ReadLgxData(cur_device->eip_session,cur_device->eip_con,tagname,1);
		if(!rdata) {
			zlog_error("[%s:%s] ReadLgxData() failed to read from target! %s (%i : %i)\n",cur_device->sname, tagname, cip_err_msg,cip_errno,cip_ext_errno);
			set_target_msg(cur_device,"[%s] Failed to read tag from target. [%s] (%i:%i)",tagname,cip_err_msg,cip_errno,cip_ext_errno);

			cur_device->status = STATUS_COMFAIL;
			eip_readerr = 1;
			if(eip_autorcx && (eip_rcxattempt < eip_maxattempts)) {
				zlog_error("[%s] Attempting to reconnect to target... [Attempt %i]\n",cur_device->sname,eip_rcxattempt+1);
				// dump current connection
				eip_stop(cur_device);
				// increment connection ID and serial
				cur_device->conn_id++;
				cur_device->conn_serial++;
				// reconnect...
				eip_start(cur_device);
				// increment attempt counter
				eip_rcxattempt++;
				// now retry eip_readtag() call...
				return eip_readtag(tagname,cur_device,dtype_ret);
			} else {
				cur_device->status = STATUS_COMFAIL;
				return -1;
			}
		} else {
			eip_rcxattempt = 0;
			zlog_debug("ReadLgxData() OK! type[%d] varcount[%d] totalsize[%d] elementsize[%d] mask[0x%08x]\n",
		        	   rdata->type,rdata->Varcount,rdata->totalsize,rdata->elementsize,rdata->mask);
		}
	} else {
		pdata = ReadPLCData(cur_device->eip_session,cur_device->eip_con,NULL,NULL,0,cur_device->target_type,txnum++,tagname,1);
	}

	// Retrieve multiple values, if necessary...
	for(int i = 0; i < rdata->Varcount; i++) {
		vfloat = _GetLGXValueAsFloat(rdata,i);
		vint   = _GetLGXValueAsInteger(rdata,i);
		sprintf(tempreg,"%d",vint);
		zlog_debug("eip_readtag: index[%i] = vfloat[%0.04f] vint[%d]\n",i,vfloat,vint);
		switch(rdata->type) {
			case EIP_DTYPE_BOOL:
				rval.v_int = vint;
				if(dtype_ret) (*dtype_ret) = DTYPE_RET_BOOL;
				zlog_debug("eip_readtag: [EIP_DTYPE_BOOL] v_int = %i\n",rval.v_int);
				break;
			case EIP_DTYPE_ASC:
			case EIP_DTYPE_INT:
				rval.v_int = vint;
				if(dtype_ret) (*dtype_ret) = DTYPE_RET_INT;
				zlog_debug("eip_readtag: [EIP_DTYPE_INT] v_int = %i\n",rval.v_int);
				break;
			case EIP_DTYPE_FLP:
				rval.v_float = vfloat;
				if(dtype_ret) (*dtype_ret) = DTYPE_RET_FLOAT;
				zlog_debug("eip_readtag: [EIP_DTYPE_FLP] v_float = %f\n",rval.v_float);
				break;
			default:
				if(dtype_ret) (*dtype_ret) = 0;
				zlog_error("eip_readtag: unknown data type! type = %i\n",rdata->type);
		}
	}

	// Data read by ReadXXXData must be free'd!
	free(rdata);

	eip_readerr = 0;
	return &rval.v_int;
}

