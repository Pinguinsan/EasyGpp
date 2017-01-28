/***********************************************************************
*    easygppstrings.cpp:                                               *
*    Main strings used by EasyGpp and ConfigurationFileReader          *
*    Copyright (c) 2016 Tyler Lewis                                    *
************************************************************************
*    This is a source file for EasyGpp:                                *
*    https://github.com/Pinguinsan/EasyGpp                             *
*    The source code is released under the GNU LGPL                    *
*    This file holds most of the strings used in EasyGpp, as well as   *
*    the ConfigurationFileReader class, in the EasyGppStrings namespace*
*                                                                      *
*    You should have received a copy of the GNU Lesser General         *
*    Public license along with libraryprojects                         *
*    If not, see <http://www.gnu.org/licenses/>                        *
***********************************************************************/

#include "easygppstrings.h"

namespace EasyGppStrings
{
	const char PATH_DELIMITER = ':';

	const std::list<const char *> KNOWN_EDITOR_BINARIES{"notepad", "vim", "nano", "emacs", "mousepad", "leafpad", "code", "sublime_text", "vscode"};
	const std::list<const char *> STATIC_SWITCHES{"-t", "--t", "--static", "-static"};
	const std::list<const char *> STANDARD_SWITCHES{"-s", "--s", "-standard", "--standard"};    
	const std::list<const char *> HELP_SWITCHES{"-h", "--h", "-help", "--help"};
	const std::list<const char *> VERSION_SWITCHES{"v", "-v", "--v", "-version", "--version"};
	const std::list<const char *> GCC_SWITCHES{"-c", "--c", "-cc", "--cc", "-gcc", "--gcc"};
	const std::list<const char *> NAME_SWITCHES{"-n", "--n", "-name", "--name"};
	const std::list<const char *> NO_DEBUG_SWITCHES{"-d", "--d", "-no-debug", "--no-debug"};    
	const std::list<const char *> BUILD_AND_RUN_SWITCHES{"-r", "--r", "-run", "--run", "-buildandrun", "--buildandrun"};
	const std::list<const char *> LIBRARY_OVERRIDE_SWITCHES{"-o", "--o", "-loverride", "--loverride"};
	const std::list<const char *> VERBOSE_OUTPUT_SWITCHES{"-e", "--e", "-verbose", "--verbose"};
	const std::list<const char *> INCLUDE_PATH_SWITCHES{"-i", "--i", "-include", "--include", "-include-dir", "--include-dir", "-include-path", "--include-path"};
	const std::list<const char *> LIBRARY_PATH_SWITCHES{"-l", "--l", "-lib-dir", "--lib-dir", "-lib-path", "--lib-path", "-library", "--library"};
	const std::list<const char *> NO_M_TUNE_SWITCHES{"-m", "--m", "-no-mtune", "--no-mtune"};
	const std::list<const char *> NO_RECORD_GCC_SWITCHES_SWITCHES{"-h", "--h", "-no-record", "--no-record"};
	const std::list<const char *> NO_F_SANITIZE_SWITCHES{"-f", "--f", "-no-fsanitize", "--no-fsanitize"};
	const std::list<const char *> CONFIGURATION_FILE_SWITCHES{"-p", "--p", "-config-file", "--config-file"};
	const char *WARNING_LEVEL{" -Wall -Wextra -Wpedantic"};
	const char *STANDARD_PROMPT_STRING{"enter a selection: "};
	const char *DEFAULT_CPP_COMPILER_STANDARD{"-std=c++14"};
	const char *DEFAULT_C_COMPILER_STANDARD{"-std=c11"};
	const char *GDB_SWITCH{" -ggdb"};
	const char *GCC_COMPILER{"gcc"};
	const char *GPP_COMPILER{"g++"};
	#if defined(_WIN32) || defined(__CYGWIN__) || defined(__arm__)
	    const char *RECORD_GCC_SWITCHES{""};
	    const char *F_SANITIZE_UNDEFINED{""};
	    const char *M_TUNE_GENERIC{""};
	#else
	    const char *RECORD_GCC_SWITCHES{" -frecord-gcc-switches"};
	    const char *F_SANITIZE_UNDEFINED{" -fsanitize=undefined"};
	    const char *M_TUNE_GENERIC{" -mtune=generic"};
	#endif

	const char *EDITOR_IDENTIFIER{"addeditor("};
	const char *LIBRARY_IDENTIFIER{"addlibrary("};
	const char *CONFIGURATION_FILE_NAME{"easygpp.config"};
	const std::string DEFAULT_CONFIGURATION_FILE{static_cast<std::string>(getenv("HOME"))
		                                            + "/.easygpp/" 
		                                            + static_cast<std::string>(CONFIGURATION_FILE_NAME)};
	const std::string BACKUP_CONFIGURATION_FILE{"/usr/share/easygpp/" 
		                                           + static_cast<std::string>(CONFIGURATION_FILE_NAME)};
	const std::string LAST_CHANCE_CONFIGURATION_FILE{"/opt/GitHub/EasyGpp/config/"
		                                                + static_cast<std::string>(CONFIGURATION_FILE_NAME)};

	const std::vector<const char *> PTHREAD_IDENTIFIERS{"<thread>", "<future>"};

	const char *DEFAULT_CONFIGURATION_FILE_BASE{"Default configuration file path: "};
    const char *BACKUP_CONFIGURATION_FILE_BASE{"Backup configuration file path: "};
    const char *LAST_CHANCE_CONFIGURATION_FILE_BASE{"Last change configuration file path: "};
    const char *USING_CONFIGURATION_FILE_STRING{"Using configuration file "};

    const char *NO_CONFIGURATION_FILE_WARNING_STRING{"WARNING: No configuration file found"};
    const char *UNABLE_TO_OPEN_CONFIGURATION_FILE_STRING_BASE{"WARNING: unable to open configuration file "};
    const char *UNABLE_TO_OPEN_CONFIGURATION_FILE_STRING_TAIL{", but the file exists (maybe a permission problem?), "};
    const char *FALL_BACK_ON_DEFAULTS_STRING{"falling back on program defaults"};
    const char *TRYING_BACKUP_FILE_STRING{"trying backup file "};
    const char *NO_CLOSING_PARENTHESIS_FOUND_STRING{"    No matching parenthesis found, ignoring option"};
    const char *NO_CLOSING_QUOTATION_MARKS_FOUND_STRING{"    No matching quotation marks found, ingoring option"};
    const char *NO_PARAMETER_SEPARATING_COMMA_STRING{"    No parameter separating comma found, ignoring option"};
    const char *EXPECTED_HERE_STRING{"^---expected here"};
    const char *HERE_STRING{"^---here"};
    const char *GENERIC_CONFIG_WARNING_BASE_STRING{"WARNING: line "};
    const char *GENERIC_CONFIG_WARNING_TAIL_STRING{" of configuration file:"};
    const char *CONFIG_EXPRESSION_MALFORMED_STRING{"    expression is malformed/has invalid syntax, ignoring option"};
    const char *NO_H_EXTENSION_FOUND_STRING{"    No .h extension found, but one was expected, ignoring option"};
    const char *NO_LIBRARY_NAME_SPECIFIED_STRING{"    No library name specified after header file, ignoring option"};
	const char *STANDARD_EXCEPTION_CAUGHT_IN_CONSTRUCTOR_STRING{"Standard exception caught in ReadConfigurationFile() constructor: "};
}
