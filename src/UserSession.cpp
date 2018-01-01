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
#include <fstream>
#include <cerrno>
#include <cstring>
#include <system_error>

#include <uuid/uuid.h>
#include <proxygen/lib/utils/Base64.h>
#include <folly/ssl/OpenSSLHash.h>

#include "UserSession.h"

using namespace std;
using namespace proxygen;
using namespace folly;
using namespace folly::ssl;

namespace mimeographer
{

UserSession::UserSession(DBConn &db, const string &uuid) :
    db(db), uuid(uuid)
{
    VLOG(2) << "Start " << __PRETTY_FUNCTION__;
    if(uuid == "")
    {
        VLOG(1) << "Generate new UUID";
        uuid_t nUUID;
        uuid_generate_random(nUUID);
        char cTmp[37];
        uuid_unparse(nUUID, cTmp);
        this->uuid = cTmp;
    }
    else
        VLOG(1) << "UUID provided";

    try
    {
        VLOG(1) << "Save UUID to session DB";
        db.saveSession(this->uuid);
        userId = db.getMappedUser(this->uuid);
    }
    catch (DBConn::DBError &e)
    {
        LOG(ERROR) << "DB-related error encountered at user session: "
            << e.what() << " proceeding as unauthenticated user";
    }

    VLOG(2) << "End " << __PRETTY_FUNCTION__;
}

tuple<string,string> UserSession::hashPassword(const string &pass,
    const string &salt)
{
    string useSalt;
    if(salt == "")
    {
        VLOG(1) << "Generating new salt";
        ifstream rndFile("/dev/urandom", ios_base::in | ios_base::binary);
        if(!rndFile)
        {
            auto err = errno;
            throw runtime_error(
                string("Failed to open /dev/random. Cause: ") + strerror(err)
            );
        }

        VLOG(2) << "/dev/urandom open";
        unsigned char rndBuf[32];
        rndFile.read((char *)rndBuf, 32);
        if(!rndFile)
            throw runtime_error("Failed to read from /dev/urandom");

        VLOG(2) << "BASE64 encode seed";
        useSalt = Base64::urlEncode(ByteRange(rndBuf, 32));
    }
    else
    {
        VLOG(2) << "Use existing salt";
        useSalt = salt;
    }

    VLOG(2) << "Calculate hash. Salt to use: " << useSalt;
    unsigned char hash[32];
    OpenSSLHash::sha256(MutableByteRange(hash, 32), ByteRange((unsigned char *)useSalt.c_str(), (size_t)useSalt.size()));
    string hashStr = Base64::urlEncode(ByteRange(hash, 32));
    VLOG(2) << "sha256 output: " << hashStr;
    
    return move(make_tuple(hashStr, useSalt));
}

const bool UserSession::authenticateLogin(const std::string &email,
    const std::string &password)
{
    bool rslt = false;
    auto dbRet = db.getUserInfo(email);
    if(!dbRet)
        LOG(INFO) << "User info not found";
    else
    {
        VLOG(1) << "User info retrieved from DB";
        auto userInfo = *dbRet;
        auto hash = hashPassword(password, userInfo[(int)DBConn::UserParts::salt]);
        auto savePass = get<0>(hash);
        if(savePass != userInfo[(int)DBConn::UserParts::password])
            LOG(INFO) << "Password mismatch";
        else
        {   
            LOG(INFO) << "User authenticated";
            userId = stoi(userInfo[(int)DBConn::UserParts::id]);
            db.mapUuidToUser(uuid, *userId);
            rslt = true;
        }
    }

    return rslt;
}

} // namespace
