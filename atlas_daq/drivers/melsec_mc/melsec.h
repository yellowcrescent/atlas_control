/*

	Atlas Project - Data Acquisition Daemon (DAQ)
	J. Hipps - http://jhipps.org/ (jhipps@nichiha.com)
	Nichiha USA, Inc.

	MELSEC Communication Protocol (MC Protocol) Driver Implementation
	Header File
	Copyright (c) 2013 Jacob Hipps/Nichiha USA, Inc.

	2013.07.03 - Started

	This driver implements "MC Protocol" communications as described
	in Mitsubishi "MELSEC-Q/L MELSEC Communication Protocol Reference
	Manual", in the following manuals (English & Japanese)...
	[en] SH(NA)-080008-Q - http://www.meau.com/
	[jp] SH-08003-V      - http://www.mitsubishielectric.co.jp/

	* Protocol Type 3E (binary) QnA compatible framing

*/

#define MC_3E_REQ_SIG				0x50
#define MC_3E_RSP_SIG				0xD0

#define MC_DEFAULT_NETWORK			0x00
#define MC_DEFAULT_PC				0xFF
#define MC_DEFAULT_MODULE_IO		0x03FF
#define MC_DEFAULT_MODULE_STATION	0x00

#define MC_3E_HEADER_SZ				9

/*
	subheader		2 bytes \
	network_no		1 byte   \
	pc_no			1 byte    \_ 9 bytes
	request_dest_io		2 bytes   /
	request_dest_sta	1 byte   /
	data_length		2 bytes /
	monitor_timer		2 bytes \___ 4 bytes
	command			2 bytes /
*/

// MC Frame Header - Type 3E/Binary - Request
typedef struct {
	char subheader[2];			// subheader value
	unsigned char network_no;		// network/station number
	unsigned char pc_no;			// PC number
	unsigned short request_dest_io;		// request destination module i/o number
	unsigned char request_dest_sta;		// request destination module station number
	unsigned short data_length;		// length of request data section
	unsigned short monitor_timer;		// CPU monitoring timer
	unsigned short command;			// command
} ATLAS_MC_3E_REQ;

// MC Frame Header - Type 3E/Binary - Response
typedef struct {
	char subheader[2];			// subheader value
	unsigned char network_no;		// network/station number
	unsigned char pc_no;			// PC number
	unsigned short request_dest_io;		// request destination module i/o number
	unsigned char request_dest_sta;		// request destination module station number
	unsigned short data_length;		// length of response data
	unsigned short complete_code;		// completion code
} ATLAS_MC_3E_ACK;

// Type 3E/Binary - Abnormal Response structure
typedef struct {
	unsigned char rsp_network;		// responder network/station number
	unsigned char rsp_pc;			// responder PC number
	unsigned short request_dest_io;		// request destination module i/o number
	unsigned char request_dest_sta;		// request destination module station number
	unsigned short command;			// failed command
	unsigned short subcommand;		// failed subcommand
} ATLAS_MC_3E_ABNORMAL;

// Batch Read/Write - Binary [0401/1401]
// Subcommands: [00x0] Word, [00x1] Point
typedef struct {
	unsigned short subcommand;		// subcommand
	char head_device[3];			// head device code (3 byte integer)
	unsigned char  dev_type;		// device type code
	unsigned short num_points;		// number of device points
} ATLAS_MC_BATCHRW;

typedef struct {
	unsigned char bval;
	char cval[3];
} ATLAS_REGMAP;

typedef struct {
	unsigned short errval;
	char errdesc[128];
} ATLAS_ERRMAP;

#ifdef ATLS_DRIVER_MAIN_MELSEC

// Mapping of register types from ASCII to binary codes used by MC Protocol
ATLAS_REGMAP regmap[] =	{
				{ 0x91, "SM" },
				{ 0xA9, "SD" },
				{ 0x9C, "X"  },
				{ 0x9D, "Y"  },
				{ 0x90, "M"  },
				{ 0x92, "L"  },
				{ 0x93, "F"  },
				{ 0x94, "V"  },
				{ 0xA0, "B"  },
				{ 0xA8, "D"  },
				{ 0xB4, "W"  },
				{ 0xC1, "TS" },
				{ 0xC0, "TC" },
				{ 0xC7, "SS" },
				{ 0xC6, "SC" },
				{ 0xC8, "SN" },
				{ 0xC4, "CS" },
				{ 0xC3, "CC" },
				{ 0xC5, "CN" },
				{ 0xA1, "SB" },
				{ 0xB5, "SW" },
				{ 0x98, "S"  },
				{ 0xA2, "DX" },
				{ 0xA3, "DY" },
				{ 0xCC, "Z"  },
				{ 0xAF, "R"  },
				{ 0xB0, "ZR" },
				{ NULL, NULL }
			};

// Error code to Error message mapping
// Derived from Appendex 1.11 of the Q Series Hardware Design & Maintenance Manual
ATLAS_ERRMAP errmap[] = {
				{ 0x4000, "Serial communication checksum error" },
				{ 0x4001, "Unsupported request" },
				{ 0x4002, "Unsupported request" },
				{ 0x4003, "Global request cannot be performed" },
				{ 0x4004, "Access denied. System Protection is in effect" },
				{ 0x4005, "Data size too large" },
				{ 0x4006, "Initial communication failed" },
				{ 0x4008, "CPU module is busy (buffer full)" },
				{ 0x4010, "Cannot execute while CPU is in RUN Mode" },
				{ 0x4013, "Cannot execute while CPU is in RUN Mode" },

				{ 0x4021, "Specified drive memory does not exist or read error" },
				{ 0x4022, "Specified file name or number does not exist" },
				{ 0x4023, "Specified file name and file number do not match" },
				{ 0x4024, "Access denied. Insufficient user permissions" },
				{ 0x4025, "File is busy" },
				{ 0x4026, "Password, keyword, or password-32 must be set before access" },
				{ 0x4027, "Specified range is greater than file size" },
				{ 0x4028, "File already exists" },
				{ 0x4029, "Insufficient space to allocate file memory" },
				{ 0x402A, "Specified file is corrupt/abnormal" },
				{ 0x402B, "Unable to execute request in specified memory location" },
				{ 0x402C, "Operation failed, device is busy. Try again later" },

				{ 0x4030, "Specified device type unsupported or device number exceeds CPU limits for extended registers" },
				{ 0x4031, "Specified device number is out of range" },
				{ 0x4032, "Invalid device type specification or unsupported device type for requested operation" },
				{ 0x4033, "Write access denied. Device is reserved for system use only" },
				{ 0x4034, "Execution failed. CPU is in STOP Mode" },

				{ 0x4040, "Function not supported by target module" },
				{ 0x4041, "Specified address is not within the target module's memory range" },
				{ 0x4042, "Failed to access target module" },
				{ 0x4043, "Incorrect I/O address spec for target module" },
				{ 0x4044, "Failed to access target module due to bus or hardware error" },

				{ 0x4170, "Specified password is incorrect" },
				{ 0x4171, "Password is required before communication can proceed" },
				{ 0x4174, "Invalid request during password unlock attempt" },
				{ 0x4176, "Direct connection failed" },
				{ 0x4178, "Access denied. An FTP operation is currently active" },
				{ 0x4180, "System error due to module, CPU, or power supply hardware failure" },
				{ 0x4181, "Downstream transmission failed" },
				{ 0x4182, "Downstream communication timed out" },
				{ 0x4183, "Downstream connection lost" },
				{ 0x4184, "Communication buffer full" },
				{ 0x4185, "Lost connection to remote device" },

				{ NULL, NULL }
			};
#else

	extern ATLAS_REGMAP regmap[];
	extern ATLAS_ERRMAP errmap[];

#endif