#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <time.h>
#include <math.h>
#include <strsafe.h>
#include <Windows.h>

#define BUFFSIZE 255 
#define NUM_OF_THREAD 4

HANDLE g_hStdIn = NULL;
HANDLE g_hStdOut = NULL;


int _tmain(int argc, TCHAR* argv[])
{
	DWORD dwRead, dwWritten;
	time_t t;
	int id;
	char message[BUFFSIZE];


	g_hStdIn = GetStdHandle(STD_INPUT_HANDLE);
	g_hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

	if ((g_hStdOut == INVALID_HANDLE_VALUE) || (g_hStdIn == INVALID_HANDLE_VALUE))
	{
		fprintf(stderr, "ERROR: Unable to inherit STDIN and STDOUT handlers.\n");
		system("pause");
		ExitProcess(EXIT_FAILURE);
	}

	if (!ReadFile(g_hStdIn, message, BUFFSIZE, &dwRead, 0))
	{
		fprintf(stderr, "Unable to read from the pipe!\n");
		system("pause");
		ExitProcess(EXIT_FAILURE);
	}

	id = atoi(message);

	memset(message, '\0', BUFFSIZE),

		srand((unsigned int)time(&t) + id);

	do {
		int slp = rand() % 250 + 50;

		Sleep(slp);

		sprintf_s(message, BUFFSIZE, "%d", slp);

		if (!WriteFile(g_hStdOut, message, (DWORD)strlen(message) + 1, &dwWritten, 0))
		{
			fprintf(stderr, "Unable to write to the pipe!\n");
			system("pause");
			ExitProcess(EXIT_FAILURE);
		}

	} while (TRUE);

	system("pause");
	return EXIT_SUCCESS;
}