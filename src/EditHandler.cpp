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
#include "HandlerRedirect.h"
#include "UserSession.h"

using namespace std;
using namespace proxygen;
using namespace folly;

namespace mimeographer 
{

void EditHandler::buildLoginPage(const bool &showMismatch)
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

    const static string mismatchBanner =
        "<div class=\"alert alert-primary\" role=\"alert\">"
            "Username/password incorrect. Try again!"
        "</div>";
    if(showMismatch)
    {
        VLOG(1) << "Showing mismatch banner";
        prependResponse(mismatchBanner);
    }
    else
        VLOG(1) << "Not showing mismatch banner";

    prependResponse(html);
}

void EditHandler::processLogin()
{
    const static string cookieName = "session";

    auto login = getPostParam("login");
    auto pass = getPostParam("password");
    if(login && pass && 
        login->type == PostParamType::VALUE && pass->type == PostParamType::VALUE)
    {
        auto dbConn = connectDb();
        auto uuid = getCookie(cookieName);
        if(!uuid)
            uuid = "";

        VLOG(1) << "Login credentials supplied";
        VLOG(3) << "Login: " << login->value
            << "\n\tPassword: NOT PRINTED"
            << "\n\tUUID from cookie: " << *uuid;

        UserSession session(dbConn, *uuid);
        VLOG(1) << "Check provided credential";
        if(session.authenticateLogin(login->value, pass->value))
        {
            LOG(INFO) << "Login authenticated";
            addCookie(cookieName, session.getUUID());
            throw HandlerRedirect(HandlerRedirect::RedirCode::HTTP_303, "/edit");
        }
        else
            LOG(INFO) << "Login rejected";
    }
    else
        LOG(WARNING) << "POST missing login credential"; 

    // If code gets here; let user retry
    buildLoginPage(true);
}

void EditHandler::processRequest() 
{
    auto path = getPath();
    if(path.back() == '/')
        path = path.substr(0,path.size()-1);

    if(path == "/edit/login")
    {
        if(getMethod() == "POST")
        {
            LOG(INFO) << "Process login request";
            processLogin();
        }
        else if(getMethod() == "GET")
        {
            LOG(INFO) << "Display login page";
            buildLoginPage();
        }
    }
    else
    {
        VLOG(1) << "Not /edit/login";

        auto dbConn = connectDb();
        auto sessionId = getCookie("session");
        VLOG(3) << "Value of session cookie: " << (sessionId ? *sessionId : "Not provided");
        UserSession session(dbConn, (sessionId ? *sessionId : ""));
        if(!session.userAuthenticated())
        {
            LOG(INFO) << "Redirect to login page";
            addCookie("session", session.getUUID());
            throw HandlerRedirect(HandlerRedirect::RedirCode::HTTP_303,
                "/edit/login"
            );
        }
        
        LOG(INFO) << "User is logged-in";
        if(path == "/edit")
            prependResponse("<p>Edit page here</p>");
        else
        {
            LOG(INFO) << path << "not handled";
            throw HandlerError(404, "File not found");
        }
    }
}

}
