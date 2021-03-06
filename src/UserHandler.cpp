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
#include <proxygen/lib/utils/Base64.h>
#include <folly/ssl/OpenSSLHash.h>

#include <boost/filesystem.hpp>

#include "UserHandler.h"
#include "HandlerError.h"
#include "HandlerRedirect.h"

using namespace std;
using namespace proxygen;
using namespace folly;
using namespace folly::ssl;

namespace mimeographer 
{

tuple<string,string> UserHandler::hashPassword(const string &pass,
    string salt)
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    string useSalt;
    if(salt == "")
    {
        VLOG(1) << "Generating new salt";
        ifstream rndFile("/dev/urandom", ios_base::in | ios_base::binary);
        if(!rndFile)
        {
            auto err = errno;

            VLOG(2) << "End " << __PRETTY_FUNCTION__;
            throw runtime_error(
                string("Failed to open /dev/random. Cause: ") + strerror(err)
            );
        }

        VLOG(3) << "/dev/urandom open";
        unsigned char rndBuf[32];
        rndFile.read((char *)rndBuf, 32);
        if(!rndFile)
        {
            VLOG(2) << "End " << __PRETTY_FUNCTION__;
            throw runtime_error("Failed to read from /dev/urandom");
        }

        VLOG(1) << "BASE64 encode seed";
        salt = Base64::urlEncode(ByteRange(rndBuf, 32));
    }
    else
        VLOG(1) << "Use existing salt";

    VLOG(1) << "Calculate hash.";
    auto saltedPass = salt + pass;
    unsigned char hash[32];
    OpenSSLHash::sha256(MutableByteRange(hash, 32), ByteRange((unsigned char *)saltedPass.c_str(), (size_t)saltedPass.size()));
    string hashStr = Base64::urlEncode(ByteRange(hash, 32));
    VLOG(2) << "sha256 output: " << hashStr;
    
    VLOG(2) << "End " << __PRETTY_FUNCTION__;
    return move(make_tuple(hashStr, salt));
}

const bool UserHandler::authenticateLogin(const DBConn::UserRecord &userInfo,
    const string &password)
{
    bool rslt = false;
    auto hash = hashPassword(password, get<3>(*userInfo));
    auto savePass = get<0>(hash);
    auto expectPass = get<4>(*userInfo);

    VLOG(3) << "Save pass: " << savePass << "\n"
        << "expectPass: " << expectPass;
    if(savePass != expectPass)
        LOG(INFO) << "Password mismatch";
    else
    {   
        LOG(INFO) << "User authenticated";
        rslt = true;
    }

    return rslt;
}

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
    if(login && pass && login->type == PostParamType::VALUE &&
        pass->type == PostParamType::VALUE)
    {
        VLOG(3) << "Login credentials supplied"
            << "\n\tLogin: " << login->value
            << "\n\tPassword: NOT PRINTED"
            << "\n\tUUID from cookie: " << session.getUUID();

        VLOG(1) << "Check provided credential";
        auto dbRet = db.getUserInfo(login->value);
        if(dbRet)
        {
            VLOG(1) << "User info retrieved from DB";
            if(authenticateLogin(*dbRet, pass->value))
            {
                LOG(INFO) << "Login authenticated";
                db.mapUuidToUser(session.getUUID(), get<0>(*dbRet));
                addCookie(cookieName, session.getUUID());
                VLOG(2) << "End " << __PRETTY_FUNCTION__;
                throw HandlerRedirect(HandlerRedirect::RedirCode::HTTP_303, "/");
            }
            else
                LOG(INFO) << "User authentication failed";
        }
        else
            LOG(INFO) << "User info not found";
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

bool UserHandler::changeUserPassword(const string &oldPass,
    const string &newPass)
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;
    int userId;
    {
        auto tmp = session.getUserId();
        if(tmp == boost::none)
        {
            VLOG(2) << "End " << __PRETTY_FUNCTION__;
            throw invalid_argument("User ID not initialized, can't store new password");
        }
        userId = *tmp;
    }

    auto userInfo = db.getUserInfo(userId);
    if(userInfo == boost::none)
    {
        LOG(WARNING) << "User info for ID " << userId << " not found";
        VLOG(2) << "End " << __PRETTY_FUNCTION__;
        return false;
    }
    else
        VLOG(1) << "User info for " << userId << " found";

    if(!authenticateLogin(userInfo, oldPass))
    {
        LOG(INFO) << "Failed to authenticate user " << userId
            << "'s current password";
        VLOG(2) << "End " << __PRETTY_FUNCTION__;
        return false;
    }
    else
        VLOG(1) << "User " << userId << "'s current password validated";

    string newHash, newSalt;
    tie(newHash, newSalt) = hashPassword(newPass);
    VLOG(1) << "New hash: " << newHash;
    VLOG(1) << "New salt: " << newSalt;

    db.savePassword(userId, newHash, newSalt);

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
    return true;
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
        if(changeUserPassword(oldPass->value, newPass->value))
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

void UserHandler::buildAddUserPage()
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;
    const static string html = 
        "<form method=\"post\" action=\"/user/add\" "
            "enctype=\"multipart/form-data\" class=\"form-signin\" "
            "style=\"max-width:330px; margin:0 auto\">\n"
        "<h2 class=\"form-signin-heading\">Add New User</h2>\n"
        "<label for=\"fullname\">Full name</label>\n"
        "<input type=\"text\" name=\"fullname\" id=\"fullname\" "
            "class=\"form-control\" required autofocus>\n"
        "<label for=\"email\">Email</label>\n"
        "<input type=\"text\" name=\"email\" id=\"email\" "
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

    prependResponse(html);

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
}

void UserHandler::processAddUser()
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    auto fullName = getPostParam("fullname");
    auto email = getPostParam("email");
    auto newPass = getPostParam("newpass");
    if(fullName && email && newPass &&
        fullName->type == PostParamType::VALUE &&
        email->type == PostParamType::VALUE &&
        newPass->type == PostParamType::VALUE)
    {
        VLOG(1) << "Adding new user with email " << email->value;
        if(!createLogin(email->value, newPass->value, fullName->value))
            throw HandlerError(200, "Provided email already exists");
    }
    else
    {
        LOG(WARNING) << "POST missing data";
        throw HandlerError(400, "POST missing data");
    }

    LOG(INFO) << "User " << email->value << " added";
    prependResponse("User added");
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
        else if(path == "/user/add")
        {
            if(getMethod() == "GET")
                buildAddUserPage();
            else if(getMethod() == "POST")
                processAddUser();
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

const bool UserHandler::createLogin(const string &email, const string &password,
    const string &name)
{
    VLOG(1) << "Start " << __PRETTY_FUNCTION__;

    bool rslt = false;
    auto dbRet = db.getUserInfo(email);
    if(dbRet)
        LOG(INFO) << "Email already in database";
    else
    {
        VLOG(1) << "Email not in DB";
        auto hash = hashPassword(password);
        VLOG(3) << "Pass hash: " << get<0>(hash)
            << " Salt: " << get<1>(hash);
        VLOG(1) << "Saving new user to database";
        db.addUser(email, get<0>(hash), get<1>(hash), name);

        rslt = true;
    }

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
    return rslt;
}

}
