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
    FRIEND_TEST(UserSessionTest, initSession);

    FRIEND_TEST(HandlerBaseTest, buildPageHeader);

    FRIEND_TEST(UserHandlerTest, changeUserPassword);

private:
    DBConn &db;
    std::string uuid;
    boost::optional<int> userId;
    std::string csrfkey;


    inline const std::string genUUID() const
    {
        VLOG(2) << "Start " << __PRETTY_FUNCTION__;

        uuid_t nUUID;
        uuid_generate_random(nUUID);
        char cTmp[37];
        uuid_unparse(nUUID, cTmp);

        VLOG(2) << "End " << __PRETTY_FUNCTION__;
        return std::move(cTmp);
    }

public:
    explicit UserSession(DBConn &db);
    
    void initSession(const std::string &uuid = "");

    inline const std::string &getUUID() const
    {
        return uuid;
    }

    inline boost::optional<int> getUserId() const
    {
        return userId;
    }


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

    ////
    /// "Logout" user
    ////
    void logoutUser()
    {
        db.unmapUuidToUser(uuid, *userId);
        userId = boost::none;
    }

};

} // namespace
