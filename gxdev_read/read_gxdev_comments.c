/*****************************************************************************
 *
 * Atlas Project - Mitsubishi MELSEC GX Developer File Parser
 * 
 * Global Makefile
 * Copyright (c) 2013 J. Hipps/Neo-Retro Group LLC., All Rights Reserved.
 *
 * @author-mail:	tetrisfrog@gmail.com
 * @group-url:		http://neoretro.net/
 * @prog-url:		http://neoretro.net/progs/jhipps/gxdev_read
 *
 * @started:		2013.07.03
 *
 *
 * Licensed under GNU LGPLv3 
 * Located online via: [http://www.gnu.org/licenses/lgpl-3.0.txt]
 *
 * @disclaimer:
 *
 * USER ACKNOWLEDGES THAT INITIAL SETUP, OPERATION & USE, TROUBLESHOOTING,
 * AND REPORTING OF SOFTWARE DEFECTS ("BUGS") ARE THEIR SOLE RESPONSIBILITY.
 *
 * ABSOLUTELY NO WARRANTY, EXPRESSED OR IMPLIED, INCLUDING USE AND EFFECTIVENESS
 * FOR A PARTICULAR PURPOSE.
 *
 * ATLAS SYSTEM (INCLUDING HARDWARE, SOFTWARE AND/OR IP CORES) ARE NOT DESIGNED
 * OR INTENDED TO BE FAIL-SAFE, OR FOR USE IN ANY APPLICATION REQUIRING FAIL-
 * SAFE PERFORMANCE, SUCH AS LIFE-SUPPORT OR SAFETY DEVICES OR SYSTEMS,
 * CLASS III MEDICAL DEVICES, NUCLEAR FACILITIES, APPLICATIONS RELATED TO THE
 * DEPLOYMENT OF AIRBAGS, OR ANY OTHER APPLICATIONS THAT COULD LEAD TO DEATH,
 * PERSONAL INJURY OR SEVERE PROPERTY OR ENVIRONMENTAL DAMAGE (INDIVIDUALLY
 * AND COLLECTIVELY KNOWN AS “CRITICAL APPLICATIONS”).
 *
 * FURTHERMORE, ATLAS SYSTEM/NEO-RETRO GROUP LLC PRODUCTS ARE NOT DESIGNED
 * OR INTENDED FOR USE IN ANY APPLICATIONS THAT AFFECT CONTROL OF A VEHICLE
 * OR AIRCRAFT, UNLESS THERE IS A FAIL-SAFE REDUNDANCY FEATURE (WHICH DOES
 * NOT INCLUDE USE OF SOFTWARE TO IMPLEMENT THE REDUNDANCY) AND A WARNING
 * SIGNAL UPON FAILURE TO THE OPERATOR.
 *
 * USER AGREES, PRIOR TO USING OR DISTRIBUTING ANY SYSTEMS THAT RUN
 * ATLAS-RELATED SOFTWARE FOR-- DATA ACQUISITION, MONITORING, REPORTING,
 * PRODUCT TRACKING, INTERNAL PARTS INVENTORY MANAGEMENT, DOWNTIME MANGEMENT,
 * EQUIPMENT AND DEVICE DOCUMENTATION STORAGE AND RETRIEVAL, AND ANY OTHER
 * MISCELLANEOUS OR AUXIALLARY FUNCTIONS COVERED-- THAT THE USER AND OPERATOR
 * (INCLUDING COMPANY OR CORPORATION) FIRST HAVE THE SYSTEM FULLY SETUP,
 * TESTED, AND VERIFIED TO BE FUNCTIONING PROPERLY, AS IS INDICATED IN
 * THE DETAILED LOG RESULTS OF ATLAS SOFTWARE.
 *
 * IN USING ANY PART OF THIS SOFTWARE, INCLUDING THE DATA ACQUISITION DAEMON
 * (atlas_daq), ET. AL., THE CUSTOMER ASSUMES THE SOLE RISK AND LIABILITY
 * OF ANY USE  IN CRITICAL APPLICATIONS, SUBJECT ONLY TO APPLICABLE LAWS
 * AND REGULATIONS GOVERNING LIMITATIONS ON PRODUCT LIABILITY.
 *
 * CRITICAL APPLICATIONS: THIS SOFTWARE IS NOT TYPED, EVALUATED, DESIGNED,
 * OR RATED FOR ANY TYPE OF APPLICATIONS TERMED AS "CRITICAL", WHICH INCLUDE
 * BUT MAY NOT BE LIMITED TO: LIFE SUPPORT SYSTEMS, AVIATION/AVIONICS
 * SYSTEMS, ANY MACHINE OR DEVICE WHICH HAS THE CAPACITY TO HARM, MAIM,
 * OR KILL A HUMAN BEING. SUCH USE IS NOT SUPPORTED BY NEO-RETRO GROUP,
 * LLC, AND IN FACT MAY BE ILLEGAL DEPENDING ON THE LOCAL LAWS OF YOUR
 * JURISDICTION.
 *
 * ALL PERSONELL SHOULD STAY CLEAR OF ANY EQUIPMENT WHICH IS TARGETED BY
 * THIS SOFTWARE AT ALL TIMES WHENEVER HARDWARE SAFETY INTERLOCKS ARE
 * ENGAGED, FOR EXAMPLE, THE MASTER CONTROL RELAY IS ENERGIZED AND CAN
 * WORK AUTOMATICALLY AND UNEXPECTEDLY AT ANY MOMENT!
 * (NOTE THAT "SOFT" EMERGENCY STOP BITS AND SIMPLY PUTTING
 * A MACHINE IN ITS "STOP" MODE IS INSUFFICIENT PROTECTION. FURTHERMORE,
 * THE MACHINE MAY EVEN BE REQUIRED TO BE LOCKED OUT IF YOU ARE REQUIRED
 * TO ENTER-- WITH ANY PORTION OF YOUR BODY-- THE MACHINE'S POINT(S) OF
 * OPERATION IN ACCORDANCE LOCAL AND NATIONAL GUIDELINES SUCH AS OSHA/NAMSHA
 * IN THE UNITED STATES, CANOSH IN CANADA, HSE/HSENI IN THE UK, AND
 * EU-OSHA FOR EURO-ZONE MEMBER COUNTRIES.
 *
 * AN UNLIKELY EVENT OR BUG IN THE SOFTWARE MAY ALSO CAUSE A PSUEDO-RANDOM
 * VALUE TO BE WRITTEN AT A PARTICULAR REGISTER OR TAG ADDRESS OF ANY ONE OF
 * THE TARGET PLCs. USER, OPERATOR, AND ADMINISTRATOR MUST ACCEPT THIS
 * POSSIBILITY IN ORDER TO PROCEED WITH OPERATION OF THE FULL ATLAS SYSTEM
 * SOFTWARE SUITE.
 *
 * IN SUMMARY: Although Atlas System components (such as atlas_daq) are
 * tested rigorously and contain extensive debugging features, we cannot
 * guarantee its safety in situations where human lives may be in
 * jeopardy. And as such, prohibit its use to only industrial, commercial,
 * home/office/personal, and testing purposes. Educational institutions
 * such as Primary or High Schools are good candidates for Atlas'
 * expansive and rich reporting features combined with massive fast 
 * storage power provided by a hybrid database schema, however, we ask
 * that such educational institutions please fully complete and return
 * a Record of Installation form [FRMx-NRG-ATLS-INST]
 *
 * All users which are currently employed by, contracted by, or operated
 * under the government of the United States of America, including all
 * divisions of the Armed Forces and U.S. Department of Defense, or any other
 * Federally or State-run institution will
 * also be require to provide a Record of Installation [FRMx-NRG-ATLS-INST]
 *
 
PRODUCTS INDICATED AS "COMMERCIAL ENGINEERING SAMPLE (CES)" OR "BETA-TEST" HARD- AND SOFTWARE ARE EXPLICITLY FREE OF ANY WARRANTY AND NO CLAIMS CAN BE MADE AGAINST SCIENGINES GMBH. "DEVELOPMENT" HARDWARE EXPLICITLY CARRIES A REDUCED WARRANTY OF 14 DAYS ONLY, AS SET FORTH IN THE "SCIENGINES LIMITED WARRANTY FOR DEVELOPMENT SYSTEMS" SECTION.


 *****************************************************************************/
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
