/*

	Atlas Project - Data Acquisition Daemon (DAQ)
	J. Hipps - http://jhipps.org/ (jhipps@nichiha.com)
	Nichiha USA, Inc.

	MELSEC Communication Protocol (MC Protocol) Driver Implementation
	Copyright (c) 2012-2013 Jacob Hipps/Nichiha USA, Inc.

	2012.09.25 - Started.

	This driver implements "MC Protocol" communications as described
	in Mitsubishi "MELSEC-Q/L MELSEC Communication Protocol Reference
	Manual", in the following manuals (English & Japanese)...
	[en] SH(NA)-080008-Q - http://www.meau.com/
	[jp] SH-08003-V      - http://www.mitsubishielectric.co.jp/

	* Protocol Type 3E (binary) QnA compatible framing

*/

#define ATLS_DRIVER_MAIN
#define ATLS_DRIVER_MAIN_MELSEC

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "atlas_daq.h"
#include "melsec.h"

char* mc_errmsg(unsigned short errnum) {
	char* msgptr = NULL;

	for(int ez = 0; ez < 1024; ez++) {
		if(!errmap[ez].errval && !errmap[ez].errdesc[0]) break;
		if(errmap[ez].errval == errnum) {
			msgptr = errmap[ez].errdesc;
			return msgptr;
		}
	}

	return NULL;
}

int mc_start(ATLAS_TARGET* atag) {
	char p_ipaddr[24];
	int  p_port;
	int port_retries = 0;

	// allocate memory for mc_session struct
	if((atag->mc_session = malloc(sizeof(ATLAS_MCS))) == NULL) {
		zlog_error("CRITICAL: Failed to allocate memory for mc_session!\n\n");
		atlas_shutdown(254);
		return 0;
	}

	// Copy IP address to mc_session data
	strcpy(atag->mc_session->ip_addr,atag->ip_addr);

	// set to default port_num
	atag->port_num_active = atag->port_num;

	// Attempt to connect to target...
	while(atag->status != STATUS_READY && port_retries < 8) {
		// Copy port_num to mc_session
		atag->mc_session->port_num = atag->port_num_active;

		// increment retry count
		atag->retry_count++;

		zlog_debug("mc_start(): Connecting to \"%s\" at %s on port %u ...\n",atag->sname,atag->ip_addr,atag->port_num_active);
		if(!atlas_sock_connect(atag->mc_session)) {
			atag->status = STATUS_COMFAIL;
			atag->port_num_active++;
			zlog_error("mc_start(): Failed to connect to \"%s\" (%s:%u).\n",atag->sname,atag->ip_addr,atag->port_num_active);
			port_retries++;
		} else {
			atag->status = STATUS_READY;
		}		
	}

	if(atag->status == STATUS_READY) {
		zlog_debug("mc_start(): Connected to target. Ready!\n");
		atag->connect_count++;
		atag->retry_count = 0;
		if(atag->connect_count > 1) set_target_msg(atag,"OK. Reconnect count = %i",atag->connect_count);
	} else {
		zlog_error("mc_start(): Failed to connect!\n");
		set_target_msg("Connection failed. (retries = %i)",atag->retry_count);
		return -1;		
	}
	
	return 0;
}

void mc_stop(ATLAS_TARGET* atag) {

	zlog_debug("mc_stop(): Disconnecting from target. Freeing mc_session memory.\n");
	if(atag->mc_session) {
		atlas_sock_close(atag->mc_session);
		free(atag->mc_session);
		atag->mc_session = NULL;
	}

	atag->status = STATUS_NOTREADY;
}

//ATLAS_MC_3E_REQ* mc_dset_header_3e(ATLAS_MC_3E_REQ* hdr) {
void* mc_dset_header_3e(ATLAS_MC_3E_REQ* ahdr) {

	
	ahdr->subheader[0]  	= MC_3E_REQ_SIG;
	ahdr->subheader[1]	= 0x00;
	ahdr->network_no 	= MC_DEFAULT_NETWORK;
	ahdr->pc_no		= MC_DEFAULT_PC;
	ahdr->request_dest_io	= MC_DEFAULT_MODULE_IO;
	ahdr->request_dest_sta	= MC_DEFAULT_MODULE_STATION;
	ahdr->monitor_timer	= 0x0000;
	
	return ahdr;
}

int mc_decode_station(char* confstr, ATLAS_MC_3E_REQ* ahdr) {
	// Decodes a station address, such as "1-2" (Network 1, Station 2) and
	// sets the appropriate params within the header.
	// Also accepts full spec with "1-2-3-36400" (Network, Station, Destinition Station, Destination I/O)
	// All numbers must be decimal integers and not in hex!

	int clen;

	// sanity check
	if(!confstr) {
		//zlog_error("[assert fail] NULL pointer passed as string argument!\n");
		return 100;
	} else if(!ahdr) {
		zlog_error("[assert fail] NULL pointer passed as header ref!\n");
		return 100;
	}

	clen = strlen(confstr);

	char cc;
	char cbuf[4][8];
	int cbuf_len = 0;
	int zstate = 0;

	for(int i = 0; i < clen; i++) {
		cc = confstr[i];
		if(cc == ' ') continue;
		if(cc == '-' || cc == '.' || cc == ',' || cc == ':' || cc == '/') {
			zstate++;
			cbuf_len = 0;
			if(zstate > 3) break;
			continue;
		}
		cbuf[zstate][cbuf_len] = cc;
		cbuf_len++;
		if(cbuf_len > 7) {
			zlog_error("Invalid station spec! [%s]\n",confstr);
			return 1;
		}
		cbuf[zstate][cbuf_len] = 0x00;
	}

	// "Net:Sta" format
	if(zstate >= 1) {
		ahdr->network_no = (unsigned char)atoi(cbuf[0]);
		ahdr->pc_no = (unsigned char)atoi(cbuf[1]);	
	}

	// "Net:Sta:DestSta" format
	if(zstate >= 2) {
		ahdr->request_dest_sta = (unsigned char)atoi(cbuf[2]);
	}

	// "Net:Sta:DestSta:DestIO" long format
	if(zstate == 3) {
		ahdr->request_dest_io = (unsigned short)atoi(cbuf[3]);
	}

	zlog_debug("mc_decode_station(): Network = %hhu / Station = %hhu / Dest Station = %hhu / Dest I/O = 0x%04lX (%hu)\n",ahdr->network_no,ahdr->pc_no,ahdr->request_dest_sta,ahdr->request_dest_io,ahdr->request_dest_io);

	return 0;
}

int mc_decode_device(char* devstr, unsigned char* dcode, int* dnum) {
	// Takes a string such as "M5120" or "ZR1244" and returns
	// the device number and device type code

	char dcd[32];
	int  dcn;
	int  matched = -1;

	zlog_debug("mc_decode_device(): Decoding \"%s\"...\n",devstr);
	if(sscanf(devstr,"%c%u",dcd,&dcn) == 2) {
		dcd[1] = 0;
	} else if(sscanf(devstr,"%2c%u",dcd,&dcn) == 2) {
		dcd[2] = 0;
	} else {
		zlog_error("mc_decode_device(): Failed to decode device \"%s\"!\n",devstr);
		return 0;
	}

	// Loop through and check against regmap values
	for(int i = 0; i < 100; i++) {
		if(regmap[i].bval == NULL && regmap[i].cval == NULL) break;
		if(!strcmp(dcd,regmap[i].cval)) {
			matched = i;
			break;
		}
	}
	
	if(matched == -1) return 0;

	// Set return values
	(*dcode) = regmap[matched].bval;
	(*dnum)  = dcn;
	zlog_debug("mc_decode_device(): Decode OK. dcode[0x%hhX] dnum[%u]\n",regmap[matched].bval,dcn);

	return 1;
}

int mc_batch_read(char* devname, ATLAS_TARGET* atag, void* outbuf, unsigned short seq) {
	ATLAS_MC_3E_REQ request_header;
	ATLAS_MC_3E_ACK resp_header;
	ATLAS_MC_BATCHRW read_req;
	unsigned char dev_code;
	int head_dev;
	char tx_buf[128];
	char rx_buf[4096];
	unsigned short qcontent_sz = 12; // Q content size is always 12 bytes for Tx
	int tx_sz = MC_3E_HEADER_SZ + qcontent_sz;
	int rx_sz = 0;
	unsigned short rez_datalen = 0;
	unsigned short rez_wordlen = 0;
	unsigned short cdatabuf[1024];

	// Setup header with defaults
	mc_dset_header_3e(&request_header);

	// Setup station params
	if(mc_decode_station(atag->path_str, &request_header)) {
		set_target_msg(atag,"[%s] Failed to decode station spec! [%s]\n",devname,atag->path_str);
		return 0;
	}

	// Set command
	request_header.command = 0x0401;	// Batch Read

	// Setup command area (read request)
	read_req.subcommand = 0x0000;		// Response data type = Word [0000]
						// [0001] = Point
	// Device Type
	if(!mc_decode_device(devname, &dev_code, &head_dev)) {
		set_target_msg(atag,"Failed to decode device \"%s\"",devname);
		return 0;
	}
	read_req.dev_type = dev_code;

	// Set content length
	request_header.data_length = qcontent_sz;

	// Copy head device number (24-bit integer)
	memcpy(read_req.head_device, &head_dev, 3);

	// Specify number of points
	read_req.num_points = seq;

	// Copy data to transmit buffer
	memcpy(tx_buf,&request_header,sizeof(request_header));
	memcpy(tx_buf+sizeof(request_header),&read_req,sizeof(read_req));

	// Tx
	if(atlas_sock_send(atag->mc_session, tx_buf, tx_sz) != tx_sz) {
		zlog_error("mc_batch_read(): Data send error!\n");
		set_target_msg(atag,"[%s] mc_batch_read(): Data send error!\n",devname);
		return 0;
	}

	// Rx
	if((rx_sz = atlas_sock_recv(atag->mc_session, rx_buf, 4096)) <= 0) {
		zlog_error("mc_batch_read(): No data received!\n");
		set_target_msg(atag,"[%s] mc_batch_read(): No data received!\n",devname);
		return 0;
	}

	zlog_debug("mc_batch_read(): Got %i bytes!\n",rx_sz);

	memcpy(&resp_header,rx_buf,sizeof(resp_header));

	zlog_debug("mc_batch_read(): net[0x%02hhX] pc[0x%02hhX] reqdest_io[0x%04hX] datalen[%hu bytes / 0x%04hX] ccode[0x%04hX]\n",resp_header.network_no,resp_header.pc_no, resp_header.request_dest_io, resp_header.data_length, resp_header.data_length, resp_header.complete_code);

	//if(resp_header.subheader[0] == MC_3E_RSP_SIG) {
	if(resp_header.complete_code == 0x0000) {
		rez_datalen = resp_header.data_length - 2; // subtract 2 from data length to account for response code (word)
		rez_wordlen = rez_datalen / 2;
		zlog_debug("mc_batch_read(): Char data len = %u bytes (%i words).\n",rez_datalen, rez_wordlen);
		if(outbuf > 0) {
			memcpy(&cdatabuf, rx_buf+sizeof(resp_header), rez_datalen);
			memcpy(outbuf, rx_buf+sizeof(resp_header), rez_datalen);
			zlog_debug(">> outbuf[0] = 0x%04hX\n",cdatabuf[0]);
			return rez_wordlen;
		} else {
			zlog_error("mc_batch_read(): Invalid outbuf pointer!\n");
			set_target_msg(atag,"[%s] mc_batch_read(): Invalid outbuf pointer!\n",devname);
		}
	} else {
		zlog_error("mc_batch_read(): Abnormal completion. [%04hX] %s\n",resp_header.complete_code,mc_errmsg(resp_header.complete_code));
		set_target_msg(atag,"[%s] mc_batch_read(): Abnormal response: [0x%04hX] %s",devname,mc_errmsg(resp_header.complete_code));
		return -1;
	}
	//} else {
	//	zlog_error("mc_batch_read(): Invalid response subheader signature! Expected 0x%02hhX / Rcvd 0x%02hhX\n",MC_3E_RSP_SIG,resp_header.subheader[0]);
	//	return -1;
	//}

	return -1;
}

void* mc_readword(char* devname, ATLAS_TARGET* atag) {
	unsigned short zword = 0;
	static volatile unsigned int iword = 0;

	if(atag->status != STATUS_READY) {
		zlog_error("[%s] Target not ready!\n",atag->sname);
		zlog_error("[%s] Attempting to reconnect...\n",atag->sname);
		mc_stop(atag);
		mc_start(atag);
		if(atag->sname != STATUS_READY) {
			zlog_error("[%s] Target is still not ready! Will try again next time...\n",atag->sname);
			return -1;
		} else {
			zlog_info("[%s] Connection re-established successfully!\n",atag->sname);
		}
	}

	if(mc_batch_read(devname, atag, &zword, 1) != 1) {
		zlog_error("mc_readword(): batch read failed.\n");
		return -1;
	}

	// perform conversion from word (unsigned short) to dword (int)
	iword = zword;

	return &iword;
}

int mc_readbit(char *devname, ATLAS_TARGET* atag) {
	int outbit;
	int* xword;

	if((xword = mc_readword(devname, atag)) == -1) {
		return -1;
	}
	
	outbit = (*xword) & 0x0001;

	return outbit;
}