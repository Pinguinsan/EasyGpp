/***********************************************************************
*    configurationfilereader.h:                                        *
*    A class for reading the configuration file for EasyGpp            *
*    Copyright (c) 2016 Tyler Lewis                                    *
************************************************************************
*    This is a header file for EasyGpp:                                *
*    https://github.com/Pinguinsan/EasyGpp                             *
*    The source code is released under the GNU LGPL                    *
*    This file holds the declarations of a ConfigurationFileReader     *
*    class. This class is used to configure EasyGpp, with things like  *
*    auto populating included libraries (according to headers used),   *
*    as well as specifying extra editors to choose from if the target  *
*    program does not compile correctly, and must be edited            *
*                                                                      *
*    You should have received a copy of the GNU Lesser General         *
*    Public license along with libraryprojects                         *
*    If not, see <http://www.gnu.org/licenses/>                        *
***********************************************************************/

#ifndef EASYGPP_CONFIGURATIONFILEREADER_H
#define EASYGPP_CONFIGURATIONFILEREADER_H

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <list>
#include <set>
#include <map>

#include <fileutilities.h>
#include <generalutilities.h>

#include "easygppstrings.h"


class ConfigurationFileReader
{
public:
    ConfigurationFileReader();
    std::set<std::string> extraEditors() const;
    std::map<std::string, std::string> libraryToHeaderMap() const;
    std::vector<std::string> output() const;

private:
    std::set<std::string> m_extraEditors;
    std::map<std::string, std::string> m_libraryToHeaderMap;
    std::vector<std::string> m_output;
};

#endif //EASYGPP_CONFIGURATIONFILEREADER_H
