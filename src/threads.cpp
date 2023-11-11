#include "defs.hpp"

thrd_t mainSearchThread;
thrd_t workerThreads[MAX_THREADS];
bool threadCreated = false;

static int startWorkerThread(void* workerData)
{
    SearchWorkerData* data = (SearchWorkerData*)workerData;
    if (data->sInfo->debugMode)
		std::cout << "Thread " << data->threadID << " began working...\n";
    workerSearchPos(data);
    if (data->sInfo->debugMode)
		std::cout << "Thread " << data->threadID << " finished working...\n";
    if (data->threadID == 0)
        std::cout << "bestmove " << moveToStr(data->sTable->pvTable[0][0]) << "\n";
    delete workerData;

    return 0;
}

static void setupWorkerThread(Board* board, SearchInfo* sInfo, SearchTable* sTable, HashTable* tt,
                              thrd_t* workerThread, int threadID)
{
    SearchWorkerData* data = new SearchWorkerData();
    data->board = new Board();
    memcpy(data->board, board, sizeof(Board));
    data->sTable = new SearchTable();
    memcpy(data->sTable, sTable, sizeof(SearchTable));
    data->sInfo = sInfo;
    data->threadID = threadID;
    data->tt = tt;

    thrd_create(workerThread, startWorkerThread, (void*)data);
}

void createSearchWorkers(Board* board, SearchInfo* sInfo, SearchTable* sTable, HashTable* tt)
{
    for (int i = 0; i < sInfo->threadCount; i++) {
        setupWorkerThread(board, sInfo, sTable, tt, &workerThreads[i], i);
    }
}

void joinSearchWorkers(const SearchInfo* sInfo)
{
    for (int i = 0; i < sInfo->threadCount; i++)
        thrd_join(workerThreads[i], NULL);
}

static int threadSearchPos(void* thData)
{
    SearchThreadData* data = (SearchThreadData*)thData;
    if (data->sInfo->debugMode)
        std::cout << "Entered a new thread!\n";

    searchPos(data->board, &hashTable, data->sInfo, data->sTable);

    if (data->sInfo->debugMode)
        std::cout << "Exiting the new thread!\n";

    delete data;
    return 0;
}

thrd_t launchSearchThread(Board* board, HashTable* tt, SearchInfo* sInfo, SearchTable* sTable)
{
    SearchThreadData* data = new SearchThreadData();
    data->board = new Board();
    memcpy(data->board, board, sizeof(Board));
    data->sTable = new SearchTable();
    memcpy(data->sTable, sTable, sizeof(SearchTable));
    data->tt = tt;
    data->sInfo = sInfo;

    thrd_t th;
    thrd_create(&th, threadSearchPos, (void*)data);
    threadCreated = true;
    return th;
}

void joinSearchThread(SearchInfo* uci)
{
    uci->stop = true;
    thrd_join(mainSearchThread, NULL);
}