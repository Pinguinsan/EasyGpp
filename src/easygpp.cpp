/***********************************************************************
*    easygpp.cpp:                                                      *
*    Interface program for quickly using gcc/g++                       *
*    Copyright (c) 2016 Tyler Lewis                                    *
************************************************************************
*    This is a source code file for EasyGpp:                           *
*    https://github.com/Pinguinsan/EasyGpp                             *
*    The source code is released under the GNU LGPL                    *
*    This file holds the main logic for the EasyGpp program            *
*    It is used to quickly compile programs with gcc/g++ using         *
*    minimal command line switches, when quick prototyping of concepts *
*    is of the essence. It also allows for "build-and-run" capability, *
*    and can recognize header files and automatically include relevant *
*    libraries using a configuration file. As well, it also allows one *
*    to quickly edit a file if it fails to compile, using either a     *
*    known or custom editor by adding them to the configuration file   *
*                                                                      *
*    You should have received a copy of the GNU Lesser General         *
*    Public license along with libraryprojects                         *
*    If not, see <http://www.gnu.org/licenses/>                        *
***********************************************************************/

#include <unistd.h>
#include <signal.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <string>
#include <vector>
#include <sstream>
#include <map>
#include <set>
#include <utility>
#include <iterator>
#include <future>

#include <datetime.h>
#include <generalutilities.h>
#include <systemcommand.h>
#include <fileutilities.h>

using namespace GeneralUtilities;
using namespace FileUtilities;
using namespace DateTime;

const std::string PROGRAM_NAME = "easyg++";
const std::string AUTHOR_NAME = "Tyler Lewis";
const int SOFTWARE_MAJOR_VERSION = 0;
const int SOFTWARE_MINOR_VERSION = 1;
const int SOFTWARE_PATCH_VERSION = 7;

#ifdef __GNUC__
    const int GCC_MAJOR_VERSION = __GNUC__;
    const int GCC_MINOR_VERSION = __GNUC_MINOR__;
    const int GCC_PATCH_VERSION = __GNUC_PATCHLEVEL__;
#else
    #error "The compiler must define __GNUC__ to use this program, but the compiler does not have it defined"
#endif

enum class MovementState {
    Walking,
    Flying
};

struct Animal {
    MovementState movementState() {return MovementState::Walking;}
};

Animal Pigs;

static const char PATH_DELIMITER = ':';

static const std::vector<std::string> KNOWN_EDITOR_BINARIES = {"notepad", "vim", "nano", "emacs", "mousepad", "leafpad", "code", "sublime_text", "vscode"};
static const std::vector<std::string> KNOWN_LIBRARY_BINARIES = {"datetime.h", "generalutilities.h", "systemcommand.h", "fileutilities.h", "templateobject.h", "crypto.h", "mathutilities.h", "tjlutils.h"};
static const std::vector<std::string> STATIC_SWITCHES{"-st", "--st", "--static", "-static"};
static const std::vector<std::string> STANDARD_SWITCHES{"-s", "--s", "-standard", "--standard"};    
static const std::vector<std::string> HELP_SWITCHES{"-h", "--h", "-help", "--help"};
static const std::vector<std::string> VERSION_SWITCHES{"v", "-v", "--v", "-version", "--version"};
static const std::vector<std::string> GCC_SWITCHES{"-c", "--c", "-cc", "--cc", "-gcc", "--gcc"};
static const std::vector<std::string> NAME_SWITCHES{"-n", "--n", "-name", "--name"};
static const std::vector<std::string> NO_DEBUG_SWITCHES{"-nd", "--nd", "-nodebug", "--nodebug", "-ndebug", "--ndebug"};    
static const std::vector<std::string> BUILD_AND_RUN_SWITCHES{"-r", "--r", "-run", "--run", "-buildandrun", "--buildandrun"};
static const std::vector<std::string> LIBRARY_OVERRIDE_SWITCHES{"-lo", "--lo", "-loverride", "--loverride"};
static const std::vector<std::string> VERBOSE_OUTPUT_SWITCHES{"-e", "--e", "-verbose", "--verbose"};
static const std::vector<std::string> INCLUDE_PATH_SWITCHES{"-i", "--i", "-include", "--include", "-includedir", "--includedir", "-includepath", "--includepath"};
static const std::vector<std::string> LIBRARY_PATH_SWITCHES{"-l", "--l", "-libdir", "--libdir", "-libpath", "--libpath", "-library", "--library"};
static const std::string STANDARD_PROMPT_STRING = "Please enter a selection: ";
static const std::string DEFAULT_CPP_COMPILER_STANDARD{"-std=c++14"};
static const std::string DEFAULT_C_COMPILER_STANDARD{"-std=c14"};
static const std::string EDITOR_IDENTIFIER{"addeditor("};
static const std::string LIBRARY_IDENTIFIER{"addlibrary("};
static const std::string CONFIGURATION_FILE_NAME{"easygpp.config"};
static const std::string DEFAULT_CONFIGURATION_FILE{static_cast<std::string>(getenv("HOME")) + "/.local/easygpp/" + CONFIGURATION_FILE_NAME};
static const std::string BACKUP_CONFIGURATION_FILE{"/usr/share/easygpp/" + CONFIGURATION_FILE_NAME};
static const std::string LAST_CHANCE_CONFIGURATION_FILE{"/opt/GitHub/EasyGpp/config/" + CONFIGURATION_FILE_NAME};
static const std::vector<std::string> PTHREAD_IDENTIFIERS{"<thread>", "<future>"};

static std::vector<std::string> extraEditors;
static std::map<std::string, std::string> libraryToHeaderMap;
static std::map<std::string, std::string> editorPrograms; 

void displayHelp();
void displayVersion();
void interruptHandler(int signalNumber);

bool isGeneralSwitch(const std::string &stringToCheck);
bool isLibrarySwitch(const std::string &stringToCheck);
bool isSourceCodeFile(const std::string &stringToCheck);
bool isSwitch(const std::string &stringToCheck, const std::vector<std::string> &switches);
bool isSwitch(const std::string &stringToCheck, const std::string &switchToCheck);

std::string determineOverrideStandard(const std::string &stringToDetermine);
std::map<std::string, std::string> getEditorProgramPaths();

bool matchesKnownEditorBinaries(const std::string &binaryNameToCheck);
std::vector<std::string> readConfigurationFile();

int main(int argc, char *argv[])
{
    using namespace FileUtilities;
   //Signal handling stuff
   struct sigaction sigIntHandler;
   sigIntHandler.sa_handler = interruptHandler;
   sigemptyset(&sigIntHandler.sa_mask);
   sigIntHandler.sa_flags = 0;
   sigaction(SIGINT, &sigIntHandler, NULL);
   //End signal handling

    std::cout << std::endl;
    for (int i = 0; i < argc; i++) { 
        if (isSwitch(static_cast<std::string>(argv[i]), HELP_SWITCHES)) {
            displayHelp();
            return 0;
        } else if (isSwitch(static_cast<std::string>(argv[i]), VERSION_SWITCHES)) {
            displayVersion();
            return 0;
        }
    }
    displayVersion();
    
    bool gccFlag{false};
    bool buildAndRun{false};
    bool verboseOutput{false};
    bool libraryOverride{false};
    bool editorProgramsRetrieved{false};
    bool configurationFileRead{false};
    std::string compilerType{"g++"};
    std::string gnuDebugSwitch{" -ggdb"};
    std::string executableName{""};
    std::string staticSwitch{""};
    std::string staticLibGCCSwitch{""};
    std::vector<std::string> sourceCodeFiles;
    std::vector<std::string> generalSwitches;
    std::set<std::string> includePaths;
    std::set<std::string> libraryPaths;
    std::set<std::string> librarySwitches;
    std::string compilerStandard{DEFAULT_CPP_COMPILER_STANDARD};

    std::future<std::vector<std::string>> configFileTask = std::async(std::launch::async, readConfigurationFile);
    std::future<std::map<std::string, std::string>> editorProgramsTask = std::async(std::launch::async, getEditorProgramPaths);

    for (int i = 0; i < argc; i++) {
        if (isSwitch(static_cast<std::string>(argv[i]), GCC_SWITCHES)) {
            gccFlag = true;
            compilerType = "gcc";
            compilerStandard = DEFAULT_C_COMPILER_STANDARD;
        } else if (isSwitch(static_cast<std::string>(argv[i]), NAME_SWITCHES)) {
            if (argv[i+1]) {
                executableName = static_cast<std::string>(argv[i+1]);
                i++;
            } else {
                std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but an output filename was not specified" << std::endl;
                std::cout << "    Falling back on default executable name being .c/.cpp file name" << std::endl << std::endl;
            }
        } else if (isSwitch(static_cast<std::string>(argv[i]), STANDARD_SWITCHES)) {
            if (argv[i+1]) {
                std::string tempCompilerStandard{determineOverrideStandard(static_cast<std::string>(argv[i+1]))};
                if (tempCompilerStandard == "") {
                    std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but standard " << tQuoted(argv[i+1]) << " is not valid" << std::endl;
                    std::cout << "    Falling back on default compiler standard of " << tQuoted(DEFAULT_CPP_COMPILER_STANDARD) << std::endl << std::endl;
                } else {
                    compilerStandard = tempCompilerStandard;
                    i++;
                }
            } else {
                std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but a standard was not specified" << std::endl;
                std::cout << "    Falling back on default compiler standard of " << tQuoted(DEFAULT_CPP_COMPILER_STANDARD) << std::endl << std::endl;
            }
        } else if (isSwitch(static_cast<std::string>(argv[i]), NO_DEBUG_SWITCHES)) {
            gnuDebugSwitch = "";
        } else if (isSwitch(static_cast<std::string>(argv[i]), STATIC_SWITCHES)) {
            staticSwitch = " -static ";
            staticLibGCCSwitch = " -static-libgcc ";
        } else if (isSwitch(static_cast<std::string>(argv[i]), VERBOSE_OUTPUT_SWITCHES)) {
            verboseOutput = true;
        } else if (isSwitch(static_cast<std::string>(argv[i]), BUILD_AND_RUN_SWITCHES)) {
            buildAndRun = true;
        } else if (isSwitch(static_cast<std::string>(argv[i]), LIBRARY_OVERRIDE_SWITCHES)) {
            libraryOverride = true;
        } else if (isSwitch(static_cast<std::string>(argv[i]), INCLUDE_PATH_SWITCHES)) {
            if (argv[i+1]) {
                std::string tempSwitchDir{static_cast<std::string>(argv[i+1])};
                if (!directoryExists(tempSwitchDir)) {
                    std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but include path " << tQuoted(argv[i+1]) << " is not a valid directory" << std::endl;
                    std::cout << "    Skipping include path option" << std::endl << std::endl;
                } else {
                    includePaths.emplace(tempSwitchDir);
                    i++;
                }
            } else {
                std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but no directory was specified" << std::endl;
                std::cout << "    Skipping include path option" << std::endl << std::endl;
            } 
        } else if (isSwitch(static_cast<std::string>(argv[i]), LIBRARY_PATH_SWITCHES)) {
            if (argv[i+1]) {
                std::string tempSwitchDir{static_cast<std::string>(argv[i+1])};
                if (!directoryExists(tempSwitchDir)) {
                    std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but library path " << tQuoted(argv[i+1]) << " is not a valid directory" << std::endl;
                    std::cout << "    Skipping library path option" << std::endl << std::endl;
                } else {
                    libraryPaths.emplace(tempSwitchDir);
                    i++;
                }
            } else {
                std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but no directory was specified" << std::endl;
                std::cout << "    Skipping library path option" << std::endl << std::endl;
            } 
        } else if (isSourceCodeFile(static_cast<std::string>(argv[i]))) {
            sourceCodeFiles.emplace_back(static_cast<std::string>(argv[i]));
        } else if (isLibrarySwitch(static_cast<std::string>(argv[i]))) {
            librarySwitches.emplace(static_cast<std::string>(argv[i]));
        } else if (isGeneralSwitch(static_cast<std::string>(argv[i]))) {
            generalSwitches.emplace_back(static_cast<std::string>(argv[i]));
        }
    }
    
    if (executableName == "") {
        if (sourceCodeFiles.empty()) {
            std::cout << "ERROR: No source code files specified, exiting " << PROGRAM_NAME << std::endl << std::endl;
            displayHelp();
            return -1;
        }
        if (directoryExists("bin/")) {
            if (verboseOutput) {
                std::cout <<"WARNING: No executable file name specified, but a directory named " << tQuoted("bin/") << " exists, so the default of the first .c/.cpp file name will be appended to that as the executable name" << std::endl << std::endl;
            }
            std::string sourceCodeName = *std::begin(sourceCodeFiles);
            size_t decimalPosition = sourceCodeName.find(".c");
            if (decimalPosition == std::string::npos) {
                std::cout << "ERROR: No .c or .cpp file listed, exiting " << PROGRAM_NAME << std::endl;
                displayHelp();
                return -1;
            } 
            while (sourceCodeName.find("/") != std::string::npos) {
                size_t foundPosition{sourceCodeName.find("/")};
                sourceCodeName = sourceCodeName.substr(foundPosition+1);
            }
            decimalPosition = sourceCodeName.find(".c");
            executableName = "bin/" + sourceCodeName.substr(0, decimalPosition);
        } else {
            if (verboseOutput) {
                std::cout << "WARNING: No executable file name specified, falling back on default executable name being first .c/.cpp file name" << std::endl << std::endl;
            }
            std::string sourceCodeName{sourceCodeFiles.at(0)};
            size_t decimalPosition{sourceCodeName.find(".c")};
            if (decimalPosition == std::string::npos) {
                std::cout << "No .c or .cpp file listed, exiting" << std::endl;
                displayHelp();
                return -1;
            }
            decimalPosition = sourceCodeName.find(".c");
            executableName = sourceCodeName.substr(0, sourceCodeName.find(".c"));
        }
    }
    
    while (Pigs.movementState() != MovementState::Flying) {
        //compilerType is "g++" by default, but gets overriden by the -c switch
        //gnuDebugSwitch will be " -ggdb " by default unless overriden by the -nd switch
        //staticSwitch will be an empty string unless it is set using the -st switch
        //staticLibGCCSwitch will be an empty string unless it is set using the -st switch
        SystemCommand systemCommand{compilerType + " -Wall" + gnuDebugSwitch + staticSwitch + staticLibGCCSwitch};
        if (gccFlag) {
            for (auto &it : sourceCodeFiles) {
                if (it.find(".cpp") != std::string::npos) {
                    if (verboseOutput) {
                        std::cout << "WARNING: the GCC switch was used but the source code file " << tQuoted(it) << " is a .cpp file" << std::endl << std::endl;
                    }
                }
            }
        }
        for (auto &it : generalSwitches) {
            systemCommand += (" " + it);
        }
        for (auto &it : includePaths) {
            systemCommand += (" -I " + tQuoted(it));
        }
        for (auto &it : libraryPaths) {
            systemCommand += (" -L " + tQuoted(it));
        }
        if (directoryExists(executableName)) {
            if (verboseOutput) {
                std::cout << "WARNING: a directory was specified as the output filename, so the default executable name (the first .c/.cpp file name) has been appended to the directory" << std::endl << std::endl;
            }
            std::string sourceCodeName = *std::begin(sourceCodeFiles);
            if ((executableName.substr(executableName.length()-1) != "/") && (executableName.substr(executableName.length()-1)!= "\\")) {
                executableName += "/";
            }
            while (sourceCodeName.find("/") != std::string::npos) {
                size_t found = sourceCodeName.find("/");
                sourceCodeName = sourceCodeName.substr(found+1);
            }
            size_t decimalPosition = sourceCodeName.find(".c");
            executableName += sourceCodeName.substr(0, decimalPosition);
        }
        systemCommand += (" " + compilerStandard + " -o " + tQuoted(executableName));
        for (auto &it : sourceCodeFiles) {
            systemCommand += (" " + tQuoted(it) + " ");
        }
        if (staticSwitch != "") {
            if (verboseOutput) {
                std::cout << "WARNING: using the " << tQuoted("-static") << " switch can be very slow on some systems, consider removing it if it takes too long to compile your project" << std::endl << std::endl;
            }
        } 
        std::vector<std::string> configFileTaskOutput;
        if (!configurationFileRead) {
            configFileTask.wait();
            configFileTaskOutput = configFileTask.get();
            configurationFileRead = true;
        }
        for (auto &it : configFileTaskOutput) {
            std::cout << it << std::endl;
        }
        if (!libraryOverride) {
            for (auto &it : sourceCodeFiles) {
                std::ifstream readFromFile;
                readFromFile.open(it);
                if (readFromFile.is_open()) {
                    std::string rawString{""};
                    while (std::getline(readFromFile, rawString)) {
                        for (auto &mapIt : libraryToHeaderMap) {
                            if (rawString.find(mapIt.first) != std::string::npos) {
                                if ((mapIt.second.find("-l") != std::string::npos) || (mapIt.second[0] == '-')) {
                                    auto result{librarySwitches.emplace(mapIt.second)};
                                    if (result.second) {
                                        if (verboseOutput) {
                                            std::cout << "NOTE: library " << tQuoted(mapIt.second) << " was associated with header file " << tQuoted(mapIt.first) << " from configuration file, so the library has been added to the command line arguments (this behavior can be disabled with the " << tQuoted("--l") << " switch)" << std::endl << std::endl;
                                        }
                                    }
                                } else {
                                    auto result{librarySwitches.emplace("-l" + mapIt.second)};
                                    if (result.second) {
                                        if (verboseOutput) {
                                            std::cout << "NOTE: library " << tQuoted(mapIt.second) << " was associated with header file " << tQuoted(mapIt.first) << " from configuration file, so the library has been added to the command line arguments (this behavior can be disabled with the " << tQuoted("--l") << " switch)" << std::endl << std::endl;
                                        }
                                    }
                                }
                            }
                        }
                        #ifdef __linux__
                            for (std::vector<std::string>::const_iterator iter = PTHREAD_IDENTIFIERS.begin(); iter != PTHREAD_IDENTIFIERS.end(); iter++) {
                                if (rawString.find(*iter) != std::string::npos) {
                                    librarySwitches.emplace("-lpthread");
                                }
                            }
                        #endif
                    }
                    readFromFile.close();
                } else {
                    if (verboseOutput) {
                        std::cout << "WARNING: could not open source file " << tQuoted(it) << " for additional library matching, skipping search" << std::endl << std::endl;
                    }
                }
            }    
        }
        for (auto &it : librarySwitches) {
            systemCommand += (" " + it);
        }
        for (auto &it : configFileTaskOutput) {
            std::cout << it << std::endl;
        }
        std::cout << "Executing below statement:" << std::endl;
        std::cout << "    " << systemCommand.command() << std::endl << std::endl;
        systemCommand.executeWithoutPipe();
        if (!systemCommand.hasError()) {
            std::string outputText{ ((sourceCodeFiles.size() > 1) ? "Source files: " : "Source file: ") };
            std::cout << outputText;
            for (auto &it : sourceCodeFiles) {
                if (it == *sourceCodeFiles.begin()) {
                    std::cout << tQuoted(it) << std::endl;
                } else {
                    std::cout << whitespace(outputText.length()) << tQuoted(it) << std::endl;
                }
            }
            std::cout << std::endl << "compiled successfully to make executable file \"" << executableName;
            if (executableName.find(".exe") != std::string::npos) {
                std::cout << "\"" << std::endl;
            } else {
                #ifdef __CYGWIN__
                    std::cout << "(.exe)\"" << std::endl;
                #else
                    std::cout << "\"" << std::endl;
                #endif
            }
            std::cout << std::endl;
            if (buildAndRun) {
                std::cout << "Either enter command line arguments to run compiled program (leave blank to run without args), or press CTRL+C to quit:" << std::endl;
                std::cout << tQuoted("./" + executableName) << " ";
                std::string commandLineArgs{""};
                std::getline(std::cin, commandLineArgs);
                SystemCommand executeProgram;
                if ((isWhitespace(commandLineArgs)) || (commandLineArgs == "")) {
                    executeProgram.setCommand(tQuoted("./" + executableName));
                } else {
                    executeProgram.setCommand(tQuoted("./" + executableName) + " " + tQuoted(commandLineArgs));
                }
                std::cout << std::endl << "Executing below statement:" << std::endl;
                std::cout << "    " << executeProgram.command() << std::endl << std::endl;
                executeProgram.executeWithoutPipe();
                std::cout << executableName << " exited with a return value of " <<  executeProgram.returnValue() << std::endl;
            }
            return 0;
        }
        //Control only reaches here if gcc/g++ doesn't compile successfully
        std::cout << std::endl;
        std::cout << (gccFlag ? "gcc" : "g++") << " returned an error compiling. Would you like to edit a file? Select from below: " << std::endl << std::endl;
        int i{1};
        for (auto &it : sourceCodeFiles) {
            std::string tempSourceName{it};
            while (tempSourceName.find("/") != std::string::npos) {
                size_t foundPosition{tempSourceName.find("/")};
                tempSourceName = tempSourceName.substr(foundPosition+1);
            }
            std::cout << i << ".) edit " << tempSourceName << std::endl;
            i++;
        }
        std::string tempProjectName{executableName};
        while (tempProjectName.find("/") != std::string::npos) {
            size_t foundPosition{tempProjectName.find("/")};
            tempProjectName = tempProjectName.substr(foundPosition+1);
        }
        std::cout << i << ".) recompile project " << tempProjectName << std::endl;
        std::cout << i + 1 << ".) do not edit, quit " << PROGRAM_NAME << std::endl << std::endl;
        bool userReplied{false};
        std::string userReplyString{""};
        int userReply{0};
        int recompileOption{i};
        int exitOption{i+1};
        while (!userReplied) {
            std::cout << STANDARD_PROMPT_STRING;
            std::getline(std::cin, userReplyString);
            try {
                userReply = stoi(userReplyString);
            } catch (std::exception &e) {
                std::cout << tQuoted(userReplyString) << " wasn't one of the selections, please enter a number between (inclusive) 1 and " << exitOption << ", or press CTRL+C to quit" << std::endl << std::endl;
                continue;
            }
            if ((userReply <= (i+1)) && (userReply > 0)) {
                userReplied = true;
            } else {
                std::cout << tQuoted(userReplyString) << " wasn't one of the selections, please enter a number between (inclusive) 1 and " << exitOption << ", or press CTRL+C to quit" << std::endl << std::endl;
            }
        }
        if (userReply == exitOption) {
            return 0;
        } else if (userReply == recompileOption) {
            continue;
        }
        std::string sourceCodeEditPath = sourceCodeFiles.at(userReply-1);
        userReplied = false;
        userReplyString = "";
        userReply = 0;
        i = 1;
        std::vector<std::string> optionCopy;
        if (!editorProgramsRetrieved) {
            editorProgramsTask.wait();
            editorPrograms = editorProgramsTask.get();
            editorProgramsRetrieved = true;
        }
        if (editorPrograms.empty()) {
            std::cout << "Sorry, it looks like you don't have any known editor programs, exiting " << PROGRAM_NAME << std::endl;
            return 2;
        }
        std::cout << std::endl << "Which editor would you like to use?" << std::endl;
        for (auto &it : editorPrograms) {
            std::cout << i << ".) " << it.first << std::endl;
            optionCopy.emplace_back(it.first);
            i++;
        }
        std::string tempSourcePath{sourceCodeEditPath};
        while (tempSourcePath.find("/") != std::string::npos) {
            size_t foundPosition{tempSourcePath.find("/")};
            tempSourcePath = tempSourcePath.substr(foundPosition+1);
        }
        std::cout << i << ".) recompile project " << tempProjectName << std::endl;
        std::cout << i+1 << ".) do not edit " << tempSourcePath << " or recompile project " << tQuoted(tempProjectName) << ", quit " << PROGRAM_NAME << std::endl << std::endl;
        recompileOption = i;
        exitOption = i+1;
        while (!userReplied) {
            std::cout << STANDARD_PROMPT_STRING;
            std::getline(std::cin, userReplyString);
            try {
                userReply = std::stoi(userReplyString);
            } catch (std::exception &e) {
                std::cout << tQuoted(userReplyString) << " wasn't one of the selections, please enter a number between (inclusive) 1 and " << exitOption << ", or press CTRL+C to quit" << std::endl << std::endl;
                continue;
            }
            if ((userReply <= i) && (userReply > 0)) {
                userReplied = true;
            } else {
                std::cout << tQuoted(userReplyString) << " wasn't one of the above number selections, please enter a number between 1 and " << exitOption << ", or press CTRL+C to quit" << std::endl << std::endl;
            }
        }
        if (userReply == exitOption) {
            return 0;
        } else if (userReply == recompileOption) {
            continue;
        }
        std::string editorProgramPath = optionCopy.at(userReply-1);
        optionCopy.clear();
        systemCommand.setCommand(editorProgramPath + " " + tQuoted(sourceCodeEditPath));
        systemCommand.printCommand();
        systemCommand.executeWithoutPipe();
    }
}

void displayHelp() 
{
    std::cout << "Usage: " << PROGRAM_NAME << " [options] [argument]" << std::endl << std::endl;
    std::cout << "Options: " << std::endl;
    std::cout << "    -c, --c, -cc, --cc, -gcc, --gcc: Use gcc instead of g++" << std::endl;
    std::cout << "    -n, --n, -name, --name: Specify name for executable (do not include " << tQuoted(".exe") << ")" << std::endl;
    std::cout << "        Note: must include a name after this without a leading " << tQuoted("-") << std::endl;
    std::cout << "    -s, --s, -standard, --standard: Override the default of -std=c++14/-std=c14" << std::endl;
    std::cout << "        Note: must include a standard following this (eg " << tQuoted("-std=c++11") << ")" << std::endl;
    std::cout << "    -st, --st, -static, --static: Link the program statically against the included libraries" << std::endl;
    std::cout << "    -h, --h, -help, --help: Display this help text" << std::endl;
    std::cout << "    -v, --v, -version, --version: Display the version" << std::endl;
    std::cout << "    -nd, --nd, -nodebug, --nodebug: Compile the program without debug information for gdb" << std::endl;
    std::cout << "    -r, --r, -run, --run: Compile the program and run after successfully compiling" << std::endl;
    std::cout << "    -lo, --lo, -loverride, --loverride: Override the default behavior to automatically add libraries, as specified by the configuration file " << std::endl;
    std::cout << "    -i, --i, -include, --include: Add an additional include path" << std::endl;
    std::cout << "    -l, --l, -libdir, --libdir: Add an additional library path" << std::endl;
    std::cout << "Normal gcc and g++ switches can be included as well (-Werror, -03, etc)" << std::endl;
    std::cout << "Default g++ switches used: -Wall -std=c++14" << std::endl;
    std::cout << "Argument: Source code that you want to compile" << std::endl;
    std::cout << "Example: " << std::endl;
    std::cout << "    Command line input: easygcc -Werror -n testProgram testProgram.cpp" << std::endl;
    std::cout << "    Output:" << std::endl; 
    std::cout << "        Executing below statement:" << std::endl;
    std::cout << "            " << tQuoted("g++ -Wall -Werror -std=c++14 -o testProgram testProgram.cpp testOtherFile.cpp") << std::endl;
    std::cout << "        Source files " << tQuoted("testProgram.cpp") << std::endl;
    std::cout << "                     " << tQuoted("testOtherFile.cpp") << std::endl;
    std::cout << "        compiled successfully to make executable file \"testProgram";
    #ifdef __CYGWIN__
        std::cout << "(.exe)\"" << std::endl;
    #else
        std::cout << "\"" << std::endl;
    #endif
    std::cout << std::endl;
}

void displayVersion() 
{
    std::cout << PROGRAM_NAME << ", v" << SOFTWARE_MAJOR_VERSION << "." << SOFTWARE_MINOR_VERSION << "." << SOFTWARE_PATCH_VERSION << std::endl;
    std::cout << "Written by " << AUTHOR_NAME << ", " << currentYear() << std::endl;
    std::cout << "Built with g++ v" << GCC_MAJOR_VERSION << "." << GCC_MINOR_VERSION << "." << GCC_PATCH_VERSION << ", " << dateStampMDY() << std::endl << std::endl;
}

bool isSwitch(const std::string &stringToCheck, const std::vector<std::string> &switches)
{
    std::string copyString{stringToCheck};
    std::transform(copyString.begin(), copyString.end(), copyString.begin(), ::tolower);
    for (auto &it : switches) {
        std::string switchCopy{it};
        std::transform(switchCopy.begin(), switchCopy.end(), switchCopy.begin(), ::tolower);
        if (copyString == switchCopy) {
            return true;
        }
    }
    return false;
}

bool isSwitch(const std::string &stringToCheck, const std::string &switchToCheck)
{
    std::string copyString{stringToCheck};
    std::string switchCopy{switchToCheck};
    std::transform(copyString.begin(), copyString.end(), copyString.begin(), ::tolower);
    std::transform(switchCopy.begin(), switchCopy.end(), switchCopy.begin(), ::tolower);
    
    return (copyString == switchCopy);
}

bool isGeneralSwitch(const std::string &stringToCheck) 
{
    if (stringToCheck.length() == 0) {
        return false;
    }
    return (stringToCheck[0] == '-');
}

bool isLibrarySwitch(const std::string &stringToCheck)
{
    if (stringToCheck.length() < 2) {
        return false;
    } else if (stringToCheck.substr(0,2) == "-l") {
        return true;
    } else {
        return false;
    }
}

bool isSourceCodeFile(const std::string &stringToCheck) 
{
    std::string tempStringToCheck = stringToCheck;
    std::transform(tempStringToCheck.begin(), tempStringToCheck.end(), tempStringToCheck.begin(), ::tolower);
    return (tempStringToCheck.find(".c") != std::string::npos);
}

std::string determineOverrideStandard(const std::string &stringToDetermine) 
{
    std::string tempStringToDetermine = stringToDetermine;
    std::transform(tempStringToDetermine.begin(), tempStringToDetermine.end(), tempStringToDetermine.begin(), ::tolower);
    
    if (tempStringToDetermine.find("c++17") != std::string::npos) {
        return "-std=c++17";
    } else if (tempStringToDetermine.find("c++14") != std::string::npos) {
        return "-std=c++14";
    } else if (tempStringToDetermine.find("c++11") != std::string::npos) {
        return "-std=c++11";
    } else if (tempStringToDetermine.find("gnu++11") != std::string::npos) {
        return "-std=gnu++11";
    } else if (tempStringToDetermine.find("c++0x") != std::string::npos) {
        return "-std=c++0x";
    } else if (tempStringToDetermine.find("c++03") != std::string::npos) {
        return "-std=c++03";
    } else if (tempStringToDetermine.find("gnu11") != std::string::npos) {
        return "-std=gnu11";
    } else if (tempStringToDetermine.find("c11") != std::string::npos) {
        return "-std=c11";
    } else if (tempStringToDetermine.find("gnu03") != std::string::npos) {
        return "-std=gnu03";
    } else if (tempStringToDetermine.find("c03") != std::string::npos) {
        return "-stc=c03";
    } else if (tempStringToDetermine.find("c98") != std::string::npos) {
        return "-stc=c98";
    } else if (tempStringToDetermine.find("c89") != std::string::npos) {
        return "-std=c89";
    } else {
        return "";
    }
    return "";
}

std::map<std::string, std::string> getEditorProgramPaths()
{
    //Executabe Name, Path
    std::map<std::string, std::string> returnMap;
    std::string pathString{static_cast<std::string>(getenv("PATH"))};
    std::vector<std::string> pathStringVector{parseToContainer<std::string, char, std::vector<std::string>>(pathString, PATH_DELIMITER)};
    if (pathStringVector.empty()) {
        return std::map<std::string, std::string>{};
    }
    SystemCommand systemCommand;
    std::vector<std::string> binaryNamesVector;
    for (auto &it : pathStringVector) {
        systemCommand.setCommand("ls " + tQuoted(it));
        binaryNamesVector = systemCommand.executeAndWaitForOutputAsVector();
        for (auto &binaryNamesIt : binaryNamesVector) {
            if (matchesKnownEditorBinaries(binaryNamesIt)) {
                returnMap.insert( std::make_pair(binaryNamesIt, (it + "/" + binaryNamesIt)) );
            }
        }
        binaryNamesVector.clear();
    }
    return returnMap;
}

bool matchesKnownEditorBinaries(const std::string &binaryNameToCheck) 
{
    size_t foundPosition{0};
    for (auto &it : KNOWN_EDITOR_BINARIES) {
        foundPosition = binaryNameToCheck.find(it);
        if ((foundPosition != std::string::npos) && (it.length() == binaryNameToCheck.length())) {
           return true;
        } else {
            std::string exeCheck = it + ".exe";
            size_t foundPosition{binaryNameToCheck.find(exeCheck)};
            if ((foundPosition != std::string::npos) && (exeCheck.length() == binaryNameToCheck.length())) {
                return true;
            }
        }
    }
    for (auto &it : extraEditors) {
        if ((foundPosition != std::string::npos) && (it.length() == binaryNameToCheck.length())) {
           return true;
        } else {
            std::string exeCheck = it + ".exe";
            size_t foundPosition = binaryNameToCheck.find(exeCheck);
            if ((foundPosition != std::string::npos) && (exeCheck.length() == binaryNameToCheck.length())) {
                return true;
            }
        }
    }
    return false;
}


std::vector<std::string> readConfigurationFile()
{
    std::vector<std::string> configurationFileOutput;
    if (!fileExists(DEFAULT_CONFIGURATION_FILE) && !fileExists(BACKUP_CONFIGURATION_FILE) && !fileExists(LAST_CHANCE_CONFIGURATION_FILE))  {
        return configurationFileOutput;
    }
    std::vector<std::string> buffer;
    std::ifstream readFromFile;
    readFromFile.open(DEFAULT_CONFIGURATION_FILE);
    if (readFromFile.is_open()) {
        std::string tempString{""};
        while (std::getline(readFromFile, tempString)) {
            buffer.emplace_back(tempString);
        }
        readFromFile.close();
    } else {
        if (fileExists(DEFAULT_CONFIGURATION_FILE)) {
            configurationFileOutput.emplace_back("WARNING: unable to open configuration file " + tQuoted(DEFAULT_CONFIGURATION_FILE) + ", but the file exists (maybe a permission problem?), trying backup file " + tQuoted(BACKUP_CONFIGURATION_FILE) + tEndl());
        }
        readFromFile.close();
        readFromFile.open(BACKUP_CONFIGURATION_FILE);
        if (readFromFile.is_open()) {
            std::string tempString{""};
            while (std::getline(readFromFile, tempString)) {
                buffer.emplace_back(tempString);
            }
            readFromFile.close();
        } else {
            if (fileExists(BACKUP_CONFIGURATION_FILE)) {
                configurationFileOutput.emplace_back("WARNING: unable to open configuration file " + tQuoted(BACKUP_CONFIGURATION_FILE) + ", but the file exists (maybe a permission problem?), trying second backup file " + tQuoted(LAST_CHANCE_CONFIGURATION_FILE) + tEndl());
            }
            readFromFile.close();
            readFromFile.open(LAST_CHANCE_CONFIGURATION_FILE);
            if (readFromFile.is_open()) {
                std::string tempString{""};
                while (std::getline(readFromFile, tempString)) {
                    buffer.emplace_back(tempString);
                }
                readFromFile.close();
            } else {
                if (fileExists(LAST_CHANCE_CONFIGURATION_FILE))   {
                    configurationFileOutput.emplace_back("WARNING: unable to open configuration file " + tQuoted(LAST_CHANCE_CONFIGURATION_FILE) + ", but the file exists (maybe a permission problem?), falling back on default editor names and custom libraries" + tEndl());
                }
                return configurationFileOutput;
            }
        }
    }
    for (std::vector<std::string>::const_iterator iter = buffer.begin(); iter != buffer.end(); iter++) {
        try {
            std::string copyString{*iter};
            std::transform(copyString.begin(), copyString.end(), copyString.begin(), ::tolower);
            //TODO: Replace with regex for searching
            size_t foundLibraryPosition{copyString.find(LIBRARY_IDENTIFIER)};
            size_t foundEditorPosition{copyString.find(EDITOR_IDENTIFIER)};
            if (copyString.length() != 0) {
                std::string otherCopy{copyString};
                int numberOfWhitespace{0};
                while (otherCopy.length() > 1 && isWhitespace(otherCopy[0])) {
                    stripFromString(otherCopy, ' ');
                    numberOfWhitespace++;
                }
                if (copyString.length() > numberOfWhitespace) {
                    if (copyString[numberOfWhitespace] == '#') {
                        continue;
                    }
                }
            } else {
                continue;
            }
            if (isWhitespace(copyString)) {
                continue;
            }

            long int currentLine{std::distance<std::vector<std::string>::const_iterator>(buffer.begin(), iter)+1};
            if (foundLibraryPosition != std::string::npos) {
                std::string targetLibrary{""};
                std::string headerFile{""};
                if (copyString.find(")") == std::string::npos) {
                    configurationFileOutput.emplace_back("WARNING: line " + toString(currentLine) + " of configuration file:");
                    configurationFileOutput.emplace_back("    No matching parenthesis found, ignoring option");
                    configurationFileOutput.emplace_back(*iter);
                    configurationFileOutput.emplace_back(whitespace(stripTrailingWhitespace(*iter).length()) + "^---expected here" + tEndl());
                    continue;
                } 
                if (copyString.find(",") == std::string::npos) { 
                    configurationFileOutput.emplace_back("WARNING: line " + toString(currentLine) + " of configuration file:");
                    configurationFileOutput.emplace_back("    No parameter separating comma found, ignoring option");
                    configurationFileOutput.emplace_back(*iter);
                    if (copyString.find(")") != std::string::npos) {
                        configurationFileOutput.emplace_back(whitespace(iter->find(")")) + "^---expected here" + tEndl());
                    } else {
                       configurationFileOutput.emplace_back(whitespace( (iter->find(".h") != std::string::npos) ? iter->find(".h") : iter->length())  + "^---expected here");
                    }
                    continue;
                } 
                headerFile = copyString.substr(LIBRARY_IDENTIFIER.length(), (copyString.find(",") - LIBRARY_IDENTIFIER.length()));
                if (headerFile.find(".h") == std::string::npos) {
                    configurationFileOutput.emplace_back("WARNING: line " + toString(currentLine) + " of configuration file:");
                    configurationFileOutput.emplace_back("    No .h extension found, but one was expected, ignoring option");
                    configurationFileOutput.emplace_back(*iter);
                    configurationFileOutput.emplace_back(whitespace(iter->find(",")) + "^---expected here" + tEndl());
                    continue;
                }
                copyString = iter->substr(copyString.find(",")+1);
                
                if (copyString.length() > 1) {
                    while ((copyString.length() > 1) && (isWhitespace(copyString[0]))) {
                        copyString = stripFromString(copyString, " ");
                    }
                }
                targetLibrary = copyString.substr(0, (copyString.find(")") != std::string::npos ? copyString.find(")") : 0));
                if (targetLibrary.length() == 0) {
                    configurationFileOutput.emplace_back("WARNING: line " + toString(currentLine) + " of configuration file:");
                    configurationFileOutput.emplace_back("    No library name specified after header file, ignoring option");
                    configurationFileOutput.emplace_back(*iter);
                    configurationFileOutput.emplace_back(whitespace(iter->find(")")) + "^---expected here" + tEndl());
                    continue;
                }
                libraryToHeaderMap.insert(std::make_pair(headerFile, targetLibrary));
            } else if (foundEditorPosition != std::string::npos) {
                if (copyString.find(")") == std::string::npos) {
                    configurationFileOutput.emplace_back("WARNING: line " + toString(currentLine) + " of configuration file:");
                    configurationFileOutput.emplace_back("    No matching parenthesis found, ignoring option");
                    configurationFileOutput.emplace_back(*iter);
                    configurationFileOutput.emplace_back(whitespace(stripTrailingWhitespace(*iter).length()) + "^---expected here" + tEndl());
                    continue;
                } 
                extraEditors.emplace_back(iter->substr(iter->find(EDITOR_IDENTIFIER)+EDITOR_IDENTIFIER.length(), iter->find(")") - iter->find(EDITOR_IDENTIFIER)+EDITOR_IDENTIFIER.length()));
            } else {
                configurationFileOutput.emplace_back("WARNING: line " + toString(currentLine) + " of configuration file:");
                configurationFileOutput.emplace_back("    expression is malformed/has invalid syntax, ignoring option");
                configurationFileOutput.emplace_back(*iter);
                configurationFileOutput.emplace_back(whitespace(stripTrailingWhitespace(*iter).length()) + "^---here" + tEndl());
            }
        } catch (std::exception &e) {
             configurationFileOutput.emplace_back("Standard exception caught in readConfigurationFile: " + toString(e.what()) + tEndl());
        }
    }
    return configurationFileOutput;
}

void interruptHandler(int signalNumber) 
{
    std::cout << std::endl << "Exiting " << PROGRAM_NAME << std::endl;
    exit (signalNumber);
}
