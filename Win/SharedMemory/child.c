#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <tchar.h>
#include <time.h>
#include <Windows.h>

#pragma comment(lib, "user32.lib")

#define MESSAGE_SIZE 32
#define BUFF_SIZE 5

TCHAR szName[] = TEXT("Global\\BufferArea");
TCHAR szMutex[] = TEXT("Global\\MutexArea");
TCHAR szFull[] = TEXT("Global\\FullArea");
TCHAR szEmpty[] = TEXT("Global\\EmptyArea");
TCHAR szBufferIn[] = TEXT("Global\\BufferInArea");
TCHAR szBufferOut[] = TEXT("Global\\BufferOutArea");

int get_task(char*, int*);

int main(int argc, char* argv[]) {
	char message[MESSAGE_SIZE];
	int task;
	int* produced = malloc(sizeof(int));

	HANDLE hMapFile;
	HANDLE hMutexFile;
	HANDLE hFullFile;
	HANDLE hEmptyFile;
	HANDLE hBufferInFile;
	HANDLE hBufferOutFile;
	LPCTSTR pBuf, pMutex, pFull, pEmpty, pBufferIn, pBufferOut;

	for (int i = 0; (message[i] = getchar()) != 0; i++);


	task = get_task(message, produced);

	hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, szName);
	hMutexFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, szMutex);
	hFullFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, szFull);
	hEmptyFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, szEmpty);
	hBufferInFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, szBufferIn);
	hBufferOutFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, szBufferOut);

	if ((hMapFile == NULL) || (hMutexFile == NULL) ||(hFullFile == NULL) || (hEmptyFile == NULL) ||(hBufferInFile == NULL) || (hBufferOutFile == NULL)) {
		fprintf(stderr, "Unable to reach shared memory at line %d!\n", __LINE__);
		system("pause");
		exit(EXIT_FAILURE);
	}

	pBuf = (LPTSTR)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, BUFF_SIZE*sizeof(int));
	pMutex = (LPTSTR)MapViewOfFile(hMutexFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(HANDLE));
	pFull = (LPTSTR)MapViewOfFile(hFullFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(HANDLE));
	pEmpty = (LPTSTR)MapViewOfFile(hEmptyFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(HANDLE));
	pBufferIn = (LPTSTR)MapViewOfFile(hBufferInFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int));
	pBufferOut = (LPTSTR)MapViewOfFile(hBufferOutFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int));

	if ((pBuf == NULL) || (pMutex == NULL) || (pFull == NULL) || (pEmpty == NULL) || (pBufferIn == NULL) || (pBufferOut == NULL)) {
		fprintf(stderr, "Unable to map view of the shared memory at line %d!\n", __LINE__);
		CloseHandle(hMapFile);
		CloseHandle(hMutexFile);
		CloseHandle(hFullFile);
		CloseHandle(hEmptyFile);
		CloseHandle(hBufferInFile);
		CloseHandle(hBufferOutFile);
		system("pause");
		exit(EXIT_FAILURE);
	}


	srand(time(NULL));

	switch (task) {
	case 0: // producer
		do
		{
			WaitForSingleObject((HANDLE)pMutex, INFINITE);
			WaitForSingleObject((HANDLE)pEmpty,INFINITE);

			if (pBuf[(*(int*)pBufferIn)] != -1) {
				if (*((int*)pBufferIn) < BUFF_SIZE)
					*((int*)pBufferIn) += 1;
				else
					*((int*)pBufferIn) = 0;
				continue;
			}
			else {
				int* myPointer = pBuf;
				myPointer[(*(int*)pBufferIn)] = *produced;
				printf("%s at index %d\n", message, (*(int*)pBufferIn));

				if (*((int*)pBufferIn) < BUFF_SIZE)
					*((int*)pBufferIn) += 1;
				else
					*((int*)pBufferIn) = 0;
			}

			Sleep(1000);

			ReleaseSemaphore((HANDLE)pMutex, 1, NULL);
			ReleaseSemaphore((HANDLE)pFull, 1, NULL);

		} while (TRUE);
		
		break;
	case 1: // consumer
		do
		{
			WaitForSingleObject((HANDLE)pFull, INFINITE);
			WaitForSingleObject((HANDLE)pMutex, INFINITE);

			if (pBuf[*((int*)pBufferOut)] == -1) {
				if (*((int*)pBufferOut) < BUFF_SIZE)
					*((int*)pBufferOut) += 1;
				else
					*((int*)pBufferOut) = 0;
				continue;
			}
			else {
				printf("%s %d at index %d\n", message, pBuf[*((int*)pBufferOut)], *((int*)pBufferOut));
				
				int* myPointer = pBuf;
				myPointer[(*(int*)pBufferOut)] = -1;

				if (*((int*)pBufferOut) < BUFF_SIZE)
					*((int*)pBufferOut) += 1;
				else
					*((int*)pBufferOut) = 0;
			}

			Sleep(500);

			ReleaseSemaphore((HANDLE)pMutex, 1, NULL);
			ReleaseSemaphore((HANDLE)pEmpty, 1, NULL);


		} while (TRUE); 
		
		break;
	case 2: // swapper
		do
		{
			WaitForSingleObject((HANDLE)pMutex, INFINITE);
			WaitForSingleObject((HANDLE)pEmpty, INFINITE);

			int index0;
			int index1;

			do {
				index0 = rand() % BUFF_SIZE;
				index1 = rand() % BUFF_SIZE;

				if (index0 != index1)
					break;

			} while (TRUE);


			if (((pBuf[index0] != -1) && (pBuf[index1] == -1)) || ((pBuf[index0] == -1) && (pBuf[index1] != -1)) || ((pBuf[index0] != -1) && (pBuf[index1] != -1))) {
				int* buff = (int*)pBuf;
				int temp = buff[index0];
				buff[index0] = buff[index1];
				buff[index1] = temp;
				printf("%s index %d with index %d\n", message, index0, index1);
			}
			else{
				continue;
			}

			Sleep(1500);

			ReleaseSemaphore((HANDLE)pMutex, 1, NULL);
			ReleaseSemaphore((HANDLE)pFull, 1, NULL);

		} while (TRUE);
	
		break;
	case -1:
		printf("Unable to take task!\n");
		system("pause");
		exit(EXIT_FAILURE);
		break;
	default:
		printf("-- FATAL ERROR --\n");
		system("pause");
		exit(EXIT_FAILURE);
		break;
	}

	UnmapViewOfFile(pBuf);
	UnmapViewOfFile(pMutex);
	UnmapViewOfFile(pFull);
	UnmapViewOfFile(pEmpty);
	UnmapViewOfFile(pBufferIn);
	UnmapViewOfFile(pBufferOut);
	CloseHandle(hMapFile);
	CloseHandle(hMutexFile);
	CloseHandle(hFullFile);
	CloseHandle(hEmptyFile);
	CloseHandle(hBufferInFile);
	CloseHandle(hBufferOutFile);

	free(produced);

	system("pause");
	return EXIT_SUCCESS;
}

int get_task(char* message, int* produced) {
	int task = -1;
	int size = strlen(message);
	
	for (int i = 0; i < size - 6; i++) {
		if (message[i] == 'p') {
			if (message[i + 1] == 'r') {
				if (message[i + 2] == 'o') {
					if (message[i + 3] == 'd') {
						if (message[i + 4] == 'u') {
							if (message[i + 5] == 'c') {
								if (message[i + 6] == 'e') {
									task = 0;
									char number[2];
									number[0] = message[size - 1];
									number[1] = '\0';
									*produced = atoi(number);
								}
							}
						}
					}
				}
			}
		} else if (message[i] == 'c') {
			if (message[i + 1] == 'o') {
				if (message[i + 2] == 'n') {
					if (message[i + 3] == 's') {
						if (message[i + 4] == 'u') {
							if (message[i + 5] == 'm') {
								if (message[i + 6] == 'e') {
									task = 1;
								}
							}
						}
					}
				}
			}
		} else if (message[i] == 's') {
			if (message[i + 1] == 'w') {
				if (message[i + 2] == 'a') {
					if (message[i + 3] == 'p') {
						task = 2;
					}
				}
			}
		}
	}


	return task;
}
