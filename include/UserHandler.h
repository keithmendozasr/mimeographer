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

class UserHandler : public HandlerBase
{
    FRIEND_TEST(UserHandlerTest, hashPassword);
    FRIEND_TEST(UserHandlerTest, buildLoginPage);
    FRIEND_TEST(UserHandlerTest, authenticateLogin);
    FRIEND_TEST(UserHandlerTest, processLogin);
    FRIEND_TEST(UserHandlerTest, changeUserPassword);

private:
    ////
    /// Generate salted password hash
    /// \return tuple of the salted password hash, and the salt used
    ////
    std::tuple<std::string, std::string>
        hashPassword(const std::string &pass, std::string salt = "");

    ////
    /// Verify login credential
    ////
    const bool authenticateLogin(const DBConn::UserRecord &userInfo,
        const std::string &password);

    ////
    /// Variant of authenticateLogin above
    /// \param email User's email
    /// \param password Cleartext password
    /// \return true if email/password combination matches
    ////
    const bool authenticateLogin(const std::string &email,
        const std::string &password);

    void buildLoginPage(const bool &showMismatch = false);
    void processLogin();
    void processLogout();

    void buildChangePassPage(const bool &showFail = false);

    ////
    /// Change user's password
    /// \param oldPass User's current password
    /// \param newPass User's new password
    /// \return true if oldPass authenticated, and the new password was saved
    ////
    bool changeUserPassword(const std::string &oldPass,
        const std::string &newPass);

    void processChangePass();

public:
    UserHandler(const Config &config) : HandlerBase(config) {}

    void processRequest() override;

    ////
    /// Create login credential
    /// \param email User's email
    /// \param password Cleartext password
    /// \return true if email/password saved
    ////
    const bool createLogin(const std::string &email, const std::string &password,
        const std::string &name);

};

} //namespace mimeographer
