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
#pragma once

#include <proxygen/httpserver/RequestHandler.h>
#include <proxygen/httpserver/ResponseBuilder.h>

#include <regex>
#include <string>
#include <exception>

#include "gtest/gtest_prod.h"

#include "HandlerBase.h"

namespace mimeographer 
{

class PrimaryHandler : public HandlerBase
{
    FRIEND_TEST(PrimaryHandlerTest, buildFrontPage);
    FRIEND_TEST(PrimaryHandlerTest, renderArticle_header);
    FRIEND_TEST(PrimaryHandlerTest, renderArticle_lists);
    FRIEND_TEST(PrimaryHandlerTest, renderArticle_paragraph);
    FRIEND_TEST(PrimaryHandlerTest, renderArticle_blockquote);
    FRIEND_TEST(PrimaryHandlerTest, renderArticle_link);
    FRIEND_TEST(PrimaryHandlerTest, renderArticle_image);
    FRIEND_TEST(PrimaryHandlerTest, renderArticle_codeblock);
    FRIEND_TEST(PrimaryHandlerTest, renderArticle_htmlblock);
    FRIEND_TEST(PrimaryHandlerTest, renderArticle_htmlinline);
    FRIEND_TEST(PrimaryHandlerTest, renderArticle_em);
    FRIEND_TEST(PrimaryHandlerTest, renderArticle_strong);

private:

    ////
    /// Parse the markdown for sending in the response
    /// \param data Markdown string to parse
    ////
    void renderArticle(const std::string &data);

    ////
    /// Render the site's front/index page
    ////
    void buildFrontPage();
    void buildArchive();
    void buildArticlePage();
    void processRequest();

public:
    PrimaryHandler(const Config &config) : HandlerBase(config) {};
};

}
