/*
	Atlas Project - Data Acquisition Daemon (DAQ)
	J. Hipps - http://jhipps.org/ (jhipps@nichiha.com)
	Nichiha USA, Inc.

	Management Pipe FIFO Handler

	Copyright (c) 2013 Jacob Hipps/Nichiha USA, Inc.
	Version 0.03

	22 Jun 2013 - Started
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <errno.h>
#include "atlas_daq.h"

#define ATLS_DBUFF_SIZE					1024			// size of data buffer in atlas_mgmt_fifo_listen()
#define ATLS_MGMT_CMD_STRSZ				32				// size of command buffer string
#define ATLS_MGMT_ARG_STRSZ				256				// size of each arg string in argument list buffer
#define ATLS_MGMT_MAX_ARGS				16				// max number of arguments, size of arglist buffer

ATLAS_FIFO mgmt_fifo_in;
ATLAS_FIFO mgmt_fifo_out;

/*

// Management console command flags for parser (ATLAS_MGMT_CMD.flags)
#define MGMTC_NORMAL	0
#define MGMTC_NOARGS	1 		// No args
#define MGMTC_NOSPLIT	2 		// Don't split argument string
#define MGMTC_SLESCAPE	4		// Use backslash (\) for escaping (default = no escape char)
#define MGMTC_UCESCAPE	8		// Use caret (^) for escaping

		typedef struct {
			char cmdtok[32];
			int flags;
			int (*ccback)(char** cargs, int argcnt);
		} ATLAS_MGMT_CMD;
*/

ATLAS_MGMT_CMD mgmt_cmddex[] = {
	{"status",			MGMTC_NOARGS,				&mgmtcb_status },
	{"reload",			MGMTC_NORMAL,				&mgmtcb_reload },
	{"tag_read",		MGMTC_NORMAL,				&mgmtcb_tag_read },
	{"tag_add",			MGMTC_NORMAL,				&mgmtcb_tag_add },
	{NULL, 0, NULL}
};


int atlas_mgmt_fifo_init(char* pipe_path) {
	struct stat atr_in;
	struct stat atr_out;
	int srv_in, srv_out;

	ATLS_FENTER();

	mgmt_fifo_in.status = STATUS_NOTREADY;
	mgmt_fifo_out.status = STATUS_NOTREADY;
	ATLS_ASSERT_NULLPTR(pipe_path,-1);

	// copy pipe path
	strcpy(mgmt_fifo_in.fifo_path, pipe_path);
	strcpy(mgmt_fifo_out.fifo_path, pipe_path);
	strcat(mgmt_fifo_in.fifo_path,"_in");
	strcat(mgmt_fifo_out.fifo_path,"_out");

	zlog_debug("atlas_mgmt_init(): Initializing Management FIFOs [in=%s/out=%s] ...\n ",mgmt_fifo_in.fifo_path,mgmt_fifo_out.fifo_path);

	// check to see if fifos already exist...
	// In
	if((srv_in = stat(mgmt_fifo_in.fifo_path, &atr_in)) != 0) {
		zlog_error("atlas_mgmt_fifo_init(): FIFO-IN file does not exist. Creating.\n");
		if(mkfifo(mgmt_fifo_in.fifo_path, 0666)) {
			zlog_error("mkfifo() failed to create FIFO-IN file (errno = %i) [%s].\n",errno, mgmt_fifo_in.fifo_path);
			return -1;
		} else {
			zlog_debug("FIFO-IN created OK!\n");
		}
	} else {
		if(!S_ISFIFO(atr_in.st_mode)) {
			zlog_error("FIFO-IN file already exists as a non-FIFO file type! [%s]\n",mgmt_fifo_in.fifo_path);
			return -1;
		}
	}
	// Out
	if((srv_out = stat(mgmt_fifo_out.fifo_path, &atr_out)) != 0) {
		zlog_error("atlas_mgmt_fifo_init(): FIFO-OUT file does not exist. Creating.\n");
		if(mkfifo(mgmt_fifo_out.fifo_path, 0666)) {
			zlog_error("mkfifo() failed to create FIFO-OUT file (errno = %i) [%s].\n",errno, mgmt_fifo_in.fifo_path);
			return -1;
		} else {
			zlog_debug("FIFO-OUT created OK!\n");
		}
	} else {
		if(!S_ISFIFO(atr_out.st_mode)) {
			zlog_error("FIFO-OUT file already exists as a non-FIFO file type! [%s]\n",mgmt_fifo_out.fifo_path);
			return -1;
		}
	}

	// get handle/open the FIFOs
	// In (Rx)
	if((mgmt_fifo_in.fifoHandle = open(mgmt_fifo_in.fifo_path, O_RDONLY | O_NONBLOCK)) == 0) {
		zlog_error("atlas_mgmt_init(): open() failed for mgmt_fifo_in!\n");
		mgmt_fifo_in.status = STATUS_INITFAIL;
		return -1;
	}
	// Out (Tx)
	if((mgmt_fifo_out.fifoHandle = open(mgmt_fifo_out.fifo_path, O_WRONLY | O_NONBLOCK)) == 0) {
		zlog_error("atlas_mgmt_init(): open() failed for mgmt_fifo_out!\n");
		mgmt_fifo_out.status = STATUS_INITFAIL;
		return -1;
	}

	// FIFO setup & ready!
	mgmt_fifo_in.status = STATUS_READY;
	mgmt_fifo_out.status = STATUS_READY;

	zlog_debug("Mgmt FIFOs created successfully!\n");

	ATLS_FLEAVE();
	return 0;
}

int atlas_mgmt_fifo_close() {
	ATLS_FENTER();
	if(mgmt_fifo_in.status) {
		close(mgmt_fifo_in.fifoHandle);
		mgmt_fifo_in.status = STATUS_NOTREADY;
		zlog_debug("atlas_mgmt_fifo_close(): Management FIFO-IN closed.\n");
	}

	if(mgmt_fifo_out.status) {
		close(mgmt_fifo_out.fifoHandle);
		mgmt_fifo_out.status = STATUS_NOTREADY;
		zlog_debug("atlas_mgmt_fifo_close(): Management FIFO-OUT closed.\n");
	}

	ATLS_FLEAVE();
	return 0;
}

int atlas_mgmt_fifo_listen() {
	int rv = 0;
	char dbuff[ATLS_DBUFF_SIZE];

	ATLS_FENTER();

	// ensure the FIFO is open & ready
	if(mgmt_fifo_in.status != STATUS_READY) return -1;

	// check it for new data...
	if((rv = read(mgmt_fifo_in.fifoHandle, dbuff, ATLS_DBUFF_SIZE - 1)) > 0) {
		zlog_debug("atlas_mgmt_fifo_listen(): got %i bytes from FIFO!\n",rv,dbuff);
		atlas_mgmt_parse(dbuff,rv);
		return 1;
	}

	ATLS_FLEAVE();
	return 0;
}

int atlas_mgmt_fifo_tx(char* odat, int dsz) {
	int rrv = 0;

	ATLS_FENTER();
	ATLS_ASSERT_NULLPTR(odat,-1);
	ATLS_ASSERT_NONPOS(dsz,-1);

	if(mgmt_fifo_out.status == STATUS_READY) {

		if((rrv = write(mgmt_fifo_out.fifoHandle,odat,dsz)) == 0) {
			zlog_error("atlas_mgmt_fifo_tx(): Failed to write %i bytes to management FIFO!\n",dsz);
			return -1;
		}

		return rrv;
	}

	ATLS_FLEAVE();
	return -1;
}

int atlas_mgmt_parse(char* cbuff, int bufsz) {
	char cc;				// current char
	char lc = 0;			// last char
	int acx = 0;			// current arg char index
	int esc_active = 0;		// used for quoted strings
	int argcnt = 0;			// arg counter (0 = command name, 1 = arglist[0])
	int xrec = 0;			// recording

	int cmatch = -1;		// cmd index from match
	int cflags = 0;			// cmd flags from command LUT

	int (*cmd_cback)(char*,int);	// pointer to command callback function
	int exec_rv;			// return val from callback func

	char escaper = 0;

	char arglist[ATLS_MGMT_MAX_ARGS][ATLS_MGMT_ARG_STRSZ];
	char cmdname[ATLS_MGMT_CMD_STRSZ];

	ATLS_FENTER();
	ATLS_ASSERT_NULLPTR(cbuff,-1);
	ATLS_ASSERT_NONPOS(bufsz,-1);

	// clear arglist memory
	memset(arglist,0,ATLS_MGMT_MAX_ARGS * ATLS_MGMT_ARG_STRSZ);

	// iterate through and parse the command name and argument strings
	// if the command does not exist, no further processing is done and atlas_mgmt_parse() returns (-2).
	// General or other fatal errors return (-1). On success, return value of callback function is returned.
	for(int i = 0; i <= bufsz; i++) {
		cc = cbuff[i];
		if(i > 0) lc = cbuff[i - 1];

		if(cc == '\'' || cc == '\"' || cc == '\n' || cc == '\t') {
			// check to see if we have two quotes together (to represent an actual quote mark instead of delimeter)
			// an empty quoted string doesn't count and is skipped
			if(cc == lc && acx > 0) {
				esc_active = 1;
				// ensure escape is active, then copy the quote to output string
			} else {
				if(esc_active) {
					esc_active = 0;
					continue;
				} else {
					esc_active = 1;
					continue;
				}
			}
		} else if((cc == ' ' || cc == '\t' || cc == '\n') && !esc_active && xrec && lc != escaper) {

			// ensure this arg wasn't somehow empty, if so, keep trying
			if(acx == 0) continue;

			// increment arg counter
			argcnt++;

			if(argcnt >= ATLS_MGMT_MAX_ARGS) {
				zlog_error("atlas_mgmt_parse(): Max number of arguments reached! [argcnt = %i] [ATLS_MGMT_MAX_ARGS = %i]\n",argcnt,ATLS_MGMT_MAX_ARGS);
				return -1;
			}

			// perform command-related stuff
			if(argcnt == 1) {
				if((cmatch = atlas_mgmt_cmdluk(cmdname)) == -1) {
					zlog_error("atlas_mgmt_parse(): Command \"%s\" not found!\n",cmdname);
					return -2;
				} else {
					cflags = mgmt_cmddex[cmatch].flags;
					cmd_cback = mgmt_cmddex[cmatch].ccback;

					if(cflags & MGMTC_NOARGS) {
						// Don't bother collecting args (ignore them if they exist)
						exec_rv = cmd_cback(NULL,0);
						return exec_rv;
					} else if(cflags & MGMTC_NOSPLIT) {
						// If MGMTC_NOSPLIT defined, pass whole arg string as one chunk via cbuff pointer
						exec_rv = cmd_cback((char*)(cbuff + acx), 0);
						return exec_rv;
					}

					if(cflags & MGMTC_UCESCAPE) {
						// Use caret for escaping chars
						escaper = '^';
					}
					if(cflags & MGMTC_SLESCAPE) {
						// Use backslash for esacping chars
						escaper = '\\';
					}
				}
			}

			acx = 0;	// reset char index

		} else if((cc == ' ' || cc == '\t') && !xrec && !escaper) {
			// skip whitespace
			continue;
		} else if(escaper && cc == escaper) {
			if(!esc_active) {
				esc_active = 2;
				continue;
			} else {
				esc_active = 0;
			}
		} else if(cc > 32 && cc < 127) {
			xrec = 1;
		}

		// copy valid char to arglist/cmdname
		if(!argcnt) {
			cmdname[acx] = cc;
			cmdname[acx+1] = 0; // append null terminator
		} else {
			arglist[argcnt - 1][acx] = cc;
			arglist[argcnt - 1][acx+1] = 0; // null terminator
		}


		// if parsing the command, convert to lowercase
		if(!argcnt && cc > 64 && cc < 91) cc += 32;

		acx++;		// increment index for current arg

		// ensure we haven't exceeded max limits
		if(!argcnt && acx > ATLS_MGMT_CMD_STRSZ) {
			zlog_error("atlas_mgmt_parse(): command string exceeded max size of 32 bytes!\n");
			return -1;
		} else if(argcnt && acx > ATLS_MGMT_ARG_STRSZ) {
			zlog_error("atlas_mgmt_parse(): argument %i exceeded max size of 256 bytes!\n",argcnt);
			return -1;
		}
	}

	// run command using callback function
	zlog_debug("");
	exec_rv = cmd_cback(arglist, argcnt);

	ATLS_FLEAVE();
	return exec_rv;
}

int atlas_mgmt_cmdluk(char* cmdchk) {
	char cc;
	int cmatch = -1;
	int clen = strlen(cmdchk);
	int cddex = 0;

	ATLS_FENTER();
	ATLS_ASSERT_NULLPTR(cmdchk,-1);

	zlog_debug("atlas_mgmt_cmdluk(): cmdchk = \"%s\" (clen = %i)\n",cmdchk,clen);

	while(cmatch == -1) {
		if(!mgmt_cmddex[cddex].cmdtok[0]) break;
		if(!strcmp(mgmt_cmddex[cddex].cmdtok, cmdchk)) {
			cmatch = cddex;
			break;
		}
		cddex++;
	}

	ATLS_FLEAVE();
	return cmatch;
}

void AMF_printf(char* fmt, ...) {
	char texbuf[4096];
	int bufsz = 0;

	ATLS_ASSERT_NULLPTR(fmt,);

	va_list ap;
	va_start(ap, fmt);
	vsprintf(texbuf, fmt, ap);
	va_end(ap);

	// get string length
	bufsz = strlen(texbuf);

	// write to FIFO
	atlas_mgmt_fifo_tx(texbuf,bufsz);

}
