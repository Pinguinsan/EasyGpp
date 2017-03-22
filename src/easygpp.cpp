/***********************************************************************
*    easygpp.cpp:                                                      *
*    Interface program for quickly using gcc/g++                       *
*    Copyright (c) 2016 Tyler Lewis                                    *
************************************************************************
*    This is a source file for EasyGpp:                                *
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

#include <iostream>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <string>
#include <vector>
#include <sstream>
#include <memory>
#include <utility>
#include <iterator>
#include <future>
#include <chrono>
#include <cstring>
#include <list>
#include <map>
#include <set>

#include <unistd.h>
#include <signal.h>

#include <generalutilities.h>
#include <systemcommand.h>
#include <fileutilities.h>

#include "easygppstrings.h"
#include "configurationfilereader.h"

using namespace GeneralUtilities;
using namespace FileUtilities;
using namespace EasyGppStrings;

static const char *PROGRAM_NAME = "easyg++";
static const char *LONG_PROGRAM_NAME = "EasyGpp";
static const char *AUTHOR_NAME = "Tyler Lewis";
static const int SOFTWARE_MAJOR_VERSION = 0;
static const int SOFTWARE_MINOR_VERSION = 2;
static const int SOFTWARE_PATCH_VERSION = 0;

#if defined(__GNUC__)
    static const int GCC_MAJOR_VERSION = __GNUC__;
    static const int GCC_MINOR_VERSION = __GNUC_MINOR__;
    static const int GCC_PATCH_VERSION = __GNUC_PATCHLEVEL__;
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

static std::map<std::string, std::string> editorPrograms; 

void displayHelp();
void displayVersion();
void interruptHandler(int signalNumber);
void installSignalHandlers(void (*signalHandler)(int));

void displayConfigurationFilePaths();
bool isGeneralSwitch(const std::string &stringToCheck);
bool isLibrarySwitch(const std::string &stringToCheck);
bool isSourceCodeFile(const std::string &stringToCheck);

void doLibraryAdditions();

std::string determineOverrideStandard(const std::string &stringToDetermine);
std::map<std::string, std::string> getEditorProgramPaths();

bool matchesKnownEditorBinaries(const std::string &binaryNameToCheck);
void readConfigurationFile();
std::unique_ptr<ConfigurationFileReader> configurationFileReader;


static bool gccFlag{false};
static bool buildAndRun{false};
static bool verboseOutput{false};
static bool libraryOverride{false};
static bool editorProgramsRetrieved{false};
static std::string mTune{M_TUNE_GENERIC};
static std::string recordGCCSwitches{RECORD_GCC_SWITCHES};
static std::string sanitize{F_SANITIZE_UNDEFINED};
static std::string compilerType{static_cast<std::string>(GPP_COMPILER)};
static std::string gnuDebugSwitch{static_cast<std::string>(GDB_SWITCH)};
static std::string executableName{""};
static std::string staticSwitch{""};
static std::string staticLibGCCSwitch{""};
static std::vector<std::string> sourceCodeFiles;
static std::vector<std::string> generalSwitches;
static std::set<std::string> includePaths;
static std::set<std::string> libraryPaths;
static std::set<std::string> librarySwitches;
static std::string compilerStandard{DEFAULT_CPP_COMPILER_STANDARD};

int main(int argc, char *argv[])
{
    using namespace FileUtilities;
    installSignalHandlers(interruptHandler);
    
    std::cout << std::endl;
    for (int i = 0; i < argc; i++) { 
        if (isSwitch(argv[i], HELP_SWITCHES)) {
            displayHelp();
            return 0;
        } else if (isSwitch(argv[i], VERSION_SWITCHES)) {
            displayVersion();
            return 0;
        } else if (isSwitch(argv[i], CONFIGURATION_FILE_SWITCHES)) {
            displayConfigurationFilePaths();
            return 0;
        }
    }
    displayVersion();

    auto configFileTask = std::async(std::launch::async, readConfigurationFile);
    std::future<std::map<std::string, std::string>> editorProgramsTask = std::async(std::launch::async, getEditorProgramPaths);

    for (int i = 0; i < argc; i++) {
        if (isSwitch(argv[i], GCC_SWITCHES)) {
            gccFlag = true;
            compilerType = GCC_COMPILER;
            compilerStandard = DEFAULT_C_COMPILER_STANDARD;
        } else if (isSwitch(argv[i], NAME_SWITCHES)) {
            if (argv[i+1]) {
                executableName = static_cast<std::string>(argv[i+1]);
                i++;
            } else {
                std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but an output filename was not specified, skipping option" << std::endl;
                std::cout << "    Falling back on default executable name being .c/.cpp file name" << std::endl << std::endl;
            }
        } else if (isEqualsSwitch(argv[i], NAME_SWITCHES)) {
            std::string copyString{static_cast<std::string>(argv[i])};
            size_t foundPosition{copyString.find("=")};
            size_t foundEnd{copyString.substr(foundPosition).find(" ")};
            if (copyString.substr(foundPosition+1, (foundEnd - foundPosition)) == "") {
                std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but an output filename was not specified, skipping option" << std::endl;
            } else {
                executableName = stripAllFromString(copyString.substr(foundPosition+1, (foundEnd - foundPosition)), "\"");
            }   
        }  else if (isSwitch(argv[i], STANDARD_SWITCHES)) {
            if (argv[i+1]) {
                std::string tempCompilerStandard{determineOverrideStandard(static_cast<std::string>(argv[i+1]))};
                if (tempCompilerStandard == "") {
                    std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but standard " << tQuoted(argv[i+1]) << " is not a valid standard valid" << std::endl;
                    std::cout << "    Falling back on default compiler standard of " << tQuoted(DEFAULT_CPP_COMPILER_STANDARD) << std::endl << std::endl;
                } else {
                    compilerStandard = tempCompilerStandard;
                    i++;
                }
            } else {
                std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but a standard was not specified, skipping option" << std::endl;
                std::cout << "    Falling back on default compiler standard of " << tQuoted(DEFAULT_CPP_COMPILER_STANDARD) << std::endl << std::endl;
            }
        } else if (isEqualsSwitch(argv[i], STANDARD_SWITCHES)) {
            std::string copyString{static_cast<std::string>(argv[i])};
            size_t foundPosition{copyString.find("=")};
            size_t foundEnd{copyString.substr(foundPosition).find(" ")};
            if (copyString.substr(foundPosition+1, (foundEnd - foundPosition)) == "") {
                std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but a standard was not specified, skipping option" << std::endl;
            } else {
                std::string tempCompilerStandard{determineOverrideStandard(stripAllFromString(copyString.substr(foundPosition+1, (foundEnd - foundPosition)), "\""))};
                if (tempCompilerStandard == "") {
                    std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but standard " << tQuoted(stripAllFromString(copyString.substr(foundPosition+1, (foundEnd - foundPosition)), "\"")) << " is not a valid standard" << std::endl;
                    std::cout << "    Falling back on default compiler standard of " << tQuoted(DEFAULT_CPP_COMPILER_STANDARD) << std::endl << std::endl;
                } else {
                    compilerStandard = tempCompilerStandard;
                }
            }   
        } else if (isSwitch(argv[i], CLANG_SWITCHES)) {
            recordGCCSwitches = "";
            sanitize = "";
            mTune = "";
            compilerType = CLANG_COMPILER;
        } else if (isSwitch(argv[i], NO_DEBUG_SWITCHES)) {
            gnuDebugSwitch = "";
        } else if (isSwitch(argv[i], STATIC_SWITCHES)) {
            staticSwitch = " -static ";
            staticLibGCCSwitch = " -static-libgcc ";
        } else if (isSwitch(argv[i], VERBOSE_OUTPUT_SWITCHES)) {
            verboseOutput = true;
        } else if (isSwitch(argv[i], BUILD_AND_RUN_SWITCHES)) {
            buildAndRun = true;
        } else if (isSwitch(argv[i], LIBRARY_OVERRIDE_SWITCHES)) {
            libraryOverride = true;
        } else if (isSwitch(argv[i], NO_M_TUNE_SWITCHES)) {
            mTune = "";
        } else if (isSwitch(argv[i], NO_RECORD_GCC_SWITCHES_SWITCHES)) {
            recordGCCSwitches = "";
        } else if (isSwitch(argv[i], NO_F_SANITIZE_SWITCHES)) {
            sanitize = "";
        } else if (isSwitch(argv[i], INCLUDE_PATH_SWITCHES)) {
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
                std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but no directory was specified, skipping option" << std::endl;
                std::cout << "    Skipping include path option" << std::endl << std::endl;
            } 
        } else if (isEqualsSwitch(argv[i], INCLUDE_PATH_SWITCHES)) {
            std::string copyString{static_cast<std::string>(argv[i])};
            size_t foundPosition{copyString.find("=")};
            size_t foundEnd{copyString.substr(foundPosition).find(" ")};
            if (copyString.substr(foundPosition+1, (foundEnd - foundPosition)) == "") {
                std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but no directory was specified, skipping option" << std::endl;
            } else {
                std::string tempSwitchDir{stripAllFromString(copyString.substr(foundPosition+1, (foundEnd - foundPosition)), "\"")};
                if (!directoryExists(tempSwitchDir)) {
                    std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but include path " << tQuoted(tempSwitchDir) << " is not a valid directory" << std::endl;
                    std::cout << "    Skipping include path option" << std::endl << std::endl;
                } else {
                    includePaths.emplace(tempSwitchDir);
                }
            }   
        } else if (isSwitch(argv[i], LIBRARY_PATH_SWITCHES)) {
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
                std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but no directory was specified, skipping option" << std::endl;
                std::cout << "    Skipping library path option" << std::endl << std::endl;
            } 
        } else if (isEqualsSwitch(argv[i], LIBRARY_PATH_SWITCHES)) {
            std::string copyString{static_cast<std::string>(argv[i])};
            size_t foundPosition{copyString.find("=")};
            size_t foundEnd{copyString.substr(foundPosition).find(" ")};
            if (copyString.substr(foundPosition+1, (foundEnd - foundPosition)) == "") {
                std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but no directory was specified, skipping option" << std::endl;
            } else {
                std::string tempSwitchDir{stripAllFromString(copyString.substr(foundPosition+1, (foundEnd - foundPosition)), "\"")};
                if (!directoryExists(tempSwitchDir)) {
                    std::cout << "WARNING: Switch " << tQuoted(argv[i]) << " accepted, but library path " << tQuoted(tempSwitchDir) << " is not a valid directory" << std::endl;
                    std::cout << "    Skipping include path option" << std::endl << std::endl;
                } else {
                    libraryPaths.emplace(tempSwitchDir);
                }
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
        SystemCommand systemCommand{compilerType 
                                    + WARNING_LEVEL 
                                    + mTune
                                    + sanitize
                                    + recordGCCSwitches
                                    + gnuDebugSwitch 
                                    + staticSwitch 
                                    + staticLibGCCSwitch};
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
        if (configFileTask.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
            configFileTask.wait();
        }
        if (!libraryOverride) {
            doLibraryAdditions();
        }
        for (auto &it : librarySwitches) {
            systemCommand += (" " + it);
        }
        for (auto &it : configurationFileReader->output()) {
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
                    std::cout << tWhitespace(outputText.length()) << tQuoted(it) << std::endl;
                }
            }
            std::cout << std::endl << "compiled successfully to make executable file \"" << executableName;
            if (executableName.find(".exe") != std::string::npos) {
                std::cout << "\"" << std::endl;
            } else {
                #if defined(__CYGWIN__)
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
                std::cout << tQuoted(userReplyString) << " wasn't one of the selections, enter a number between (inclusive) 1 and " << exitOption << ", or press CTRL+C to quit" << std::endl << std::endl;
                continue;
            }
            if ((userReply <= (i+1)) && (userReply > 0)) {
                userReplied = true;
            } else {
                std::cout << tQuoted(userReplyString) << " wasn't one of the selections, enter a number between (inclusive) 1 and " << exitOption << ", or press CTRL+C to quit" << std::endl << std::endl;
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
            std::cout << "No known editor programs were found, exiting " << PROGRAM_NAME << std::endl;
            return 2;
        }
        std::cout << std::endl << "Which editor should be used?" << std::endl;
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
                std::cout << tQuoted(userReplyString) << " wasn't one of the selections, enter a number between (inclusive) 1 and " << exitOption << ", or press CTRL+C to quit" << std::endl << std::endl;
                continue;
            }
            if ((userReply <= (i + 1)) && (userReply > 0)) {
                userReplied = true;
            } else {
                std::cout << tQuoted(userReplyString) << " wasn't one of the above number selections, enter a number between 1 and " << exitOption << ", or press CTRL+C to quit" << std::endl << std::endl;
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
    std::cout << "    -s, --s, -standard, --standard: Override the default of -std=c++14/-std=c11" << std::endl;
    std::cout << "        Note: must include a standard following this (eg " << tQuoted("-std=c++11") << ")" << std::endl;
    std::cout << "    -st, --st, -static, --static: Link the program statically against the included libraries" << std::endl;
    std::cout << "    -h, --h, -help, --help: Display this help text" << std::endl;
    std::cout << "    -v, --v, -version, --version: Display the version" << std::endl;
    std::cout << "    -nd, --nd, -nodebug, --nodebug: Compile the program without debug information for gdb" << std::endl;
    std::cout << "    -r, --r, -run, --run: Compile the program and run after successfully compiling" << std::endl;
    std::cout << "    -lo, --lo, -loverride, --loverride: Override the default behavior to automatically add libraries, as specified by the configuration file " << std::endl;
    std::cout << "    -i, --i, -include, --include: Add an additional include path" << std::endl;
    std::cout << "    -l, --l, -libdir, --libdir: Add an additional library path" << std::endl;
    std::cout << "    -m, --m, -nomtune, --nomtune: Do not inclue -mtune=generic switch" << std::endl;
    std::cout << "    -nr, --nr, -norecord, --norecord: Do not include -frecord-gcc-switches switch" << std::endl;
    std::cout << "    -f, --f, -nofsanitize, --nofsanitize: Do not include -fsanitize=undefined switch" << std::endl;
    std::cout << "    -p, --p, -config-file, --config-file: List the configuration file paths" << std::endl;
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
    #if defined(__CYGWIN__)
        std::cout << "(.exe)\"" << std::endl;
    #else
        std::cout << "\"" << std::endl;
    #endif
    std::cout << std::endl;
}

void displayVersion() 
{
    std::cout << PROGRAM_NAME << ", v" << SOFTWARE_MAJOR_VERSION << "." << SOFTWARE_MINOR_VERSION << "." << SOFTWARE_PATCH_VERSION << std::endl;
    std::cout << "Written by " << AUTHOR_NAME << ", " << __DATE__ << std::endl;
    std::cout << "Built with g++ v" << GCC_MAJOR_VERSION << "." << GCC_MINOR_VERSION << "." << GCC_PATCH_VERSION << ", " << __DATE__ << std::endl << std::endl;
}

void displayConfigurationFilePaths()
{
    using namespace FileUtilities;
    using namespace GeneralUtilities;
    std::vector<std::pair<std::string, std::string>> configurationFiles{std::make_pair("Default: ", DEFAULT_CONFIGURATION_FILE),
                                                                        std::make_pair("Backup: ", BACKUP_CONFIGURATION_FILE),
                                                                        std::make_pair("Backup 2: ", LAST_CHANCE_CONFIGURATION_FILE)};
    long unsigned int maximumLength{0};
    for (auto &it : configurationFiles) {
        if ((it.first.length() + it.second.length()) > maximumLength) {
            maximumLength = it.first.length() + it.second.length();
        }
    }
    for (auto &it : configurationFiles) {
        std::cout << it.first << it.second;
        long unsigned int tempLength{it.first.length() + it.second.length()};
        std::cout << tWhitespace(maximumLength - tempLength) << "    ";
        if (fileExists(it.second)) {
            std::cout << "<---Existing File";
        } else {
            std::cout << "<---File Does Not Exist";
        } 
        std::cout << std::endl;
    }
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
    std::string tempStringToCheck{stringToCheck};
    std::transform(tempStringToCheck.begin(), tempStringToCheck.end(), tempStringToCheck.begin(), ::tolower);
    return (tempStringToCheck.find(".c") != std::string::npos);
}

std::string determineOverrideStandard(const std::string &stringToDetermine) 
{
    std::string tempStringToDetermine{stringToDetermine};
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
    std::vector<std::string> pathStringVector{parseToContainer<std::vector<std::string>>(pathString.begin(), pathString.end(), PATH_DELIMITER)};
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
    for (auto &iter : KNOWN_EDITOR_BINARIES) {
        std::string it{static_cast<std::string>(iter)};
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
    for (auto &it : configurationFileReader->extraEditors()) {
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

void doLibraryAdditions()
{
    for (auto &it : sourceCodeFiles) {
        std::ifstream readFromFile;
        readFromFile.open(it);
        if (readFromFile.is_open()) {
            std::string rawString{""};
            while (std::getline(readFromFile, rawString)) {
                for (auto &mapIt : configurationFileReader->libraryToHeaderMap()) {
                    if (rawString.find(mapIt.first) != std::string::npos) {
                        if ((mapIt.second.find("-l") != std::string::npos) || (mapIt.second[0] == '-')) {
                            auto result = librarySwitches.emplace(mapIt.second);
                            if (result.second) {
                                if (verboseOutput) {
                                    std::cout << "NOTE: library " << tQuoted(mapIt.second) << " was associated with header file " << tQuoted(mapIt.first) << " from configuration file, so the library has been added to the command line arguments (this behavior can be disabled with the " << tQuoted("--l") << " switch)" << std::endl << std::endl;
                                }
                            }
                        } else {
                            auto result = librarySwitches.emplace("-l" + mapIt.second);
                            if (result.second) {
                                if (verboseOutput) {
                                    std::cout << "NOTE: library " << tQuoted(mapIt.second) << " was associated with header file " << tQuoted(mapIt.first) << " from configuration file, so the library has been added to the command line arguments (this behavior can be disabled with the " << tQuoted("--l") << " switch)" << std::endl << std::endl;
                                }
                            }
                        }
                    }
                }
                #ifdef __linux__
                    for (auto &it : PTHREAD_IDENTIFIERS) {
                        if (rawString.find(it) != std::string::npos) {
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

void readConfigurationFile()
{
    configurationFileReader = std::unique_ptr<ConfigurationFileReader>(new ConfigurationFileReader{});
}

void interruptHandler(int signalNumber) 
{
    std::cout << std::endl << "Caught signal " << signalNumber << " (" << std::strerror(signalNumber) << "), exiting " << PROGRAM_NAME << std::endl;
    exit (signalNumber);
}

void installSignalHandlers(void (*signalHandler)(int))
{
    static struct sigaction signalInterruptHandler;
    signalInterruptHandler.sa_handler = signalHandler;
    sigemptyset(&signalInterruptHandler.sa_mask);
    signalInterruptHandler.sa_flags = 0;
    sigaction(SIGHUP, &signalInterruptHandler, NULL);
    sigaction(SIGINT, &signalInterruptHandler, NULL);
    sigaction(SIGQUIT, &signalInterruptHandler, NULL);
    sigaction(SIGILL, &signalInterruptHandler, NULL);
    sigaction(SIGABRT, &signalInterruptHandler, NULL);
    sigaction(SIGFPE, &signalInterruptHandler, NULL);
    sigaction(SIGKILL, &signalInterruptHandler, NULL);
    sigaction(SIGSEGV, &signalInterruptHandler, NULL);
    sigaction(SIGPIPE, &signalInterruptHandler, NULL);
    sigaction(SIGALRM, &signalInterruptHandler, NULL);
    sigaction(SIGTERM, &signalInterruptHandler, NULL);
    sigaction(SIGUSR1, &signalInterruptHandler, NULL);
    sigaction(SIGUSR2, &signalInterruptHandler, NULL);
    sigaction(SIGCHLD, &signalInterruptHandler, NULL);
    sigaction(SIGCONT, &signalInterruptHandler, NULL);
    sigaction(SIGSTOP, &signalInterruptHandler, NULL);
    sigaction(SIGTSTP, &signalInterruptHandler, NULL);
    sigaction(SIGTTIN, &signalInterruptHandler, NULL);
    sigaction(SIGTTOU, &signalInterruptHandler, NULL);
}      