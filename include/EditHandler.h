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

#include <regex>
#include <string>
#include <exception>

#include <proxygen/httpserver/RequestHandler.h>
#include <proxygen/httpserver/ResponseBuilder.h>

#include "HandlerBase.h"
#include "UserSession.h"

namespace mimeographer 
{

class EditHandler : public HandlerBase
{
    FRIEND_TEST(EditHandlerTest, buildLoginPage);
    FRIEND_TEST(EditHandlerTest, processLogin);

private:
    void buildLoginPage(const bool &showMismatch = false);
    void processLogin();
    void buildMainPage();
    void buildEditor(const std::string &articleId = "");
    void processSaveArticle();
    void buildEditSelect();
    void processEditArticle();
    void buildUploadPage();
    void processUpload();
    void processViewUpload();
    void processLogout();

public:
    EditHandler(const Config &config) : HandlerBase(config) {}

    void processRequest() override;
};

}
