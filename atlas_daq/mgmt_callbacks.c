/*
	Atlas Project - Data Acquisition Daemon (DAQ)
	J. Hipps - http://jhipps.org/ (jhipps@nichiha.com)
	Nichiha USA, Inc.

	Management backend - user command callbacks

	Copyright (c) 2013 Jacob Hipps/Nichiha USA, Inc.
	Version 0.03

	22 Jun 2013 - Started
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "atlas_daq.h"

/********************************************************************
Callback prototype:

	int mgmtcb_CMDNAME(char** cargs, int argcnt)
	cargs = Argument list; 2D array (array of string pointers)
	argcnt = Argument count
********************************************************************/


int mgmtcb_status(char* cargs, int argcnt) {
	ATLS_DEBUG_LOGFUNC();
	AMF_printf("%s EXEC OK\n\n",__func__);
	return 0;
}

int mgmtcb_reload(char* cargs, int argcnt) {
	ATLS_DEBUG_LOGFUNC();
	AMF_printf("%s EXEC OK\n\n",__func__);
	//zlog_debug("mgmtcb_reload(): Dropping all connections and reloading target & tag information...\n");
	return 0;
}

int mgmtcb_tag_read(char* cargs, int argcnt) {
	ATLS_DEBUG_LOGFUNC();
	AMF_printf("%s EXEC OK\n\n",__func__);
	return 0;
}

int mgmtcb_tag_add(char* cargs, int argcnt) {
	ATLS_DEBUG_LOGFUNC();
	AMF_printf("%s EXEC OK\n\n",__func__);
	return 0;
}
