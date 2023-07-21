#include "defs.hpp"

#include <chrono>
#include <cctype>

Board mainBoard;
bool uciQuit = false;
bool uciStop = false;
static bool isTimeControlled = false;

static int timeLeft = -1;
static int increment = 0;
static int movesToGo = 40;
static int moveTime = -1;
static long long startTime = 0L;
static long long stopTime = 0L;

bool uciDebugMode = false;
bool uciUseBook = true;

void uciLoop()
{
    std::string input;
    while (!uciQuit) {
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
        uciQuit = true;
    } else if (command == "ucinewgame")
        parsePos("position startpos");
    else if (command == "uci") {
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
        mainBoard.display();
    else if (command == "eval") {
        int eval = evaluatePos(mainBoard);
        std::cout << "Current eval: " << eval << "\n";
    } else if (command.compare(0, 5, "debug") == 0) {
        // The shortest legal command for 'debug' is "debug on", which is 8 characters long.
        if (command.length() < 8) {
            return;
        }
        std::string debugStatus = command.substr(5 + 1);
        uciDebugMode = (debugStatus == "on");
    } else if (command == "getbookmoves") {
        int bookMove = getBookMove(mainBoard);
        if (bookMove)
            std::cout << "Chosen book move: " << moveToStr(bookMove) << "\n";
        else
            std::cout << "No book move for the current position\n";
    } else if (command.compare(0, 9, "setoption") == 0) {
        parseSetOption(command);
    } else if (command.compare(0, 5, "perft") == 0) {
        int depth = atoi(command.substr(6).c_str());
        perftTest(mainBoard, depth, AllMoves);
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
    mainBoard = Board();
    if (command.compare(currentInd, 8, "startpos") == 0) {
        currentInd += 8 + 1; // (+ 1) for the space
        mainBoard.parseFen(FEN_POSITIONS[1]);
    } else if (command.compare(currentInd, 3, "fen") == 0) {
        currentInd += 3 + 1;
        mainBoard.parseFen(command.substr(currentInd));
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
            move = parseMoveStr(moveStr, mainBoard);
            if (move == 0)
                continue;

            // Update repetition table
            mainBoard.repetitionIndex++;
            mainBoard.repetitionTable[mainBoard.repetitionIndex] = mainBoard.key;

            makeMove(&mainBoard, move, MoveType::AllMoves);
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
    uciQuit = false;
    uciStop = false;
    isTimeControlled = false;
    timeLeft = -1;
    increment = 0;
    movesToGo = 40;
    moveTime = -1;
    // Shift pointer to the beginning of args
    int currentInd = 3;

    if (mainBoard.side == WHITE) {
        parseParamI(command.substr(currentInd), "wtime", timeLeft);
        parseParamI(command.substr(currentInd), "winc", increment);
    } else {
        parseParamI(command.substr(currentInd), "btime", timeLeft);
        parseParamI(command.substr(currentInd), "binc", increment);
    }
    parseParamI(command.substr(currentInd), "movetime", moveTime);
    parseParamI(command.substr(currentInd), "movestogo", movesToGo);

    int depth = -1;

    if (command.compare(currentInd, 8, "infinite") == 0) {
        depth = MAX_PLY;
    } else {
        parseParamI(command.substr(currentInd), "depth", depth);
    }

    if (moveTime != -1) {
        timeLeft = moveTime;
        movesToGo = 1;
    }
    startTime = getCurrTime();
    if (timeLeft != -1) {
        isTimeControlled = true;
        timeLeft /= movesToGo;
        // Just to be safe, reduce time per move by 50 ms
        if (timeLeft > 1500)
            timeLeft -= 50;
        if (timeLeft < 1500 && increment && depth == MAX_PLY)
            stopTime = startTime + increment - 50;
        else
            stopTime = startTime + increment + timeLeft;
    }

    if (depth == -1)
        depth = MAX_PLY;
    if (uciDebugMode) {
		std::cout << "time: " << timeLeft << " start: " << startTime << " stop: " << stopTime
				  << " depth: " << depth << " timeset: " << (int)isTimeControlled << "\n";
    }
    searchPos(mainBoard, depth);
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
#if 1
        tt.init(hashSizeVal);
#else
        initTTable(hashSizeVal);
#endif
    } else if (name == "Book") {
        uciUseBook = (value == "true");
    }
}

void checkUp()
{
    if (isTimeControlled && getCurrTime() >= stopTime)
        uciStop = true;
}

void printEngineID()
{
    std::cout << "id name Fortune v" << VERSION << "\n";
    std::cout << "id author michabay05\n";
}

void printEngineOptions() {
    std::cout << "option name Hash type spin default 128 min 1 max 1024\n";
    if (uciUseBook) {
		std::cout << "option name Book type check default true\n";
    }
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