/*
 * Copyright 2017-present Keith Mendoza
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <array>
#include <fstream>
#include <cerrno>
#include <sstream>

#include <glog/logging.h>

#include "SiteTemplates.h"

using namespace std;

namespace mimeographer
{

std::map<string, string> SiteTemplates::templateItems;

void SiteTemplates::init(const Config &config)
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;
    auto templatePath = config.staticBase + "/templates/";

    VLOG(3) << "Template path: " << templatePath;
    array<string, 8> templateNames {
        "header",
        "navbase",
        "login",
        "editnav",
        "usernav",
        "navclose",
        "contentopen",
        "contentclose"
    };

    for(auto i : templateNames)
    {
        auto fileName = templatePath + i + ".html";
        VLOG(1) << "Load template file " << fileName;
        ifstream inFile(fileName, ios_base::in | ios_base::binary);
        if(!inFile.is_open())
        {
            int err = errno;
            ostringstream msg;
            msg << "Failed to open " << i <<" template file. Cause: "
                << strerror(err);
            LOG(ERROR) << msg.str();
            throw runtime_error(msg.str());
        }
        else
            VLOG(1) << "Reading template " << i << " content";

        string data;
        while(!inFile.eof())
        {
            const size_t tmpSize = 4096;
            char tmp[tmpSize];
            inFile.read(tmp, tmpSize-1);
            if(!inFile.eof() && !inFile.good())
            {
                int err = errno;
                ostringstream msg;
                msg << "Error reading " << i << ". Cause: " << strerror(err);
                LOG(ERROR) << msg.str();
                throw runtime_error(msg.str());
            }
            
            VLOG(3) << "Number of characters read: " << inFile.gcount();
            tmp[inFile.gcount()]='\0';
            VLOG(1) << "Chunk read from " << i << ": \"" << tmp << "\"";
            data += tmp;
        }

        VLOG(1) << "Storing " << i << " to map";
        VLOG(1) << "String to store: \"" << data << "\"";
        templateItems[i] = data;
    } //for(auto i in templateNames)

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
}

const string &SiteTemplates::getTemplate(const string &name)
{
    return templateItems.at(name);
}

} //namespace
