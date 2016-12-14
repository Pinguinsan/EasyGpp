/***********************************************************************
*    easygppstrings.h:                                                 *
*    Main strings used by EasyGpp and ConfigurationFileReader          *
*    Copyright (c) 2016 Tyler Lewis                                    *
************************************************************************
*    This is a header file for EasyGpp:                                *
*    https://github.com/Pinguinsan/EasyGpp                             *
*    The source code is released under the GNU LGPL                    *
*    This file holds most of the strings used in EasyGpp, as well as   *
*    the ConfigurationFileReader class, in the EasyGppStrings namespace*
*                                                                      *
*    You should have received a copy of the GNU Lesser General         *
*    Public license along with libraryprojects                         *
*    If not, see <http://www.gnu.org/licenses/>                        *
***********************************************************************/

#ifndef EASYGPP_EASYGPPSTRINGS_H
#define EASYGPP_EASYGPPSTRINGS_H

#include <list>
#include <string>
#include <vector>

namespace EasyGppStrings
{
	extern const char PATH_DELIMITER;
	extern const std::list<const char *> KNOWN_EDITOR_BINARIES;
	extern const std::list<const char *> STATIC_SWITCHES;
	extern const std::list<const char *> STANDARD_SWITCHES;    
	extern const std::list<const char *> HELP_SWITCHES;
	extern const std::list<const char *> VERSION_SWITCHES;
	extern const std::list<const char *> GCC_SWITCHES;
	extern const std::list<const char *> NAME_SWITCHES;
	extern const std::list<const char *> NO_DEBUG_SWITCHES;    
	extern const std::list<const char *> BUILD_AND_RUN_SWITCHES;
	extern const std::list<const char *> LIBRARY_OVERRIDE_SWITCHES;
	extern const std::list<const char *> VERBOSE_OUTPUT_SWITCHES;
	extern const std::list<const char *> INCLUDE_PATH_SWITCHES;
	extern const std::list<const char *> LIBRARY_PATH_SWITCHES;
	extern const std::list<const char *> NO_M_TUNE_SWITCHES;
	extern const std::list<const char *> NO_RECORD_GCC_SWITCHES_SWITCHES;
	extern const std::list<const char *> NO_F_SANITIZE_SWITCHES;
	extern const std::list<const char *> CONFIGURATION_FILE_SWITCHES;
	extern const char *GDB_SWITCH;
	extern const char *GCC_COMPILER;
	extern const char *GPP_COMPILER;
	extern const char *WARNING_LEVEL;
	extern const char *STANDARD_PROMPT_STRING;
	extern const char *DEFAULT_CPP_COMPILER_STANDARD;
	extern const char *DEFAULT_C_COMPILER_STANDARD;
	extern const char *M_TUNE_GENERIC;
	#if defined(_WIN32) || defined(__CYGWIN__)
	    extern const char *RECORD_GCC_SWITCHES;
	    extern const char *F_SANITIZE_UNDEFINED;
	#else
	    extern const char *RECORD_GCC_SWITCHES;
	    extern const char *F_SANITIZE_UNDEFINED;
	#endif

	extern const char *EDITOR_IDENTIFIER;
	extern const char *LIBRARY_IDENTIFIER;
	extern const char *CONFIGURATION_FILE_NAME;
	extern const std::string DEFAULT_CONFIGURATION_FILE;
	extern const std::string BACKUP_CONFIGURATION_FILE;
	extern const std::string LAST_CHANCE_CONFIGURATION_FILE;
	extern const std::vector<const char *> PTHREAD_IDENTIFIERS;

	extern const char *DEFAULT_CONFIGURATION_FILE_BASE;
    extern const char *BACKUP_CONFIGURATION_FILE_BASE;
    extern const char *LAST_CHANCE_CONFIGURATION_FILE_BASE;
    extern const char *USING_CONFIGURATION_FILE_STRING;

    extern const char *NO_CONFIGURATION_FILE_WARNING_STRING;
    extern const char *UNABLE_TO_OPEN_CONFIGURATION_FILE_STRING_BASE;
    extern const char *UNABLE_TO_OPEN_CONFIGURATION_FILE_STRING_TAIL;
    extern const char *FALL_BACK_ON_DEFAULTS_STRING;
    extern const char *TRYING_BACKUP_FILE_STRING;
    extern const char *NO_CLOSING_PARENTHESIS_FOUND_STRING;
    extern const char *NO_CLOSING_QUOTATION_MARKS_FOUND_STRING;
    extern const char *NO_PARAMETER_SEPARATING_COMMA_STRING;
    extern const char *EXPECTED_HERE_STRING;
    extern const char *HERE_STRING;
    extern const char *GENERIC_CONFIG_WARNING_BASE_STRING;
    extern const char *GENERIC_CONFIG_WARNING_TAIL_STRING;
    extern const char *CONFIG_EXPRESSION_MALFORMED_STRING;
    extern const char *NO_H_EXTENSION_FOUND_STRING;
    extern const char *NO_LIBRARY_NAME_SPECIFIED_STRING;
	extern const char *STANDARD_EXCEPTION_CAUGHT_IN_CONSTRUCTOR_STRING;

}

#endif //EASYGPP_EASYGPPSTRINGS_H