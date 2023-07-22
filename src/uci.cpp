#include "defs.hpp"

#include <chrono>
#include <cctype>

UCIInfo uci;
Board board;

void uciLoop()
{
    std::string input;
    while (!uci.quit) {
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
        uci.quit = true;
        if (threadCreated)
			joinSearchThread(&uci);
    } else if (command == "stop") {
        joinSearchThread(&uci);
    } else if (command == "ucinewgame") {
        hashTable->clear();
        parsePos("position startpos");
    } else if (command == "uci") {
        printEngineID();
        printEngineOptions();
        std::cout << "uciok\n";
    } else if (command == "isready")
        std::cout << "readyok\n";
    else if (command.compare(0, 8, "position") == 0)
        parsePos(command);
    else if (command.compare(0, 2, "go") == 0)
        parseGo(command);
    else if (command == "d" || command == "display")
        board.display();
    else if (command == "eval") {
        int eval = evaluatePos(board);
        std::cout << "Current eval: " << eval << "\n";
    } else if (command.compare(0, 5, "debug") == 0) {
        // The shortest legal command for 'debug' is "debug on", which is 8 characters long.
        if (command.length() < 8) {
            return;
        }
        std::string debugStatus = command.substr(5 + 1);
        uci.debugMode = (debugStatus == "on");
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
    uci.quit = false;
    uci.stop = false;
    uci.isTimeControlled = false;
    uci.timeLeft = -1;
    uci.increment = 0;
    uci.movesToGo = 40;
    uci.moveTime = -1;
    // Shift pointer to the beginning of args
    int currentInd = 3;

    if (board.side == WHITE) {
        parseParamI(command.substr(currentInd), "wtime", uci.timeLeft);
        parseParamI(command.substr(currentInd), "winc", uci.increment);
    } else {
        parseParamI(command.substr(currentInd), "btime", uci.timeLeft);
        parseParamI(command.substr(currentInd), "binc", uci.increment);
    }
    parseParamI(command.substr(currentInd), "movetime", uci.moveTime);
    parseParamI(command.substr(currentInd), "movestogo", uci.movesToGo);


    if (command.compare(currentInd, 8, "infinite") == 0) {
        uci.searchDepth = MAX_PLY;
    } else {
        parseParamI(command.substr(currentInd), "depth", uci.searchDepth);
    }

    if (uci.moveTime != -1) {
        uci.timeLeft = uci.moveTime;
        uci.movesToGo = 1;
    }
    uci.startTime = getCurrTime();
    if (uci.timeLeft != -1) {
        uci.isTimeControlled = true;
        uci.timeLeft /= uci.movesToGo;
        // Just to be safe, reduce time per move by 50 ms
        if (uci.timeLeft > 1500)
            uci.timeLeft -= 50;
        if (uci.timeLeft < 1500 && uci.increment && uci.searchDepth == MAX_PLY)
            uci.stopTime = uci.startTime + uci.increment - 50;
        else
            uci.stopTime = uci.startTime + uci.increment + uci.timeLeft;
    }

    if (uci.searchDepth == -1)
        uci.searchDepth = MAX_PLY;
    if (uci.debugMode) {
		std::cout << "time: " << uci.timeLeft << " start: " << uci.startTime << " stop: " << uci.stopTime
				  << " depth: " << uci.searchDepth << " timeset: " << (int)uci.isTimeControlled << "\n";
    }
    mainSearchThread = launchSearchThread(&board, hashTable, &uci);
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
        hashTable->init(hashSizeVal);
    } else if (name == "Book") {
        uci.useBook = (value == "true");
    }
}

void checkUp(UCIInfo* info)
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

void printEngineOptions() {
    std::cout << "option name Hash type spin default 128 min 1 max 1024\n";
    std::cout << "option name Book type check default " << (uci.useBook ? "true" : "false") << "\n ";
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