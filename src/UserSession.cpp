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
#include <proxygen/lib/utils/Base64.h>
#include <folly/ssl/OpenSSLHash.h>
#include <boost/optional/optional_io.hpp>

#include "UserSession.h"

using namespace std;
using namespace proxygen;
using namespace folly;
using namespace folly::ssl;

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

tuple<string,string> UserSession::hashPassword(const string &pass,
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

const bool UserSession::authenticateLogin(const std::string &email,
    const std::string &password)
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    bool rslt = false;
    auto dbRet = db.getUserInfo(email);
    if(!dbRet)
        LOG(INFO) << "User info not found";
    else
    {
        VLOG(1) << "User info retrieved from DB";
        auto userInfo = *dbRet;
        auto hash = hashPassword(password, get<3>(userInfo));
        auto savePass = get<0>(hash);
        auto expectPass = get<4>(userInfo);

        VLOG(3) << "Save pass: " << savePass << "\n"
            << "expectPass: " << expectPass;
        if(savePass != expectPass)
            LOG(INFO) << "Password mismatch";
        else
        {   
            LOG(INFO) << "User authenticated";
            userId = get<0>(userInfo);
            db.mapUuidToUser(uuid, *userId);
            rslt = true;
        }
    }

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
    return rslt;
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

bool UserSession::changeUserPassword(const string &oldPass,
    const string &newPass)
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;

    if(userId == boost::none)
    {
        VLOG(2) << "End " << __PRETTY_FUNCTION__;
        throw invalid_argument("User ID not initialized, can't store new password");
    }

    auto userInfo = db.getUserInfo(*userId);
    if(userInfo == boost::none)
    {
        LOG(WARNING) << "User info for ID " << userId << " not found";
        VLOG(2) << "End " << __PRETTY_FUNCTION__;
        return false;
    }
    else
        VLOG(1) << "User info for " << *userId << " found";

    auto hash = hashPassword(oldPass, get<3>(*userInfo));
    if(get<0>(hash) != get<4>(*userInfo))
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

    db.savePassword(*userId, newHash, newSalt);

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
    return true;
}

const bool UserSession::createLogin(const string &email, const string &password,
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

} // namespace
