#include "./../libc/stdio.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <syscalls.h>

#include <exceptions.h>
#include <sys.h>

#include "shell_internal.h"

static char buffer[MAX_BUFFER_SIZE];
static int buffer_dim = 0;

static void printPreviousCommand(enum REGISTERABLE_KEYS scancode);
static void printNextCommand(enum REGISTERABLE_KEYS scancode);
static int run_pipeline(const char *leftCmd, const char *rightCmd);
static void trim(char *s);

char command_history[HISTORY_SIZE][MAX_BUFFER_SIZE] = {0};
static char command_history_buffer[MAX_BUFFER_SIZE] = {0};
uint8_t command_history_last = 0;

static uint8_t last_command_arrowed = 0;
static uint64_t last_command_output = 0;
static uint8_t builtin_background_flag = 0;
static volatile int current_pipeline_pids[2] = {0, 0};

uint8_t getCurrentBuiltinBackground(void)
{
	return builtin_background_flag;
}

void setCurrentBuiltinBackground(uint8_t value)
{
	builtin_background_flag = value;
}

int main(void)
{
	clearScreen();

	registerKey(KP_UP_KEY, printPreviousCommand);
	registerKey(KP_DOWN_KEY, printNextCommand);

	while (1)
	{
		printf("\e[0mshell \e[0;32m$\e[0m ");

		signed char c;

		while (buffer_dim < MAX_BUFFER_SIZE && (c = getchar()) != '\n')
		{
			command_history_buffer[buffer_dim] = c;
			buffer[buffer_dim++] = c;
		}

		buffer[buffer_dim] = 0;
		command_history_buffer[buffer_dim] = 0;

		if (buffer_dim == MAX_BUFFER_SIZE)
		{
			perror("\e[0;31mShell buffer overflow\e[0m\n");
			buffer[0] = 0;
			buffer_dim = 0;
			while (c != '\n')
			{
				c = getchar();
			}
			continue;
		}

		char *command = strtok(buffer, " ");

		char *pipeSep = NULL;
		for (int k = 0; k < buffer_dim; k++)
		{
			if (command_history_buffer[k] == '|')
			{
				pipeSep = &command_history_buffer[k];
				break;
			}
		}

		if (pipeSep != NULL)
		{
			char left[MAX_BUFFER_SIZE];
			char right[MAX_BUFFER_SIZE];
			int leftLen = 0;
			int rightLen = 0;

			for (int k = 0; k < buffer_dim && &command_history_buffer[k] < pipeSep && leftLen + 1 < MAX_BUFFER_SIZE; k++)
			{
				left[leftLen++] = command_history_buffer[k];
			}
			left[leftLen] = '\0';

			for (int k = (int)(pipeSep - command_history_buffer) + 1; k < buffer_dim && rightLen + 1 < MAX_BUFFER_SIZE; k++)
			{
				right[rightLen++] = command_history_buffer[k];
			}
			right[rightLen] = '\0';

			trim(left);
			trim(right);

			char leftName[MAX_BUFFER_SIZE];
			char rightName[MAX_BUFFER_SIZE];
			leftName[0] = rightName[0] = '\0';

			int iLeft = 0;
			int oLeft = 0;
			while (left[iLeft] == ' ' || left[iLeft] == '\t')
			{
				iLeft++;
			}
			while (left[iLeft] && left[iLeft] != ' ' && left[iLeft] != '\t' && oLeft + 1 < MAX_BUFFER_SIZE)
			{
				leftName[oLeft++] = left[iLeft++];
			}
			leftName[oLeft] = '\0';

			int iRight = 0;
			int oRight = 0;
			while (right[iRight] == ' ' || right[iRight] == '\t')
			{
				iRight++;
			}
			while (right[iRight] && right[iRight] != ' ' && right[iRight] != '\t' && oRight + 1 < MAX_BUFFER_SIZE)
			{
				rightName[oRight++] = right[iRight++];
			}
			rightName[oRight] = '\0';

			if (leftName[0] == '\0' || rightName[0] == '\0')
			{
				fprintf(FD_STDERR, "Invalid pipeline. Usage: cmd1 | cmd2\n");
			}
			else
			{
				(void)run_pipeline(leftName, rightName);
			}

			strncpy(command_history[command_history_last], command_history_buffer, MAX_BUFFER_SIZE - 1);
			command_history[command_history_last][MAX_BUFFER_SIZE - 1] = '\0';
			INC_MOD(command_history_last, HISTORY_SIZE);
			last_command_arrowed = command_history_last;
			buffer[0] = 0;
			buffer_dim = 0;
			continue;
		}

		int found = 0;

		if (command != NULL && *command != '\0')
		{
			for (int i = 0; i < command_count; i++)
			{
				if (strcmp(commands[i].name, command) != 0)
				{
					continue;
				}

				int runInBackground = 0;
				if (buffer_dim > 0)
				{
					int tail = buffer_dim - 1;
					while (tail >= 0 && (command_history_buffer[tail] == ' ' || command_history_buffer[tail] == '\t'))
					{
						tail--;
					}
					if (tail >= 0 && command_history_buffer[tail] == '&')
					{
						runInBackground = 1;
					}
				}

				if (commands[i].isProcess)
				{
					char *argv[32];
					int argc = 0;

					argv[argc++] = commands[i].name;

					char *token = NULL;
					while ((token = strtok(NULL, " ")) != NULL && argc < 31)
					{
						if (strcmp(token, "&") == 0)
						{
							break;
						}
						argv[argc++] = token;
					}
					argv[argc] = NULL;

					int pid = createProcess(commands[i].name, commands[i].entry, argv, argc, 0, 0, 0, runInBackground ? 0 : 1);
					if (!runInBackground && pid > 0)
					{
						waitProcess(pid);
					}
				}
				else
				{
					setCurrentBuiltinBackground((uint8_t)runInBackground);
					last_command_output = commands[i].builtin();
					setCurrentBuiltinBackground(0);
				}

				strncpy(command_history[command_history_last], command_history_buffer, MAX_BUFFER_SIZE - 1);
				command_history[command_history_last][MAX_BUFFER_SIZE - 1] = '\0';
				INC_MOD(command_history_last, HISTORY_SIZE);
				last_command_arrowed = command_history_last;
				found = 1;
				break;
			}
		}

		if (!found)
		{
			if (command != NULL && *command != '\0')
			{
				fprintf(FD_STDERR, "\e[0;33mCommand not found:\e[0m %s\n", command);
			}
			else if (command == NULL)
			{
				printf("\n");
			}
		}

		buffer[0] = 0;
		buffer_dim = 0;
	}

	__builtin_unreachable();
}

static void printPreviousCommand(enum REGISTERABLE_KEYS scancode)
{
	(void)scancode;
	clearInputBuffer();
	last_command_arrowed = SUB_MOD(last_command_arrowed, 1, HISTORY_SIZE);
	if (command_history[last_command_arrowed][0] != 0)
	{
		fprintf(FD_STDIN, command_history[last_command_arrowed]);
	}
}

static void printNextCommand(enum REGISTERABLE_KEYS scancode)
{
	(void)scancode;
	clearInputBuffer();
	last_command_arrowed = (last_command_arrowed + 1) % HISTORY_SIZE;
	if (command_history[last_command_arrowed][0] != 0)
	{
		fprintf(FD_STDIN, command_history[last_command_arrowed]);
	}
}

static int run_pipeline(const char *leftCmd, const char *rightCmd)
{
	int leftIdx = -1;
	int rightIdx = -1;

	for (int i = 0; i < command_count; i++)
	{
		if (leftIdx == -1 && strcmp(commands[i].name, leftCmd) == 0)
		{
			leftIdx = i;
		}
		if (rightIdx == -1 && strcmp(commands[i].name, rightCmd) == 0)
		{
			rightIdx = i;
		}
		if (leftIdx != -1 && rightIdx != -1)
		{
			break;
		}
	}

	if (leftIdx == -1)
	{
		fprintf(FD_STDERR, "Command not found: %s\n", leftCmd);
		return 1;
	}

	if (rightIdx == -1)
	{
		fprintf(FD_STDERR, "Command not found: %s\n", rightCmd);
		return 1;
	}

	if (!commands[leftIdx].isProcess || !commands[rightIdx].isProcess)
	{
		fprintf(FD_STDERR, "Only process commands can be piped\n");
		return 1;
	}

	int fds[2];
	if (pipe(fds) < 0)
	{
		fprintf(FD_STDERR, "pipeline: pipe() failed\n");
		return 1;
	}

	const int savedIn = 10;
	const int savedOut = 11;
	close(savedIn);
	close(savedOut);
	dup2(FD_STDIN, savedIn);
	dup2(FD_STDOUT, savedOut);

	dup2(fds[1], FD_STDOUT);
	int leftPid = createProcess(commands[leftIdx].name, commands[leftIdx].entry, 0, 0, 0, 0, 0, 0);
	dup2(savedOut, FD_STDOUT);
	close(fds[1]);

	dup2(fds[0], FD_STDIN);
	int rightPid = createProcess(commands[rightIdx].name, commands[rightIdx].entry, 0, 0, 0, 0, 0, 0);
	dup2(savedIn, FD_STDIN);
	close(fds[0]);

	close(savedIn);
	close(savedOut);

	current_pipeline_pids[0] = leftPid;
	current_pipeline_pids[1] = rightPid;

	if (leftPid > 0)
	{
		waitProcess(leftPid);
	}

	if (rightPid > 0)
	{
		waitProcess(rightPid);
	}

	current_pipeline_pids[0] = 0;
	current_pipeline_pids[1] = 0;
	return 0;
}

static void trim(char *s)
{
	if (s == NULL)
	{
		return;
	}

	int length = (int)strlen(s);
	int start = 0;
	int end = length - 1;

	while (start < length && (s[start] == ' ' || s[start] == '\t'))
	{
		start++;
	}

	while (end >= start && (s[end] == ' ' || s[end] == '\t'))
	{
		end--;
	}

	int dst = 0;
	for (int idx = start; idx <= end; idx++)
	{
		s[dst++] = s[idx];
	}
	s[dst] = '\0';
}
