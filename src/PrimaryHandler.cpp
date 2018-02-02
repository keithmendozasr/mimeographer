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
            << get<1>(article) << "</a></h1>\n<div class=\"col col-12\" >"
            << get<2>(article) << "\n</div>\n";

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

void PrimaryHandler::renderArticle(const string &data)
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;
    unique_ptr<cmark_node, function<void(cmark_node*)>> rootNode(
        cmark_parse_document(data.c_str(), data.size(),
            CMARK_OPT_DEFAULT),
        [](cmark_node *node)
        {
            if(node)
                cmark_node_free(node);
        }
    );

    auto iterator = move(
        shared_ptr<cmark_iter>(cmark_iter_new(rootNode.get()), 
            [](cmark_iter *iter)
            {
                if(iter)
                    cmark_iter_free(iter);
            }
    ));

    bool inItem;
    string body;
    cmark_event_type evType;
    while((evType = cmark_iter_next(iterator.get())) != CMARK_EVENT_DONE)
    {
        auto node = cmark_iter_get_node(iterator.get());
        auto nodeType = cmark_node_get_type(node);
        ostringstream chunk;
        if(evType == CMARK_EVENT_ENTER)
        {
            VLOG(2) << "Node entry " << cmark_node_get_type_string(node);
            switch(nodeType)
            {
            case CMARK_NODE_DOCUMENT:
                VLOG(1) << "Start of document";
                break;
            case CMARK_NODE_HEADING:
                VLOG(1) << "Open header tag";
                chunk << "<h" << cmark_node_get_heading_level(node) << ">";
                break;
            case CMARK_NODE_PARAGRAPH:
                if(!inItem)
                {
                    VLOG(1) << "Open paragraph tag";
                    chunk << "<p>";
                }
                else
                    VLOG(1) << "Skip adding <p> tag";
                break;
            case CMARK_NODE_LIST:
                switch(cmark_node_get_list_type(node))
                {
                case CMARK_BULLET_LIST:
                    VLOG(1) << "Open bullet list";
                    chunk << "<ul>";
                    break;
                case CMARK_ORDERED_LIST:
                    VLOG(1) << "Open ordered list";
                    chunk << "<ol>";
                    break;
                default:
                    throw logic_error("List type unknown");
                }
                chunk << "\n";
                break;
            case CMARK_NODE_ITEM:
                VLOG(1) << "Open item";
                inItem = true;
                chunk << "<li>";
                break;
            case CMARK_NODE_BLOCK_QUOTE:
                VLOG(1) << "Open blockquote tag";
                chunk << "<blockquote>";
                break;
            case CMARK_NODE_LINK:
                VLOG(1) << "Open link tag";
                chunk << "<a href=\"" << cmark_node_get_url(node) << "\"";
                {
                    string title = cmark_node_get_title(node);
                    if(title.size())
                        chunk << " title=\"" << title << "\"";
                }
                chunk << ">";
                break;
            case CMARK_NODE_IMAGE:
                VLOG(1) << "Begin 1st part of image tag";
                chunk << "<img class=\"img-fluid\" "
                    << "src=\"" << cmark_node_get_url(node) << "\"";
                {
                    string title = cmark_node_get_title(node);
                    if(title.size())
                        chunk << " title=\"" << title << "\"";
                }
                chunk << " alt=\"";
                // If there's a title
                break;
            case CMARK_NODE_CODE_BLOCK:
                VLOG(1) << "Render code block";
                // These nodes do not have exit events. So, it must be closed
                // here
                chunk << "<pre><code>" << cmark_node_get_literal(node)
                    << "</code></pre>\n";
                break;
            case CMARK_NODE_CODE:
                VLOG(1) << "Render code";

                // These nodes do not have exit events. So, it must be closed
                // here
                chunk << "<code>" << cmark_node_get_literal(node)
                    << "</code>\n";
                break;
            case CMARK_NODE_HTML_BLOCK:
                VLOG(1) << "Render raw HTML";
                chunk << cmark_node_get_literal(node);
                break;
            case CMARK_NODE_HTML_INLINE:
                VLOG(1) << "Render raw HTML inline";
                chunk << cmark_node_get_literal(node);
                break;
            case CMARK_NODE_TEXT:
                VLOG(1) << "Render text";
                chunk << cmark_node_get_literal(node);
                break;
            case CMARK_NODE_LINEBREAK:
                VLOG(1) << "Render br tag";
                chunk << "<br />\n";
                break;
            default:
                VLOG(1) << "Ignoring node";
                break;
            }
        }
        else if(evType == CMARK_EVENT_EXIT)
        {
            VLOG(2) << "Node exit " << cmark_node_get_type_string(node);
            switch(nodeType)
            {
            case CMARK_NODE_DOCUMENT:
                VLOG(1) << "End of document";
                break;
            case CMARK_NODE_HEADING:
                VLOG(1) << "Render header tag";
                chunk << "</h" << cmark_node_get_heading_level(node) << ">";
                break;
            case CMARK_NODE_PARAGRAPH:
                if(!inItem)
                {
                    VLOG(1) << "Close paragraph tag";
                    chunk << "</p>";
                }
                else
                    VLOG(1) << "Skip adding </p> tag";
                break;
            case CMARK_NODE_LIST:
                switch(cmark_node_get_list_type(node))
                {
                case CMARK_BULLET_LIST:
                    VLOG(1) << "Close bullet list";
                    chunk << "</ul>";
                    break;
                case CMARK_ORDERED_LIST:
                    VLOG(1) << "Close ordered list";
                    chunk << "</ol>";
                    break;
                default:
                    throw logic_error("List type unknown");
                }
                break;
            case CMARK_NODE_ITEM:
                VLOG(1) << "Open item";
                inItem = false;
                chunk << "</li>";
                break;
            case CMARK_NODE_BLOCK_QUOTE:
                VLOG(1) << "Close blockquote tag";
                chunk << "</blockquote>";
                break;
            case CMARK_NODE_LINK:
                VLOG(1) << "Close link tag";
                chunk << "</a>";
                break;
            case CMARK_NODE_IMAGE:
                VLOG(1) << "Wrap up image tag";
                chunk << "\" />";
                break;
            default:
                VLOG(1) << "Ignoring node";
                break;
            }
            chunk << "\n";
        }

        if((body.capacity() - body.size() - chunk.str().size()) < 0)
        {
            VLOG(2) << "Loading existing chunk to buffer";
            prependResponse(body);
            body = chunk.str();
        }
        else
        {
            VLOG(2) << "Append chunk to buffer";
            body += chunk.str();
        }
    }
    
    prependResponse(body);

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
            renderArticle(db.getArticle(id.str()));
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
