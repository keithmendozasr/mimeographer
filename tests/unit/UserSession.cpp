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
#include <iostream>

#include "gtest/gtest.h"

#include "params.h"
#include "UserSession.h"

using namespace std;

namespace mimeographer
{

class UserSessionTest : public ::testing::Test
{
protected:
    const char *testUUID = "4887ebff-f59e-4881-9a90-9bf4b80f415e";
    DBConn db = { FLAGS_dbUser, FLAGS_dbPass, FLAGS_dbHost, FLAGS_dbName };

    void resetSessionTable()
    {
        try
        {
            (void)db.execQuery("TRUNCATE session CASCADE");
        }
        catch(...)
        {
            LOG(WARNING) << "Exception encountered at " << __PRETTY_FUNCTION__;
        }
    }
};

TEST_F(UserSessionTest, constructor)
{
    UserSession obj(db);
    ASSERT_STREQ(obj.uuid.c_str(), "");
}

TEST_F(UserSessionTest, initSession)
{
    ASSERT_NO_THROW({
        db.saveSession(testUUID);
        UserSession obj(db);
        obj.initSession(testUUID);
        ASSERT_STREQ(obj.uuid.c_str(), testUUID);
    });
}

TEST_F(UserSessionTest, hashPassword)
{
    UserSession obj(db);
    auto salt = "VEOCBE1i2wM2tsrGwmLfsg8d74fv7M-AxsngFVcv2ow";
    auto ret = obj.hashPassword("123456", salt);
    auto hashTest = get<0>(ret);
    auto saltTest = get<1>(ret);
    ASSERT_EQ(hashTest, string("kt56uQBSTP-bT4ybmGCgsmU48BBx__mcE61X7UsWxpE"));
    ASSERT_EQ(saltTest, salt);
}

TEST_F(UserSessionTest, authenticateLogin)
{
    ASSERT_NO_THROW({
        UserSession obj(db);
        obj.initSession(testUUID);
        ASSERT_TRUE(obj.authenticateLogin("a@a.com", "123456"));
        ASSERT_EQ(obj.userId.value(), 1);
    });

    ASSERT_NO_THROW({
        UserSession obj(db);
        ASSERT_FALSE(obj.authenticateLogin("blank@example.com", "123456"));
        ASSERT_FALSE(obj.userId);
    });
}

TEST_F(UserSessionTest, userAuthenticated)
{
    {
        resetSessionTable();
        db.saveSession(testUUID);
        UserSession obj(db);
        obj.initSession(testUUID);
        ASSERT_FALSE(obj.userAuthenticated());
        obj.authenticateLogin("a@a.com", "123456");
        ASSERT_TRUE(obj.userAuthenticated());
    }

    {
        UserSession obj(db);
        obj.initSession(testUUID);
        ASSERT_TRUE(obj.userAuthenticated());
    }
}

} //namespace