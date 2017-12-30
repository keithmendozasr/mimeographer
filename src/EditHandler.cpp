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

#include <glog/logging.h>

#include <folly/io/IOBuf.h>

#include "EditHandler.h"
#include "HandlerError.h"

using namespace std;
using namespace proxygen;
using namespace folly;

namespace mimeographer 
{

void EditHandler::buildLoginPage()
{
	const static string html = 
        "<form method=\"post\" action=\"/edit/login\" enctype=\"multipart/form-data\" class=\"form-signin\" style=\"max-width:330px; margin:0 auto\">"
        "<h2 class=\"form-signin-heading\">Please sign in</h2>"
        "<label for=\"inputEmail\" class=\"sr-only\">Email address</label>"
        "<input type=\"email\" name=\"login\" id=\"inputEmail\" class=\"form-control\" placeholder=\"Email address\" required autofocus>"
        "<label for=\"inputPassword\" class=\"sr-only\">Password</label>"
        "<input type=\"password\" name=\"password\" id=\"inputPassword\" class=\"form-control\" placeholder=\"Password\" required>"
        "<button class=\"btn btn-lg btn-primary btn-block\" type=\"submit\">Sign in</button>"
        "</form>";
    prependResponse(IOBuf::copyBuffer(html));
}

void EditHandler::processLogin()
{
    if(getMethod() != "POST")
    {
        LOG(WARNING) << "Expected POST method when requesting " << getPath();
        throw HandlerError(405, "Unexpected request method");
    }

    auto param = getPostParam("login");
    string html = "<p><b>User name:</b> " + (param ? param->value : "UNKNOWN") + "</p>";
    param = getPostParam("password");
    html += "<p><b>Password:</b> " + (param ? param->value : "UNKOWN") + "</p>";
    addCookie("loggedin", "yes");   
    prependResponse(IOBuf::copyBuffer(html));
}

void EditHandler::processRequest() 
{
    auto path = getPath();
    if(path.back() == '/')
        path = path.substr(0,path.size()-1);

    if(path == "/edit")
    {
        LOG(INFO) << "Displaying login page";
        buildLoginPage();
    }
    else if(path == "/edit/login")
    {
        LOG(INFO) << "Process login request";
        processLogin();
    }
    else
    {
        LOG(INFO) << path << "not handled";
        throw HandlerError(404, "File not found");
    }
}

}
