#include "defs.hpp"

std::thread mainSearchThread;

int threadSearchPos(SearchThreadData* data)
{
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

std::thread launchSearchThread(Board* board, HashTable* tt, UCIInfo* uciInfo)
{
    SearchThreadData* data = new SearchThreadData();
    data->board = board;
    data->tt = tt;
    data->uci = uciInfo;

    std::thread th(threadSearchPos, data);
    return th;
}

void joinSearchThread(UCIInfo* uci)
{
    uci->stop = true;
    mainSearchThread.join();
}