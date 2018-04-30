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
#include <fstream>
#include <cerrno>
#include <cstring>
#include <system_error>

#include <uuid/uuid.h>
#include <boost/optional/optional_io.hpp>

#include "UserSession.h"

using namespace std;

namespace mimeographer
{

UserSession::UserSession(DBConn &db) :
    db(db)
{}

void UserSession::initSession(const std::string &uuid)
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    if(uuid == "")
    {
        VLOG(1) << "Generate new UUID";
        this->uuid = genUUID();
    }
    else
    {
        VLOG(1) << "UUID provided";
        this->uuid = uuid;
        try
        {
            VLOG(1) << "Get session data from DB";
            auto session = db.getSessionInfo(this->uuid);
            if(session)
            {
                VLOG(1) << "Session still active";
                userId = get<1>(*session);
                VLOG(1) << "Associated user: " << userId;
            }
            else
            {
                VLOG(1) << "Session already expired, regenerate a new one";
                this->uuid = genUUID();
            }

        }
        catch (DBConn::DBError &e)
        {
            LOG(ERROR) << "DB-related error encountered at user session: "
                << e.what() << " proceeding as unauthenticated user";
        }
    }

    try
    {
        VLOG(1) << "Refresh session in DB";
        db.saveSession(this->uuid);
    }
    catch (DBConn::DBError &e)
    {
        LOG(ERROR) << "DB-related error encountered at user session : "
            << e.what() << " proceeding as unauthenticated user";
        userId = boost::none;
    }

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
}

const std::string UserSession::genCSRFKey()
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    if(uuid == "" || !userId)
    {
        VLOG(2) << "End " << __PRETTY_FUNCTION__;
        throw logic_error("UUID or User ID not know at CSRF key generation");
    }
    
    string csrf = genUUID();
    VLOG(3) << "CSRF generated: " << csrf;

    db.saveCSRFKey(csrf, *userId, uuid);

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
    return move(csrf);
}

const bool UserSession::verifyCSRFKey(const string &csrfkey)
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    if(!userId)
    {
        LOG(WARNING) << "User ID not set when verifying CSRF";

        VLOG(2) << "End " << __PRETTY_FUNCTION__;
        return false;
    }
    auto expectedKey = db.getCSRFKey(*userId, uuid);
    if(!expectedKey)
    {
        LOG(WARNING) << "No CSRF key associated to user id " << *userId
            << " session " << uuid;

        VLOG(2) << "End " << __PRETTY_FUNCTION__;
        return false;
    }

    VLOG(3) << "Value of expected key: " << *expectedKey;

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
    return *expectedKey == csrfkey;
}


} // namespace
