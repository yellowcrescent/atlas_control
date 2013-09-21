/*

	Atlas Project - Data Acquisition Daemon (DAQ)
	J. Hipps - http://jhipps.org/ (jhipps@nichiha.com)
	Nichiha USA, Inc.

	Config File Reader / Register auto-import for MELSEC PLCs
	Copyright (c) 2013 Jacob Hipps/Nichiha USA, Inc.

	2013.07.03 - Started

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "atlas_daq.h"
#include "melsec.h"


typedef struct {
	char file_sig[4];
	char file_title[32];
	char dontcare[28];
	unsigned int dlist_size;
	unsigned int unknown1;
	unsigned short int dlist_num;
} MC_WCD_HEADER;

typedef struct {
	char dtype;
	char dtype2;
	unsigned int devnum;
	unsigned int dcount;
} MC_WCD_LENT;

typedef struct {
	char dalias[8];
	char dcomment[32];
} MC_WCD_STRTAB;


char* mc_get_dev_from_val(unsigned char val) {
	int matched = -1;

	for(int i = 0; i < 100; i++) {
		if(regmap[i].bval == NULL && regmap[i].cval == NULL) break;
		if(val == regmap[i].bval) {
			matched = i;
			break;
		}
	}
	
	if(matched == -1) return NULL;
	else return regmap[matched].cval;
}

int melsec_read_wcd(char* fname) {
	FILE* fhand;
	MC_WCD_HEADER thead;
	MC_WCD_LENT tent;
	MC_WCD_STRTAB ttab;

	if((fhand = fopen(fname,"rb")) == NULL) {
		zlog_error("melsec_read_wcd(): Failed to open file [%s]\n",fname);
		return 1;
	}

	zlog_debug("melsec_read_wcd(): Opened file successfully! Parsing...\n");

	// get header
	if(!fread(&thead,sizeof(thead),1,fhand)) {
		zlog_debug("failed to read header!\n");
		return 1;
	}

	zlog_debug(">> dlist_size = %u bytes / dlist_num = %hu\n",thead.dlist_size,thead.dlist_num);

	// read listing
	for(int i = 0; i < thead.dlist_num; i++) {
		if(!fread(&tent,sizeof(tent),1,fhand)) {
			zlog_debug("failed to read list entry!\n");
			return 1;
		}

		zlog_debug("[%i] %s%u -> # %u\n",i,mc_get_dev_from_val(tent.dtype),tent.devnum,tent.dcount);
	}

	// read string table
	fclose(fhand);
	return 0;
}
