/*
 * Copyright 2017 Keith Mendoza
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

#include <string>
#include <exception>
#include <utility>
#include <regex>

#include <glog/logging.h>

#include "PrimaryHandler.h"
#include "HandlerError.h"

using namespace std;
using namespace proxygen;
using namespace folly;

namespace mimeographer 
{

void PrimaryHandler::buildFrontPage()
{
    VLOG(1) << "DB connection established";
    string data;
    auto conn = connectDb();
    for(auto article : conn.getHeadlines())
    {
        auto line = string("<h1><a href=\"/article/") + article[0] + "\">"
            + article[1] + "</a></h1>\n<p>"
            + article[2] + "</p>\n" ;
        if((data.capacity() - data.size() - line.size()) < 0)
        {
            VLOG(2) << "Loading existing list to buffer";
            prependResponse(data);
            data = line;
        }
        else
        {
            VLOG(2) << "Append line to data buffer";
            data = data + line;
        }
    }
    prependResponse(data);
    VLOG(1) << "Front page data processed";
}

void PrimaryHandler::buildArticlePage()
{
    static regex parser("/article/(\\d+)");
    smatch match;
    if(regex_match(getPath(), match, parser))
    {
        ssub_match id = match[1];
        VLOG(2) << "Article id: " << id.str();
        auto conn = connectDb();
        try
        {
            auto article = conn.getArticle(id.str());
            prependResponse(string("<h1>") + get<0>(article) + "</h1>");
            for(auto contPart : get<1>(article))
                prependResponse(contPart);
        }
        catch(const range_error &)
        {
            LOG(INFO) << "Caught unexpected number of articles";
            throw HandlerError(404, "Article " + id.str() + " not found");
        }
    }
    else
    {
        LOG(WARNING) << "path didn't parse";
        throw HandlerError(404, "File not found");
    }
}

void PrimaryHandler::processRequest() 
{
    auto path = getPath();
    VLOG(2) << "path.substr(9): " << path.substr(0,9);
    if(path == "/")
    {
        VLOG(1) << "Process front page";
        buildFrontPage();
    }
    else if(path.substr(0,9) == "/article/")
    {
        VLOG(1) << "Process article";
        buildArticlePage();
    }
    else
    {
        LOG(INFO) << path << "not handled";
        throw HandlerError(404, "File not found");
    }
}

}
