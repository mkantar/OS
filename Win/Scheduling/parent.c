/**
Operating Systems Course Fall 2017
Submitted by Muharrem Kantar, 210201050
All rights reserved.
*/

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <Windows.h>
#include <strsafe.h>

#define BUFFSIZE 255
#define NUM_OF_CHILD 5
#define ALPHA 0.5

typedef struct {
	int next_cpu_burst_prediction;
	int actual_cpu_burst;
	int predicted_cpu_burst;
	int child_id;
} SCHEDULE, *PSCHEDULE;

HANDLE g_hChildStd_IN_Read[NUM_OF_CHILD] = { NULL };
HANDLE g_hChildStd_IN_Write[NUM_OF_CHILD] = { NULL };
HANDLE g_hChildStd_OUT_Read[NUM_OF_CHILD] = { NULL };
HANDLE g_hChildStd_OUT_Write[NUM_OF_CHILD] = { NULL };

typedef int(*cmp_func)(int i0, int i1);
int less(int i0, int i1) { return i0 < i1; }
int greater(int i0, int i1) { return i0 > i1; }

VOID init(LPVOID lpParam);
BOOL CreateChildProcess(LPVOID lpParam);
BOOL IsSorted(int n, PSCHEDULE* pSchedule, cmp_func cmp);
VOID PrintSchedule(int n, const PSCHEDULE* pSchedule);
VOID PrintExecutionOrder(int n, PSCHEDULE* pSchedule, int it);
VOID Exchange(PSCHEDULE* pSchedule, int i, int j);
INT Partition(PSCHEDULE* pSchedule, int l, int r, cmp_func cmp);
VOID QuickSort(PSCHEDULE* pSchedule, int l, int r, cmp_func cmp);
VOID GetBurst(PSCHEDULE pSchedule, int burst_time);
VOID RescheduleProcesses(int n, PSCHEDULE* pSchedule);
VOID RunProcesses(LPVOID scheduleInfo, LPVOID processInfo);

int _tmain(int argc, TCHAR* argv[])
{
	PROCESS_INFORMATION pi[NUM_OF_CHILD];
	SECURITY_ATTRIBUTES sa[NUM_OF_CHILD];

	printf("\n->Parent process is running...\n");

	for (int i = 0; i < NUM_OF_CHILD; i++)
	{
		sa[i].nLength = sizeof(SECURITY_ATTRIBUTES);
		sa[i].bInheritHandle = TRUE;
		sa[i].lpSecurityDescriptor = NULL;
	}

	printf("->Creating pipes...\n\n");

	for (int i = 0; i < NUM_OF_CHILD; i++)
	{
		if (!CreatePipe(&g_hChildStd_OUT_Read[i], &g_hChildStd_OUT_Write[i], &sa[i], 0))
		{
			fprintf(stderr, "ERROR: Unable to create a pipe for STDOUT!\nExitting...\n");
			system("pause");
			ExitProcess(EXIT_FAILURE);
		}

		if (!SetHandleInformation(g_hChildStd_OUT_Read[i], HANDLE_FLAG_INHERIT, 0))
		{
			fprintf(stderr, "ERROR: Unable not to inherit handle!\nExitting...\n");
			system("pause");
			ExitProcess(EXIT_FAILURE);
		}

		if (!CreatePipe(&g_hChildStd_IN_Read[i], &g_hChildStd_IN_Write[i], &sa[i], 0))
		{
			fprintf(stderr, "ERROR: Unable to create a pipe for STDIN!\nExitting...\n");
			system("pause");
			ExitProcess(EXIT_FAILURE);
		}

		if (!SetHandleInformation(g_hChildStd_IN_Write[i], HANDLE_FLAG_INHERIT, 0))
		{
			fprintf(stderr, "ERROR: Unable not to inherit handle!\nExitting...\n");
			system("pause");
			ExitProcess(EXIT_FAILURE);
		}

	}

	PSCHEDULE* pSchedule = (PSCHEDULE*)malloc(sizeof(PSCHEDULE) * NUM_OF_CHILD);

	if (pSchedule == NULL)
	{
		fprintf(stderr, "ERROR: Unable to allocate memory!\nExittig...");
		system("pause");
		ExitProcess(EXIT_FAILURE);
	}

	for (int i = 0; i < NUM_OF_CHILD; i++)
	{
		pSchedule[i] = (PSCHEDULE)malloc(sizeof(SCHEDULE) * NUM_OF_CHILD);
		if (pSchedule[i] == NULL)
		{
			fprintf(stderr, "ERROR: Unable to allocate memory!\nExittig...");
			system("pause");
			ExitProcess(EXIT_FAILURE);
		}
	}

	init(pSchedule);

	if (!CreateChildProcess(pi))
	{
		fprintf(stderr, "Unable to create child processes!\n");
		system("pause");
		ExitProcess(EXIT_FAILURE);
	}

	RunProcesses(pSchedule, pi);

	for (int i = 0; i < NUM_OF_CHILD; i++)
	{
		free(pSchedule[i]);
	}

	free(pSchedule);

	for (int i = 0; i < NUM_OF_CHILD; i++)
	{
		TerminateProcess(pi[i].hProcess, 0);
	}

	for (int i = 0; i < NUM_OF_CHILD; i++)
	{
		CloseHandle(pi[i].hThread);
		CloseHandle(pi[i].hProcess);
	}

	system("pause");
	return EXIT_SUCCESS;
}

VOID init(LPVOID lpParam)
{
	PSCHEDULE* pSchedule = (PSCHEDULE*)lpParam;
	int initial_tau[NUM_OF_CHILD] = { 300, 220, 180, 45, 255 };

	for (int i = 0; i < NUM_OF_CHILD; i++)
	{
		pSchedule[i]->next_cpu_burst_prediction = 0;
		pSchedule[i]->child_id = i;
		pSchedule[i]->predicted_cpu_burst = initial_tau[i];
		pSchedule[i]->actual_cpu_burst = 0;
	}
}

BOOL CreateChildProcess(LPVOID lpParam)
{
	TCHAR szCmdLine[] = TEXT("Child.exe");
	PROCESS_INFORMATION* pi = (PROCESS_INFORMATION*)lpParam;
	STARTUPINFO si[NUM_OF_CHILD];

	for (int i = 0; i < NUM_OF_CHILD; i++)
	{
		SecureZeroMemory(&pi[i], sizeof(PROCESS_INFORMATION));
		SecureZeroMemory(&si[i], sizeof(STARTUPINFO));
		si[i].cb = sizeof(STARTUPINFO);
		si[i].hStdError = g_hChildStd_OUT_Write[i];
		si[i].hStdOutput = g_hChildStd_OUT_Write[i];
		si[i].hStdInput = g_hChildStd_IN_Read[i];
		si[i].dwFlags |= STARTF_USESTDHANDLES;

		if (!CreateProcess(NULL, szCmdLine, NULL, NULL, TRUE, CREATE_SUSPENDED, NULL, NULL, &si[i], &pi[i]))
		{
			return FALSE;
		}

	}
	return TRUE;
}

VOID GetBurst(PSCHEDULE pSchedule, int burst_time)
{
	if (pSchedule == NULL)
		return;

	pSchedule->actual_cpu_burst = burst_time;
	pSchedule->next_cpu_burst_prediction = (int)(ALPHA * (pSchedule->actual_cpu_burst) + (1 - ALPHA) * (pSchedule->next_cpu_burst_prediction));
}

VOID RescheduleProcesses(int n, PSCHEDULE* pSchedule)
{
	for (int i = 0; i < n; i++)
	{
		pSchedule[i]->predicted_cpu_burst = pSchedule[i]->next_cpu_burst_prediction;
		pSchedule[i]->next_cpu_burst_prediction = 0;
		pSchedule[i]->actual_cpu_burst = 0;
	}
}

BOOL IsSorted(int n, PSCHEDULE* pSchedule, cmp_func cmp)
{
	for (int i = 1; i < n; ++i)
		if ((*cmp)(pSchedule[i]->predicted_cpu_burst, pSchedule[i - 1]->predicted_cpu_burst))
			return FALSE;

	return TRUE;
}

VOID PrintSchedule(int n, const PSCHEDULE* pSchedule)
{
	printf("P#\t %14s\t %9s\t %20s\n", "Tau(Predicted)", "t(Actual)", "Tau(Next Prediction)");
	for (int i = 0; i < n; i++)
	{
		printf("P%d\t %7d\t %4d\t %16d\n", pSchedule[i]->child_id + 1, pSchedule[i]->predicted_cpu_burst, pSchedule[i]->actual_cpu_burst, pSchedule[i]->next_cpu_burst_prediction);
	}
	printf("\n===================================================================\n");
}

VOID PrintExecutionOrder(int n, PSCHEDULE* pSchedule, int it)
{
	printf("Execution Order(%d): < ", it);
	for (int i = 0; i < n; i++)
	{
		printf("P%d ", pSchedule[i]->child_id + 1);
	}
	printf(">");

	printf("\n===================================================================\n");
}

VOID Exchange(PSCHEDULE* pSchedule, int i, int j)
{
	PSCHEDULE temp = pSchedule[i];
	pSchedule[i] = pSchedule[j];
	pSchedule[j] = temp;
}

INT Partition(PSCHEDULE* pSchedule, int l, int r, cmp_func cmp)
{
	int i = l - 1;
	int j = r;
	PSCHEDULE v = pSchedule[r];

	while (TRUE)
	{
		while ((*cmp)(pSchedule[++i]->predicted_cpu_burst, v->predicted_cpu_burst));

		while ((*cmp)(v->predicted_cpu_burst, pSchedule[--j]->predicted_cpu_burst))
			if (j == l)
				break;

		if (i >= j)
			break;

		Exchange(pSchedule, i, j);
	}
	Exchange(pSchedule, i, r);
	return i;
}

VOID QuickSort(PSCHEDULE* pSchedule, int l, int r, cmp_func cmp)
{
	if (r <= l)
		return;

	int i = Partition(pSchedule, l, r, cmp);
	QuickSort(pSchedule, l, i - 1, cmp);
	QuickSort(pSchedule, i + 1, r, cmp);
}

VOID RunProcesses(LPVOID scheduleInfo, LPVOID processInfo)
{
	PROCESS_INFORMATION* pi = (PROCESS_INFORMATION*)processInfo;
	PSCHEDULE* pSchedule = (PSCHEDULE*)scheduleInfo;
	DWORD dwRead = 0, dwWritten = 0;

	char message[BUFFSIZE];
	int burst_time = 0;

	memset(message, '\0', BUFFSIZE);
	for (int i = 0; i < NUM_OF_CHILD; i++)
	{
		QuickSort(pSchedule, 0, NUM_OF_CHILD - 1, &less);

		if (!IsSorted(NUM_OF_CHILD, pSchedule, &less))
		{
			fprintf(stderr, "FAILED: Sorting incorrectly.\n");
			system("pause");
			ExitProcess(EXIT_FAILURE);
		}

		PrintSchedule(NUM_OF_CHILD, pSchedule);
		PrintExecutionOrder(NUM_OF_CHILD, pSchedule, i + 1);

		for (int j = 0; j < NUM_OF_CHILD; j++)
		{
			int id = pSchedule[j]->child_id;
			if (i == 0) {
				sprintf_s(message, BUFFSIZE, "%d", id);
				if (!WriteFile(g_hChildStd_IN_Write[id], message, (DWORD)strlen(message) + 1, &dwWritten, 0))
				{
					fprintf(stderr, "Unable to write to the pipe!\n");
					system("pause");
					ExitProcess(EXIT_FAILURE);
				}
				memset(message, '\0', BUFFSIZE);
			}

			ResumeThread(pi[id].hThread);

			if (!ReadFile(g_hChildStd_OUT_Read[id], message, BUFFSIZE, &dwRead, NULL))
			{
				fprintf(stderr, "Unable to read from the pipe!\n");
				system("pause");
				ExitProcess(EXIT_FAILURE);
			}

			burst_time = atoi(message);

			GetBurst(pSchedule[id], burst_time);

			SuspendThread(pi[id].hThread);

			memset(message, '\0', BUFFSIZE);
		}

		PrintSchedule(NUM_OF_CHILD, pSchedule);
		RescheduleProcesses(NUM_OF_CHILD, pSchedule);
	}

}