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

#include <stdexcept>
#include <sstream>

#include "glog/logging.h"

#include "SummaryBuilder.h"
#include "HandlerError.h"

using namespace std;

namespace mimeographer
{

void SummaryBuilder::buildTitle()
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;
    cmark_event_type evType;
    while((evType = cmark_iter_next(iterator.get())) != CMARK_EVENT_DONE)
    {
        auto node = cmark_iter_get_node(iterator.get());
        auto nodeType = cmark_node_get_type(node);
        if(evType == CMARK_EVENT_ENTER)
        {
            VLOG(2) << "Entering node " << cmark_node_get_type_string(node);
            if(nodeType == CMARK_NODE_TEXT)
            {
                auto text = cmark_node_get_literal(node);
                VLOG(2) << "Append text to title \"" << text << "\"";
                title += text;
            }
            else
                VLOG(2) << "Skipping node";
        }
        else if(evType == CMARK_EVENT_EXIT && nodeType == CMARK_NODE_HEADING)
        {
            VLOG(2) << "Done collecting title";
            break;
        }
    }

    VLOG(3) << "New value of title: " << title;
    if(title == "")
        throw invalid_argument("Header missing from provided markdown");

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
}

void SummaryBuilder::buildPreview()
{
    cmark_event_type evType;
    while((evType = cmark_iter_next(iterator.get())) != CMARK_EVENT_DONE &&
        preview.size() < 256)
    {
        auto node = cmark_iter_get_node(iterator.get());
        auto nodeType = cmark_node_get_type(node);
        if(evType == CMARK_EVENT_ENTER)
        {
            VLOG(2) << "Entering node " << cmark_node_get_type_string(node);
            if(nodeType == CMARK_NODE_TEXT)
            {
                auto text = cmark_node_get_literal(node);
                VLOG(2) << "Append text to title \"" << text << "\"";
                preview += text;
            }
            else
                VLOG(2) << "Skipping node";
        }
        else if(evType == CMARK_EVENT_EXIT && nodeType == CMARK_NODE_PARAGRAPH)
        {
            VLOG(2) << "Done collecting preview";
            break;
        }
    }

    preview = preview.substr(0,256);
    VLOG(3) << "New value of preview:\n" << preview;
    if(preview == "")
        throw invalid_argument("No paragraphs provided in markdown");

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
}

void SummaryBuilder::build(const std::string &markdown)
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;
    unique_ptr<cmark_node, function<void(cmark_node*)>> rootNode(
        cmark_parse_document(markdown.c_str(), markdown.size(),
            CMARK_OPT_DEFAULT),
        [](cmark_node *node)
        {
            if(node)
                cmark_node_free(node);
        }
    );

    iterator = move(
        shared_ptr<cmark_iter>(cmark_iter_new(rootNode.get()), 
            [](cmark_iter *iter)
            {
                if(iter)
                    cmark_iter_free(iter);
            }
    ));

    // Skip the DOCUMENT entry node
    cmark_iter_next(iterator.get());
    cmark_event_type evType = cmark_iter_next(iterator.get());
    if(evType == CMARK_EVENT_ENTER)
    {
        auto node = cmark_iter_get_node(iterator.get());
        auto nodeType = cmark_node_get_type(node);
        if(nodeType != CMARK_NODE_HEADING)
        {
            LOG(INFO) << "Submitted article doesn't start with heading";
            throw HandlerError(400, "Heading missing from posted article");
        }

        buildTitle();
    }

    VLOG(1) << "Article title extracted";

    while((evType = cmark_iter_next(iterator.get())) != CMARK_EVENT_DONE)
    {
        auto node = cmark_iter_get_node(iterator.get());
        auto nodeType = cmark_node_get_type(node);
        if(evType == CMARK_EVENT_ENTER)
        {
            if(nodeType == CMARK_NODE_PARAGRAPH)
            {
                VLOG(1) << "Get article preview";
                buildPreview();
                break;
            }
        }
    }

    VLOG(1) << "Preview extracted";

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
}

}
