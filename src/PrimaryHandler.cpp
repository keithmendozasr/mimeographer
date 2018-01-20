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
#include <sstream>
#include <cstdlib>

#include <glog/logging.h>
#include <cmark.h>

#include "PrimaryHandler.h"
#include "HandlerError.h"

using namespace std;
using namespace proxygen;
using namespace folly;

namespace mimeographer 
{

void PrimaryHandler::buildFrontPage()
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    VLOG(1) << "DB connection established";
    string data;
    for(auto article : db.getHeadlines())
    {
        ostringstream line;
        line << "<h1><a href=\"/article/" << get<0>(article) << + "\">"
            << get<1>(article) << "</a></h1>\n<div class=\"col col-12\" >";

        {
            // TODO: Re-evaluate
            auto str = get<2>(article);
            auto tmp = cmark_markdown_to_html(str.c_str(), str.size(), CMARK_OPT_DEFAULT);
            line << tmp << "</div>\n" ;
            free(tmp);
        }

        if((data.capacity() - data.size() - line.str().size()) < 0)
        {
            VLOG(2) << "Loading existing list to buffer";
            prependResponse(data);
            data = line.str();
        }
        else
        {
            VLOG(2) << "Append line to data buffer";
            data = data + line.str();
        }
    }
    prependResponse(data);
    VLOG(1) << "Front page data processed";

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
}

void PrimaryHandler::buildArticlePage()
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    static regex parser("/article/(\\d+)");
    smatch match;
    if(regex_match(getPath(), match, parser))
    {
        ssub_match id = match[1];
        VLOG(2) << "Article id: " << id.str();
        try
        {
            auto article = db.getArticle(id.str());
            auto tmp = article;
            auto body = cmark_markdown_to_html(tmp.c_str(), tmp.size(), CMARK_OPT_DEFAULT);
            prependResponse(string(body));
            free(body);
        }
        catch(const range_error &)
        {
            LOG(INFO) << "Caught unexpected number of articles";

            VLOG(2) << "End " << __PRETTY_FUNCTION__;
            throw HandlerError(404, "Article " + id.str() + " not found");
        }
    }
    else
    {
        LOG(WARNING) << "path didn't parse";

        VLOG(2) << "End " << __PRETTY_FUNCTION__;
        throw HandlerError(404, "File not found");
    }

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
}

void PrimaryHandler::processRequest() 
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

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
        LOG(INFO) << path << " not handled";

        VLOG(2) << "End " << __PRETTY_FUNCTION__;
        throw HandlerError(404, "File not found");

    }

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
}

}
