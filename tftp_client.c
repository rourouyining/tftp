/* ************************************************************************
 *       Filename:  tftp_client.c
 *    Description:  
 *        Version:  1.0
 *        Created:  09/18/2018 10:18:00 AM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  YOUR NAME (), 
 *        Company:  
 * ************************************************************************/


#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "tftp.h"

struct sockaddr_in dest_addr;
int sockfd = 0;
int setEn = 0;

char *help = "************************************************\n"\
	      "*set [serverIP]                                *\n"\
	      "*put [filename]                                *\n"\
	      "*get [filename]                                *\n"\
	      "*help                                          *\n"\
	      "*ls                                            *\n"\
	      "*clear                                         *\n"\
	      "*exit/quit                                     *\n"\
	      "************************************************\n";

typedef void (*FUN) (int argc, char *argv[]);

typedef struct command_cmd{
	char *name;
	FUN fun;
}CMD;


void exit_fun(int argc, char *argv[])
{
	close(sockfd);
	exit(1);
}

void clear_fun(int argc, char *argv[])
{
	write(1, "\033[2J", 4);
	write(1, "\033[0:0H", 6);
	return;
}

void ls_fun(int argc, char *argv[])
{
	DIR *dp;
	char *dir = ".";
	struct dirent *dirp;

	dp = opendir(dir);
	if(dp == NULL)
		printf("cant open %s\n", dir);

	while((dirp = readdir(dp)) != NULL)
	{
		printf("%s ", dirp->d_name);
	}

	printf("\n");
	closedir(dp);
	return;
}

void help_fun(int argc, char *argv[])
{
	printf("%s", help);
	return;
}

void sendAck(struct TFTPData *data)
{
	struct TFTPACK ack;
	ack.header.opcode = htons(OPCODE_ACK);
	ack.block = data->block;

	socklen_t length = sizeof(dest_addr);
	if(sendto(sockfd, &ack, sizeof(ack), 0, (struct sockaddr*)&dest_addr, length) < 0)
	{
		perror("ack send()");
		return;
	}
	return;
}

void get_server_info(char *filename)
{
	int fd = 0;
	int len = 0;
	char *recv_buf[1024];
	
	FILE *fp = fopen(filename, "wb");
	if(fp == NULL)
	{
		perror("fopen()");
		return;
	}

	struct sockaddr_in client_addr;
	socklen_t cliaddr_len = sizeof(client_addr);

	while(1)
	{
		int bufferlen = 0;
		while(bufferlen == 0)
		{
			ioctl(sockfd, FIONREAD, &bufferlen);
		}
		printf("bufferlen = %d\n", bufferlen);

		void *buffer = malloc(bufferlen);

		len = recvfrom(sockfd, recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)&client_addr, &cliaddr_len);
		if(len < 0)
		{
			perror("recvfrom");
			return;
		}

		struct TFTPHeader *header = (struct TFTPHeader*)recv_buf;
		if(htons(header->opcode) == OPCODE_ERR)
		{
			struct TFTPERR *err = (struct TFTPERR*)buffer;
			printf("Error packet with code %d\n", ntohs(err->errcode));
			return;
		}
		else if(htons(header->opcode) == OPCODE_DATA)
		{
			struct TFTPData *data = (struct TFTPData*)buffer;
			printf("recv block %d\n", ntohs(data->block));

			fwrite(&data->data, len - sizeof(struct TFTPHeader) - sizeof(short), 1, fp);
			sendAck(data);
			
			if(len < sizeof(recv_buf))
				break;
		}
		else if(htons(header->opcode) == OPCODE_ACK)
		{
			struct stat buf;
			char *buffer;
			
			struct TFTPACK *ack = (struct TFTPACK*)buffer;
			int p_new = ntohs(ack->block);
			printf("recv block %d\n", ntohs(ack->block));
			
			fstat(fd, &buf);
			buffer = malloc(buf.st_size + 4 + 1);
			
			buffer[1] = header->opcode;
			*((unsigned short*)(buffer+2)) = htons(p_new+1);
			
			int readlen = read(fd, buffer + 4, buf.st_size +4);
			if(readlen < 0)
			{
				perror("read()");
				free(buffer);
				return;
			}

			socklen_t length = sizeof(dest_addr);
			sendto(sockfd, buffer, readlen + 4, 0, (struct sockaddr*)&dest_addr, length);
			if((readlen < sizeof(buffer)) && (feof(fp) != 0))
				break;
		}

	}
	fclose(fp);
	return;

}

void get_fun(int argc, char *argv[])
{
	if(setEn == 0)
	{
		printf("set ip please!\n");
		return;
	}

	struct TFTPHeader header;
	int filelen = 0;
	int packetsize = 0;
	void *packet;
	int flags;

	header.opcode = htons(OPCODE_RRQ);

	filelen = strlen(argv[1])+1;
	packetsize = sizeof(header) + filelen + 5 +1;

	packet = malloc(packetsize);
	memcpy(packet, &header, sizeof(header));
	memcpy(packet + sizeof(header), argv[1], filelen);

	char *mode = "octet";
	memcpy(packet + sizeof(header) + filelen, mode, strlen(mode) + 1);
	socklen_t length = sizeof(dest_addr);

	flags = sendto(sockfd, packet, packetsize, 0, (struct sockaddr *)&dest_addr, length);
	if(flags < 0)
	{
		perror("sendto()");
		return;
	}

	get_server_info(argv[1]);
	
	free(packet);
	return;
}

void put_fun(int argc, char *argv[])
{			
	if(setEn == 0)
	{
		printf("set ip please!\n");
		return;
	}

	struct TFTPHeader header;
	int filelen = 0;
	int packetsize = 0;
	void *packet;
	int flags;

	header.opcode = htons(OPCODE_WRQ);

	filelen = strlen(argv[1])+1;
	packetsize = sizeof(header) + filelen + 5 +1;

	packet = malloc(packetsize);
	memcpy(packet, &header, sizeof(header));
	memcpy(packet + sizeof(header), argv[1], filelen);

	char *mode = "octet";
	memcpy(packet + sizeof(header) + filelen, mode, strlen(mode) + 1);

	socklen_t length = sizeof(dest_addr);
	flags = sendto(sockfd, packet, packetsize, 0, (struct sockaddr *)&dest_addr, length);
	if(flags < 0)
	{
		perror("sendto()");
		return;
	}

	get_server_info(argv[1]);

	free(packet);
	return;
}

void set_fun(int argc, char *argv[])
{
	printf("ip:%s\n", argv[1]);
	inet_pton(AF_INET, argv[1], &dest_addr.sin_addr);
	setEn = 1;

	printf("set server ip successful\n");
	return;
}

CMD cmd_list[] = {
	{"set", set_fun},
	{"put", put_fun},
	{"get", get_fun},
	{"help", help_fun},
	{"ls", ls_fun},
	{"clear", clear_fun},
	{"exit", exit_fun},
	{"quit", exit_fun},		
};

int exec_cmd(char *cmd)
{
	char *buf[10] = {NULL};
	int num = 0;
	int i = 0;

	if(strlen(cmd) == 0)
		return -1;

	buf[0] = cmd;
	while((buf[num] = (strtok(buf[num], " \t"))) != NULL)
	{
		num++;
	}

	for(i = 0; i < sizeof(cmd_list)/sizeof(CMD); i++)
	{
		if(strcmp(cmd_list[i].name, buf[0]) == 0)
		{
			cmd_list[i].fun(num, buf);
			return 0;
		}
	}

	return -1;
}

int main(int argc, char *argv[])
{
	char buf[100];
	int flag;

	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(69);

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd < 0)
	{
		perror("socket");
		exit(-1);
	}

	printf("%s\n", help);
	printf("Please set the server ip\n");

	while(1)
	{		
		scanf("%s", buf);
		buf[strlen(buf)] = '\0';

		flag = exec_cmd(buf);
		if(flag < 0)
		{
			printf("command not found!\n");
			continue;
		}
	}
	
	return 0;
}


