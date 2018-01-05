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
#pragma once

#include <string>
#include <tuple>
#include <utility>

#include <boost/optional.hpp>
#include <uuid/uuid.h>

#include "DBConn.h"

namespace mimeographer
{

class UserSession
{
    FRIEND_TEST(UserSessionTest, constructor);
    FRIEND_TEST(UserSessionTest, hashPassword);
    FRIEND_TEST(UserSessionTest, authenticateLogin);

private:
    DBConn &db;
    std::string uuid;
    boost::optional<int> userId;
    std::string csrfkey;

    std::tuple<std::string, std::string>
        hashPassword(const std::string &pass, const std::string &salt = "");

    inline const std::string genUUID() const
    {
        uuid_t nUUID;
        uuid_generate_random(nUUID);
        char cTmp[37];
        uuid_unparse(nUUID, cTmp);

        return std::move(cTmp);
    }

public:
    explicit UserSession(DBConn &db, const std::string &uuid = "");

    inline const std::string &getUUID() const
    {
        return uuid;
    }

    inline boost::optional<int> getUserId() const
    {
        return userId;
    }

    ////
    /// Verify login credential
    /// \param email User's email
    /// \param password Cleartext password
    /// \return true if email/password combination matches
    ////
    const bool authenticateLogin(const std::string &email,
        const std::string &password);

    ////
    /// Check if the user's session is for an authenticated user
    ////
    const bool userAuthenticated()
    {
        return userId != boost::none;
    }

    ////
    /// Generate a new CSRF key for the given session
    ////
    const std::string genCSRFKey();

    ////
    /// Verify if CSRF key is correct
    /// \param csrfkey CSRF key to verify
    /// \return true if csrfkey is the expected key
    ////
    const bool verifyCSRFKey(const std::string &csrfkey);
};

} // namespace
