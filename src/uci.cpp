#include "defs.hpp"

#include <cctype>
#include <chrono>

SearchInfo sInfo;
static Board board;

void uciLoop()
{
    std::string input;
    while (!sInfo.quit) {
        input = "";
        // Get input
        std::getline(std::cin, input);
        // If input is null, continue
        if (input.empty())
            continue;
        parse(input);
    }
}

void parse(const std::string& command)
{
    if (command == "quit") {
        sInfo.quit = true;
        if (threadCreated)
            joinSearchThread(&sInfo);
    } else if (command == "stop") {
        joinSearchThread(&sInfo);
    } else if (command == "ucinewgame") {
        hashTable.clear();
        parsePos("position startpos");
    } else if (command == "uci") {
        printEngineID();
        printEngineOptions();
        std::cout << "uciok\n";
    } else if (command == "run") {
        parse("position fen 8/k7/3p4/p2P1p2/P2P1P2/8/8/K7 w - - 0 1");
        parse("go infinite");
    } else if (command == "isready")
        std::cout << "readyok\n";
    else if (command.compare(0, 8, "position") == 0)
        parsePos(command);
    else if (command.compare(0, 2, "go") == 0)
        parseGo(command);
    else if (command == "d" || command == "display")
        board.display();
    else if (command == "ttstat") {
        std::cout << "TT Overwrite: " << hashTable.overWrite << "\n";
        std::cout << "TT New write: " << hashTable.newWrite << "\n";
        std::cout << "TT    Filled: " << hashTable.newWrite << " / "
                  << hashTable.entryCount
                  << "\n";
        std::cout << "TT  % Filled: "
                  << ((float)hashTable.newWrite / hashTable.entryCount) * 100.f << "% \n";
    } else if (command == "eval") {
        int eval = evaluatePos(board);
        std::cout << "Current eval: " << eval << "\n";
    } else if (command.compare(0, 5, "debug") == 0) {
        // The shortest legal command for 'debug' is "debug on", which is 8 characters long.
        if (command.length() < 8) {
            return;
        }
        std::string debugStatus = command.substr(5 + 1);
        sInfo.debugMode = (debugStatus == "on");
    } else if (command == "getbookmoves") {
        int bookMove = getBookMove(board);
        if (bookMove)
            std::cout << "Chosen book move: " << moveToStr(bookMove) << "\n";
        else
            std::cout << "No book move for the current position\n";
    } else if (command.compare(0, 9, "setoption") == 0) {
        parseSetOption(command);
    } else if (command.compare(0, 5, "perft") == 0) {
        int depth = atoi(command.substr(6).c_str());
        perftTest(board, depth, AllMoves);
    } else if (command == "help")
        printHelpInfo();
    else
        std::cout << "Unknown command: " << command << "\n";
}

void parsePos(const std::string& command)
{
    if (command.empty())
        return;
    // Shift pointer to the beginning of args
    int currentInd = 9;
    // Reset board before setting piece up
    board = Board();
    if (command.compare(currentInd, 8, "startpos") == 0) {
        currentInd += 8 + 1; // (+ 1) for the space
        board.parseFen(FEN_POSITIONS[1]);
    } else if (command.compare(currentInd, 3, "fen") == 0) {
        currentInd += 3 + 1;
        board.parseFen(command.substr(currentInd));
    }
    currentInd = (int)command.find("moves", currentInd);
    if (currentInd == std::string::npos)
        return;
    currentInd += 6;
    std::string moveStr;
    int move;
    for (int i = currentInd; i <= command.length(); i++) {
        if (std::isdigit(command[i]) || std::isalpha(command[i]))
            moveStr += command[i];
        else {
            move = parseMoveStr(moveStr, board);
            if (move == 0)
                continue;

            // Update repetition table
            board.repetitionIndex++;
            board.repetitionTable[board.repetitionIndex] = board.key;

            makeMove(&board, move, MoveType::AllMoves);
            moveStr = "";
        }
    }
}

long long getCurrTime()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::high_resolution_clock::now().time_since_epoch())
        .count();
}

void parseGo(const std::string& command)
{
    // Reset time control related variables
    sInfo.quit = false;
    sInfo.stop = false;
    sInfo.isTimeControlled = false;
    sInfo.timeLeft = -1;
    sInfo.increment = 0;
    sInfo.movesToGo = 40;
    sInfo.moveTime = -1;
    // Shift pointer to the beginning of args
    int currentInd = 3;

    if (board.side == WHITE) {
        parseParamI(command.substr(currentInd), "wtime", sInfo.timeLeft);
        parseParamI(command.substr(currentInd), "winc", sInfo.increment);
    } else {
        parseParamI(command.substr(currentInd), "btime", sInfo.timeLeft);
        parseParamI(command.substr(currentInd), "binc", sInfo.increment);
    }
    parseParamI(command.substr(currentInd), "movetime", sInfo.moveTime);
    parseParamI(command.substr(currentInd), "movestogo", sInfo.movesToGo);

    if (command.compare(currentInd, 8, "infinite") == 0) {
        sInfo.searchDepth = MAX_PLY;
    } else {
        parseParamI(command.substr(currentInd), "depth", sInfo.searchDepth);
    }

    if (sInfo.moveTime != -1) {
        sInfo.timeLeft = sInfo.moveTime;
        sInfo.movesToGo = 1;
    }
    sInfo.startTime = getCurrTime();
    if (sInfo.timeLeft != -1) {
        sInfo.isTimeControlled = true;
        sInfo.timeLeft /= sInfo.movesToGo;
        // Just to be safe, reduce time per move by 50 ms
        if (sInfo.timeLeft > 1500)
            sInfo.timeLeft -= 50;
        if (sInfo.timeLeft < 1500 && sInfo.increment && sInfo.searchDepth == MAX_PLY)
            sInfo.stopTime = sInfo.startTime + sInfo.increment - 50;
        else
            sInfo.stopTime = sInfo.startTime + sInfo.increment + sInfo.timeLeft;
    }

    if (sInfo.searchDepth == -1)
        sInfo.searchDepth = MAX_PLY;
    if (sInfo.debugMode) {
        std::cout << "time: " << sInfo.timeLeft << " start: " << sInfo.startTime
                  << " stop: " << sInfo.stopTime << " depth: " << sInfo.searchDepth
                  << " timeset: " << (int)sInfo.isTimeControlled << "\n";
    }
    mainSearchThread = launchSearchThread(&board, &hashTable, &sInfo, &mainSearchTable);
}

void parseParamI(const std::string& cmdArgs, const std::string& argName, int& output)
{
    size_t currentIndex, nextSpaceInd;
    std::string param;
    if ((currentIndex = cmdArgs.find(argName)) != std::string::npos &&
        cmdArgs.length() >= argName.length() + 1) {
        param = cmdArgs.substr(currentIndex + argName.length() + 1);
        if ((nextSpaceInd = param.find(" ")) != std::string::npos)
            param = param.substr(0, nextSpaceInd);
        output = std::stoi(param);
    }
}

void parseParam(const std::string& cmdArgs, const std::string& argName, std::string& output)
{
    size_t currentIndex, nextSpaceInd;
    std::string param;
    if ((currentIndex = cmdArgs.find(argName)) != std::string::npos &&
        cmdArgs.length() >= argName.length() + 1) {
        param = cmdArgs.substr(currentIndex + argName.length() + 1);
        if ((nextSpaceInd = param.find(" ")) != std::string::npos)
            param = param.substr(0, nextSpaceInd);
        output = param;
    }
}

void parseSetOption(const std::string& command)
{
    int currentIndex = 9;
    std::string name = "";
    std::string value = "";
    parseParam(command.substr(currentIndex), "name", name);
    parseParam(command.substr(currentIndex), "value", value);
    if (name.empty() || value.empty()) {
        return;
    }

    if (name == "Hash") {
        int hashSizeVal = std::stoi(value);
        hashTable.init(hashSizeVal);
    } else if (name == "Book") {
        sInfo.useBook = (value == "true");
    }
}

void checkUp(SearchInfo* info)
{
    if (info->isTimeControlled && getCurrTime() >= info->stopTime) {
        info->stop = true;
    }
}

void printEngineID()
{
    std::cout << "id name Fortune v" << VERSION << "\n";
    std::cout << "id author michabay05\n";
}

void printEngineOptions()
{
    std::cout << "option name Hash type spin default 128 min 1 max 1024\n";
    std::cout << "option name Book type check default " << (sInfo.useBook ? "true" : "false") << "\n";
}

void printHelpInfo()
{
    std::cout << "              Command name                 |         Description\n";
    std::cout << "---------------------------------------------------------------------"
                 "----------------------------------\n";
    std::cout << "                  uci                      |    Prints engine info "
                 "and 'uciok'\n";
    std::cout << "              isready                      |    Prints 'readyok' if "
                 "the engine is ready\n";
    std::cout << "    position startpos                      |    Set board to "
                 "starting position\n";
    std::cout << "    position startpos moves <move1> ...    |    Set board to "
                 "starting position then playing following moves\n";
    std::cout << "   position fen <FEN>                      |    Set board to a "
                 "custom FEN\n";
    std::cout << "   position fen <FEN> moves <move1> ...    |    Set board to a "
                 "custom FEN then playing following moves\n";
    std::cout << "     go depth <depth>                      |    Returns the best "
                 "move after search for given amount of depth\n";
    std::cout << "     go movetime <time>                    |    Returns the best "
              << "move given the time for a single move\n";
    std::cout << "go (wtime/btime) <time>(winc/binc) <time>  |    Returns the best "
              << "move given the total amount of time for a move with increment\n";
    std::cout << "                 quit                      |    Exit the UCI mode\n";
    std::cout << "\n------------------------------------ EXTENSIONS "
              << "----------------------------------------\n";
    std::cout << "              display                      |    Display board\n";
    std::cout << "        perft <depth>                      |    Calculate the total "
              << "number of moves from a position for a given depth\n";
    std::cout << "                eval                       |    Returns the evaluation (in "
                 "centipawns) of the current position\n";
}