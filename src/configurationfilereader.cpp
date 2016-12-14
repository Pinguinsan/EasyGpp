/***********************************************************************
*    configurationfilereader.cpp:                                      *
*    A class for reading the configuration file for EasyGpp            *
*    Copyright (c) 2016 Tyler Lewis                                    *
************************************************************************
*    This is a source file for EasyGpp:                                *
*    https://github.com/Pinguinsan/EasyGpp                             *
*    The source code is released under the GNU LGPL                    *
*    This file holds the implementation of a ConfigurationFileReader   *
*    class. This class is used to configure EasyGpp, with things like  *
*    auto populating included libraries (according to headers used),   *
*    as well as specifying extra editors to choose from if the target  *
*    program does not compile correctly, and must be edited            *
*                                                                      *
*    You should have received a copy of the GNU Lesser General         *
*    Public license along with libraryprojects                         *
*    If not, see <http://www.gnu.org/licenses/>                        *
***********************************************************************/

#include "configurationfilereader.h"

ConfigurationFileReader::ConfigurationFileReader() :
    m_extraEditors{std::set<std::string>{}},
    m_libraryToHeaderMap{std::map<std::string, std::string>{}},
    m_output{std::vector<std::string>{}}
{
    using namespace FileUtilities;
    using namespace GeneralUtilities;
    using namespace EasyGppStrings;
    if (!fileExists(DEFAULT_CONFIGURATION_FILE) &&
        !fileExists(BACKUP_CONFIGURATION_FILE) &&
        !fileExists(LAST_CHANCE_CONFIGURATION_FILE))  {
        this->m_output.push_back(static_cast<std::string>(NO_CONFIGURATION_FILE_WARNING_STRING));
        this->m_output.push_back(static_cast<std::string>(DEFAULT_CONFIGURATION_FILE_BASE) + tQuoted(DEFAULT_CONFIGURATION_FILE));
        this->m_output.push_back(static_cast<std::string>(BACKUP_CONFIGURATION_FILE_BASE) + tQuoted(BACKUP_CONFIGURATION_FILE));
        this->m_output.push_back(static_cast<std::string>(LAST_CHANCE_CONFIGURATION_FILE_BASE) + tQuoted(LAST_CHANCE_CONFIGURATION_FILE));
        return;
    }
    std::vector<std::string> buffer;
    std::ifstream readFromFile;
    const std::vector<std::string> configurationFiles{DEFAULT_CONFIGURATION_FILE,
                                                      BACKUP_CONFIGURATION_FILE,
                                                      LAST_CHANCE_CONFIGURATION_FILE};
    for (auto it = configurationFiles.begin(); it != configurationFiles.end(); it++) {
        readFromFile.open(*it);
        if (readFromFile.is_open()) {
            std::cout << USING_CONFIGURATION_FILE_STRING << tQuoted(*it) << std::endl;
            std::string tempString{""};
            while (std::getline(readFromFile, tempString)) {
                buffer.emplace_back(tempString);
            }
            readFromFile.close();
            break;
        } else {
            if (*it == static_cast<std::string>(LAST_CHANCE_CONFIGURATION_FILE)) {
                if (fileExists(*it)) {
                    this->m_output.push_back(static_cast<std::string>(UNABLE_TO_OPEN_CONFIGURATION_FILE_STRING_BASE)
                                             + tQuoted(*it)
                                             + static_cast<std::string>(UNABLE_TO_OPEN_CONFIGURATION_FILE_STRING_TAIL)
                                             + static_cast<std::string>(FALL_BACK_ON_DEFAULTS_STRING));
                }
                readFromFile.close();
                return;
            } else {
                if (fileExists(*it)) {
                    this->m_output.push_back(static_cast<std::string>(UNABLE_TO_OPEN_CONFIGURATION_FILE_STRING_BASE)
                                             + tQuoted(*it)
                                             + static_cast<std::string>(UNABLE_TO_OPEN_CONFIGURATION_FILE_STRING_TAIL)
                                             + static_cast<std::string>(TRYING_BACKUP_FILE_STRING)
                                             + tQuoted(*(it+1)));
                }
                readFromFile.close();
            }
        }
    }
    for (std::vector<std::string>::const_iterator iter = buffer.begin(); iter != buffer.end(); iter++) {
        try {
            std::string copyString{*iter};
            std::transform(copyString.begin(), copyString.end(), copyString.begin(), ::tolower);
            //TODO: Replace with regex for searching
            size_t foundLibraryPosition{copyString.find(static_cast<std::string>(LIBRARY_IDENTIFIER))};
            size_t foundEditorPosition{copyString.find(static_cast<std::string>(EDITOR_IDENTIFIER))};
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
                    this->m_output.emplace_back(static_cast<std::string>(GENERIC_CONFIG_WARNING_BASE_STRING) 
                                                + toString(currentLine) 
                                                + static_cast<std::string>(GENERIC_CONFIG_WARNING_TAIL_STRING));
                    this->m_output.emplace_back(static_cast<std::string>(NO_CLOSING_PARENTHESIS_FOUND_STRING));
                    this->m_output.emplace_back(*iter);
                    this->m_output.emplace_back(tWhitespace(stripTrailingWhitespace(*iter).length()) + static_cast<std::string>(EXPECTED_HERE_STRING) + tEndl());
                    continue;
                } 
                if (copyString.find(",") == std::string::npos) { 
                    this->m_output.emplace_back(static_cast<std::string>(GENERIC_CONFIG_WARNING_BASE_STRING) 
                                                + toString(currentLine) 
                                                + static_cast<std::string>(GENERIC_CONFIG_WARNING_TAIL_STRING));                    
                    this->m_output.emplace_back(static_cast<std::string>(NO_PARAMETER_SEPARATING_COMMA_STRING));
                    this->m_output.emplace_back(*iter);
                    if (copyString.find(")") != std::string::npos) {
                        this->m_output.emplace_back(tWhitespace(iter->find(")")) + static_cast<std::string>(EXPECTED_HERE_STRING) + tEndl());
                    } else {
                       this->m_output.emplace_back(tWhitespace( (iter->find(".h") != std::string::npos) ? iter->find(".h") : iter->length())  + static_cast<std::string>(EXPECTED_HERE_STRING));
                    }
                    continue;
                }
                std::string headerAndLibrary{trimWhitespace(getBetween("(", ")", copyString))}; 
                headerFile = headerAndLibrary.substr(0, headerAndLibrary.find(","));
                targetLibrary = headerAndLibrary.substr(headerAndLibrary.find(",")+1);
                if (headerFile.find(".h") == std::string::npos) {
                    this->m_output.emplace_back(static_cast<std::string>(GENERIC_CONFIG_WARNING_BASE_STRING) 
                                                + toString(currentLine) 
                                                + static_cast<std::string>(GENERIC_CONFIG_WARNING_TAIL_STRING));
                    this->m_output.emplace_back(static_cast<std::string>(NO_H_EXTENSION_FOUND_STRING));
                    this->m_output.emplace_back(*iter);
                    this->m_output.emplace_back(tWhitespace(iter->find(",")) + static_cast<std::string>(EXPECTED_HERE_STRING) + tEndl());
                    continue;
                }
                if (targetLibrary.length() == 0) {
                    this->m_output.emplace_back(static_cast<std::string>(GENERIC_CONFIG_WARNING_BASE_STRING) 
                                                + toString(currentLine) 
                                                + static_cast<std::string>(GENERIC_CONFIG_WARNING_TAIL_STRING));                    
                this->m_output.emplace_back(static_cast<std::string>(NO_LIBRARY_NAME_SPECIFIED_STRING));
                    this->m_output.emplace_back(*iter);
                    this->m_output.emplace_back(tWhitespace(iter->find(")")) + static_cast<std::string>(EXPECTED_HERE_STRING) + tEndl());
                    continue;
                }
                this->m_libraryToHeaderMap.emplace(headerFile, targetLibrary);
            } else if (foundEditorPosition != std::string::npos) {
                if (copyString.find(")") == std::string::npos) {
                    this->m_output.emplace_back(static_cast<std::string>(GENERIC_CONFIG_WARNING_BASE_STRING) 
                                                + toString(currentLine) 
                                                + static_cast<std::string>(GENERIC_CONFIG_WARNING_TAIL_STRING));
                    this->m_output.emplace_back(static_cast<std::string>(NO_CLOSING_PARENTHESIS_FOUND_STRING));
                    this->m_output.emplace_back(*iter);
                    this->m_output.emplace_back(tWhitespace(stripTrailingWhitespace(*iter).length()) + static_cast<std::string>(EXPECTED_HERE_STRING) + tEndl());
                    continue;
                } 
                this->m_extraEditors.emplace(getBetween("(", ")", *iter));
            } else {
                    this->m_output.emplace_back(static_cast<std::string>(GENERIC_CONFIG_WARNING_BASE_STRING) 
                                                + toString(currentLine) 
                                                + static_cast<std::string>(GENERIC_CONFIG_WARNING_TAIL_STRING));                
                this->m_output.emplace_back(static_cast<std::string>(CONFIG_EXPRESSION_MALFORMED_STRING));
                this->m_output.emplace_back(*iter);
                this->m_output.emplace_back(tWhitespace(stripTrailingWhitespace(*iter).length()) + static_cast<std::string>(HERE_STRING)  + tEndl());
            }
        } catch (std::exception &e) {
             this->m_output.emplace_back(static_cast<std::string>(STANDARD_EXCEPTION_CAUGHT_IN_CONSTRUCTOR_STRING) + toString(e.what()) + tEndl());
        }
    }
}

std::set<std::string> ConfigurationFileReader::extraEditors() const
{
    return this->m_extraEditors;
}

std::map<std::string, std::string> ConfigurationFileReader::libraryToHeaderMap() const
{
    return this->m_libraryToHeaderMap;
}

std::vector<std::string> ConfigurationFileReader::output() const
{
    return this->m_output;
}