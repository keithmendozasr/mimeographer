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
#include <vector>

#include <glog/logging.h>

#include <folly/io/IOBuf.h>

#include <boost/filesystem.hpp>

#include "UserHandler.h"
#include "HandlerError.h"
#include "HandlerRedirect.h"

using namespace std;
using namespace proxygen;
using namespace folly;

namespace mimeographer 
{

void UserHandler::buildLoginPage(const bool &showMismatch)
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

	const static string html = 
        "<form method=\"post\" action=\"/user/login\" enctype=\"multipart/form-data\" class=\"form-signin\" style=\"max-width:330px; margin:0 auto\">"
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

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
}

void UserHandler::processLogin()
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    const static string cookieName = "session";

    auto login = getPostParam("login");
    auto pass = getPostParam("password");
    if(login && pass && 
        login->type == PostParamType::VALUE && pass->type == PostParamType::VALUE)
    {
        VLOG(3) << "Login credentials supplied"
            << "\n\tLogin: " << login->value
            << "\n\tPassword: NOT PRINTED"
            << "\n\tUUID from cookie: " << session.getUUID();

        VLOG(1) << "Check provided credential";
        if(session.authenticateLogin(login->value, pass->value))
        {
            LOG(INFO) << "Login authenticated";
            addCookie(cookieName, session.getUUID());
            throw HandlerRedirect(HandlerRedirect::RedirCode::HTTP_303, "/");
        }
        else
            LOG(INFO) << "Login rejected";
    }
    else
        LOG(WARNING) << "POST missing login credential"; 

    // If code gets here; let user retry
    buildLoginPage(true);

    VLOG(2) << "End " <<  __PRETTY_FUNCTION__;
}

void UserHandler::processLogout()
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;
    session.logoutUser();
    VLOG(2) << "End " << __PRETTY_FUNCTION__;
    throw HandlerRedirect(HandlerRedirect::RedirCode::HTTP_303, "/");
}

void UserHandler::buildChangePassPage(const bool &showMismatch)
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

	const static string html = 
        "<form method=\"post\" action=\"/user/changepass\" "
            "enctype=\"multipart/form-data\" class=\"form-signin\" "
            "style=\"max-width:330px; margin:0 auto\">\n"
        "<h2 class=\"form-signin-heading\">Password change</h2>\n"
        "<label for=\"oldPass\">Curent Password</label>\n"
        "<input type=\"password\" name=\"oldpass\" id=\"oldPass\" "
            "class=\"form-control\" required autofocus>\n"
        "<label for=\"newPass\">New Password</label>\n"
        "<input type=\"password\" name=\"newpass\" id=\"newPass\" "
            "class=\"form-control\" required>\n"
        "<label for=\"repeatPass\">Repeat Password</label>\n"
        "<input type=\"password\" name=\"repeatpass\" id=\"repeatPass\" "
            "class=\"form-control\" required>\n"
        "<button class=\"btn btn-lg btn-primary btn-block\" type=\"submit\">"
            "Save"
        "</button>\n"
        "</form>\n";

    const static string mismatchBanner =
        "<div class=\"alert alert-primary\" role=\"alert\">"
            "Failed to change password. Try again!"
        "</div>";
    if(showMismatch)
    {
        VLOG(1) << "Showing mismatch banner";
        prependResponse(mismatchBanner);
    }
    else
        VLOG(1) << "Not showing mismatch banner";

    prependResponse(html);

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
}

void UserHandler::processChangePass()
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    auto oldPass = getPostParam("oldpass");
    auto newPass = getPostParam("newpass");
    auto repeatPass = getPostParam("repeatpass");
    if(oldPass && newPass && repeatPass &&
        oldPass->type == PostParamType::VALUE &&
        newPass->type == PostParamType::VALUE && 
        repeatPass->type == PostParamType::VALUE &&
        newPass->value == repeatPass->value)
    {
        VLOG(1) << "Attempt password change";
        if(session.changeUserPassword(oldPass->value, newPass->value))
        {
            LOG(INFO) << "Password changed";
            throw HandlerRedirect(HandlerRedirect::RedirCode::HTTP_303, "/");
        }
        else
            LOG(INFO) << "Password change failed";
    }
    else
        LOG(WARNING) << "POST missing data"; 

    // If code gets here; let user retry
    buildChangePassPage(true);

    VLOG(2) << "End " <<  __PRETTY_FUNCTION__;
}

void UserHandler::processRequest() 
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    auto path = getPath();
    if(path.back() == '/')
        path = path.substr(0,path.size()-1);

    if(path == "/user/login")
    {
        if(getMethod() == "POST")
        {
            LOG(INFO) << "Process login request";
            processLogin();
        }
        else if(getMethod() == "GET")
        {
            LOG(INFO) << "Display login page";
            auto sessionId = getCookie("session");
            VLOG(3) << "Value of session cookie: " << (sessionId ? *sessionId : "Not provided");
            if(!session.userAuthenticated())
                buildLoginPage();
            else
            {
                VLOG(1) << "User has active session, proceed to edit home page";

                VLOG(2) << "End " <<  __PRETTY_FUNCTION__;
                throw HandlerRedirect(HandlerRedirect::RedirCode::HTTP_303, "/edit");
            }
        }
    }
    else
    {
        VLOG(1) << "Not /user/login";

        auto sessionId = getCookie("session");
        VLOG(3) << "Value of session cookie: " << (sessionId ? *sessionId : "Not provided");
        if(!session.userAuthenticated())
        {
            LOG(INFO) << "Redirect to login page";
            addCookie("session", session.getUUID());

            VLOG(2) << "End " <<  __PRETTY_FUNCTION__;
            throw HandlerRedirect(HandlerRedirect::RedirCode::HTTP_303,
                "/user/login"
            );
        }
        
        LOG(INFO) << "User is logged-in";
        if(path == "/user/logout")
            processLogout();
        if(path == "/user/changepass")
        {
            if(getMethod() == "GET")
                buildChangePassPage();
            else if(getMethod() == "POST")
                processChangePass();
        }
        else
        {
            LOG(INFO) << path << "not handled";

            VLOG(2) << "End " <<  __PRETTY_FUNCTION__;
            throw HandlerError(404, "File not found");
        }
    }

    VLOG(2) << "End " <<  __PRETTY_FUNCTION__;
}

}
