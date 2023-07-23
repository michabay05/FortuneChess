#include "defs.hpp"

thrd_t mainSearchThread;
bool threadCreated = false;

int threadSearchPos(void* thData)
{
    SearchThreadData* data = (SearchThreadData*)thData;
    if (uci.debugMode)
        std::cout << "Entered a new thread!\n";

    Board* copy = new Board();
    memcpy(copy, data->board, sizeof(Board));

    searchPos(copy, hashTable, &uci);
    delete copy;

    if (uci.debugMode)
        std::cout << "Exiting the new thread!\n";
    return 0;
}

thrd_t launchSearchThread(Board* board, HashTable* tt, UCIInfo* uciInfo)
{
    SearchThreadData* data = new SearchThreadData();
    data->board = board;
    data->tt = tt;
    data->uci = uciInfo;

    thrd_t th;
    thrd_create(&th, threadSearchPos, (void*)data);
    threadCreated = true;
    return th;
}

void joinSearchThread(UCIInfo* uci)
{
    uci->stop = true;
    thrd_join(mainSearchThread, NULL);
}