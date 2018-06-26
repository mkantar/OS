#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <tchar.h>
#include <time.h>
#include <Windows.h>

#define NUMBER_OF_PROCESS 5
#define BUFF_SIZE NUMBER_OF_PROCESS
#define MAX_SEMAPHORE NUMBER_OF_PROCESS
#define MESSAGE_SIZE 32

TCHAR szName[] = TEXT("Global\\BufferArea");
TCHAR szMutex[] = TEXT("Global\\MutexArea");
TCHAR szFull[] = TEXT("Global\\FullArea");
TCHAR szEmpty[] = TEXT("Global\\EmptyArea");
TCHAR szBufferIn[] = TEXT("Global\\BufferInArea");
TCHAR szBufferOut[] = TEXT("Global\\BufferOutArea");

int main(int argc, char* argv[]) {
	STARTUPINFO si[NUMBER_OF_PROCESS];
	PROCESS_INFORMATION pi[NUMBER_OF_PROCESS];
	SECURITY_ATTRIBUTES sa[NUMBER_OF_PROCESS];
	HANDLE writePipe[NUMBER_OF_PROCESS];
	HANDLE readPipe[NUMBER_OF_PROCESS];
	HANDLE hMapFile;
	HANDLE hMutexFile;
	HANDLE hFullFile;
	HANDLE hEmptyFile;
	HANDLE hBufferInFile;
	HANDLE hBufferOutFile;
	HANDLE mutex, full, empty;
	LPCTSTR pBuf, pMutex, pFull, pEmpty, pBufferIn, pBufferOut;

	hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, BUFF_SIZE*sizeof(int), szName);
	hMutexFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(HANDLE), szMutex);
	hFullFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(HANDLE), szFull);
	hEmptyFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(HANDLE), szEmpty);
	hBufferInFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(int), szBufferIn);
	hBufferOutFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(int), szBufferOut);

	if ((hMapFile == NULL) || (hMutexFile == NULL) || (hFullFile == NULL) || (hEmptyFile == NULL) || (hBufferInFile == NULL) ||(hBufferOutFile == NULL)) {
		fprintf(stderr, "Unable to create shared memory at line %d!\n", __LINE__);
		system("pause");
		exit(EXIT_FAILURE);
	}

	mutex = CreateSemaphore(NULL, 1, MAX_SEMAPHORE, NULL);
	full = CreateSemaphore(NULL, 0, MAX_SEMAPHORE, NULL);
	empty = CreateSemaphore(NULL, NUMBER_OF_PROCESS, MAX_SEMAPHORE, NULL);

	if ((mutex == NULL) || (full == NULL) ||(empty == NULL)) {
		fprintf(stderr, "Unable to create semaphores!\n");
		system("pause");
		exit(EXIT_FAILURE);
	}

	pBuf = (LPTSTR)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, BUFF_SIZE*sizeof(int));
	pMutex = (LPTSTR)MapViewOfFile(hMutexFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(HANDLE));
	pFull = (LPTSTR)MapViewOfFile(hFullFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(HANDLE));
	pEmpty = (LPTSTR)MapViewOfFile(hEmptyFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(HANDLE));
	pBufferIn = (LPTSTR)MapViewOfFile(hBufferInFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int));
	pBufferOut = (LPTSTR)MapViewOfFile(hBufferOutFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int));

	if ((pBuf == NULL) || (pMutex == NULL) || (pFull == NULL) || (pEmpty == NULL) || (pBufferIn == NULL)) {
		fprintf(stderr, "Unable to map view of shared memory!\n");
		CloseHandle(hMapFile);
		CloseHandle(hMutexFile);
		CloseHandle(hFullFile);
		CloseHandle(hEmptyFile);
		CloseHandle(hBufferInFile);
		CloseHandle(hBufferOutFile);
		system("pause");
		exit(EXIT_FAILURE);
	}

	int bufferInCount = 0;
	int bufferOutCount = 0;

	int initial[BUFF_SIZE];

	for (int i = 0; i < BUFF_SIZE; i++) {
		initial[i] = -1;
	}

	memcpy((PVOID)pBuf, initial, BUFF_SIZE*sizeof(int));
	memcpy((PVOID)pMutex, &mutex, sizeof(HANDLE));
	memcpy((PVOID)pFull, &full, sizeof(HANDLE));
	memcpy((PVOID)pEmpty, &empty, sizeof(HANDLE));
	memcpy((PVOID)pBufferIn, &bufferInCount, sizeof(int));
	memcpy((PVOID)pBufferOut, &bufferOutCount, sizeof(int));


	for (int i = 0; i < NUMBER_OF_PROCESS; i++) {
		SecureZeroMemory(&sa[i], sizeof(SECURITY_ATTRIBUTES));
		sa[i].bInheritHandle = TRUE;
		sa[i].lpSecurityDescriptor = NULL;
		sa[i].nLength = sizeof(SECURITY_ATTRIBUTES);

		if (!CreatePipe(&readPipe[i], &writePipe[i], &sa[i], 0)) {
			fprintf(stderr, "Unable to create pipe!\n");
			system("pause");
			exit(EXIT_FAILURE);
		}

		SecureZeroMemory(&si[i], sizeof(STARTUPINFO));
		SecureZeroMemory(&pi[i], sizeof(PROCESS_INFORMATION));

		si[i].cb = sizeof(STARTUPINFO);
		si[i].hStdInput = readPipe[i];
		si[i].hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
		si[i].hStdError = GetStdHandle(STD_ERROR_HANDLE);
		si[i].dwFlags = STARTF_USESTDHANDLES;

		if (!CreateProcess(NULL, "Child.exe", NULL, NULL, TRUE, 0, NULL, NULL, &si[i], &pi[i])) {
			fprintf(stderr, "Unable to create the child process %d!\n", i + 1);
			system("pause");
			exit(EXIT_FAILURE);
		}
	}

	const char* producer = "produced";
	const char* consumer = "consumed";
	const char* swapper = "swapped";

	int bytesToWrite = 0;
	int bytesWritten = 0;

	srand(time(NULL));

	for (int i = 0; i < NUMBER_OF_PROCESS; i++) {
		int task;
		char message[MESSAGE_SIZE];
		int produced_num = rand() % NUMBER_OF_PROCESS + 1;
		char process_num[3];
		char produced_s[3];
		_itoa_s(produced_num, produced_s, sizeof(produced_s), 10);
		_itoa_s(i + 1, process_num, sizeof(process_num), 10);
		strcpy_s(message, strlen("Child ") + 1, "Child ");
		strcat_s(message, sizeof(message), process_num);
		if (i < 3) {
			if (i == 0) {
				strcat_s(message, sizeof(message), " ");
				strcat_s(message, sizeof(message), producer);
				strcat_s(message, sizeof(message), " ");
				strcat_s(message, sizeof(message), produced_s);
			}
			else if (i == 1) {
				strcat_s(message, sizeof(message), " ");
				strcat_s(message, sizeof(message), consumer);
			}
			else {
				strcat_s(message, sizeof(message), " ");
				strcat_s(message, sizeof(message), swapper);
			}

		}
		else {
			task = rand() % 3;

			switch (task) {
			case 0:
				strcat_s(message, sizeof(message), " ");
				strcat_s(message, sizeof(message), producer);
				strcat_s(message, sizeof(message), " ");
				strcat_s(message, sizeof(message), produced_s);
				break;
			case 1:
				strcat_s(message, sizeof(message), " ");
				strcat_s(message, sizeof(message), consumer);
				break;
			case 2:
				strcat_s(message, sizeof(message), " ");
				strcat_s(message, sizeof(message), swapper);
				break;
			default:
				break;
			}

		}

		bytesToWrite = strlen(message);
		bytesToWrite++;

		if (!WriteFile(writePipe[i], message, bytesToWrite, &bytesWritten, NULL)) {
			fprintf(stderr, "Unable to send message!\n");
			system("pause");
			exit(EXIT_FAILURE);
		}

		printf("number of bytes: %d -- number of bytes is sent %d\n", bytesToWrite, bytesWritten);
	}


	for (int i = 0; i < NUMBER_OF_PROCESS; i++) {
		WaitForSingleObject(pi[i].hProcess, INFINITE);
		CloseHandle(readPipe[i]);
		CloseHandle(writePipe[i]);
		CloseHandle(pi[i].hThread);
		CloseHandle(pi[i].hProcess);
	}

	UnmapViewOfFile(pBuf);
	UnmapViewOfFile(pMutex);
	UnmapViewOfFile(pFull);
	UnmapViewOfFile(pEmpty);
	CloseHandle(hMapFile);
	CloseHandle(hMutexFile);
	CloseHandle(hFullFile);
	CloseHandle(hEmptyFile);
	CloseHandle(hBufferInFile);
	CloseHandle(hBufferOutFile);

	system("pause");
	return EXIT_SUCCESS;
}