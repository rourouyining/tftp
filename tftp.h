#ifndef __TFTP_H__
#define __TFTP_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <assert.h>

#define OPCODE_RRQ 1
#define OPCODE_WRQ 2
#define OPCODE_DATA 3
#define OPCODE_ACK 4
#define OPCODE_ERR 5

#define BLOCKSIZE 512


struct TFTPHeader{
	short opcode;
}__attribute__((packed));

struct TFTPWRRD{
	struct TFTPHeader header;
	char *filename;
	char *mode;
}__attribute__((packed));


struct TFTPData{
	struct TFTPHeader header;
	short block;
	char data[0];
}__attribute__((packed));

struct TFTPACK{
	struct TFTPHeader header;
	short block;
}__attribute__((packed));

struct TFTPERR{
	struct TFTPHeader header;
	short errcode;
	char *errmsg;
}__attribute__((packed));



#endif

