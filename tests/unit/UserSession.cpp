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

    void SetUp()
    {
        try
        {
            db.savePassword(1, "ko8hPecckl3hX4Exh7f3-sqvqJBVaLzH4thFE-vNU4U",
                "VEOCBE1i2wM2tsrGwmLfsg8d74fv7M-AxsngFVcv2ow");
            db.execQuery("DELETE FROM users "
                "WHERE email in ('newuser@example.com', 'newuse2r@example.com')");
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
    EXPECT_STREQ(obj.uuid.c_str(), "");
}

TEST_F(UserSessionTest, initSession)
{
    ASSERT_NO_THROW({
        db.saveSession(testUUID);
        UserSession obj(db);
        obj.initSession(testUUID);
        EXPECT_STREQ(obj.uuid.c_str(), testUUID);
    });
}

TEST_F(UserSessionTest, hashPassword)
{
    UserSession obj(db);
    auto salt = "VEOCBE1i2wM2tsrGwmLfsg8d74fv7M-AxsngFVcv2ow";
    auto ret = obj.hashPassword("123456", salt);
    auto hashTest = get<0>(ret);
    auto saltTest = get<1>(ret);
    EXPECT_EQ(hashTest, string("ko8hPecckl3hX4Exh7f3-sqvqJBVaLzH4thFE-vNU4U"));
    EXPECT_EQ(saltTest, salt);
}

TEST_F(UserSessionTest, authenticateLogin)
{
    ASSERT_NO_THROW({
        UserSession obj(db);
        obj.initSession(testUUID);
        EXPECT_TRUE(obj.authenticateLogin("a@a.com", "123456"));
        EXPECT_EQ(obj.userId.value(), 1);
    });

    EXPECT_NO_THROW({
        UserSession obj(db);
        EXPECT_FALSE(obj.authenticateLogin("blank@example.com", "123456"));
        EXPECT_FALSE(obj.userId);
    });

    EXPECT_NO_THROW({
        UserSession obj(db);
        EXPECT_FALSE(obj.authenticateLogin("a@a.com", "9876"));
        EXPECT_FALSE(obj.userId);
    });
}

TEST_F(UserSessionTest, userAuthenticated)
{
    {
        resetSessionTable();
        db.saveSession(testUUID);
        UserSession obj(db);
        obj.initSession(testUUID);
        EXPECT_FALSE(obj.userAuthenticated());
        obj.authenticateLogin("a@a.com", "123456");
        EXPECT_TRUE(obj.userAuthenticated());
    }

    {
        UserSession obj(db);
        obj.initSession(testUUID);
        EXPECT_TRUE(obj.userAuthenticated());
    }
}

TEST_F(UserSessionTest, changeUserPassword)
{
    UserSession obj(db);
    EXPECT_THROW({ obj.changeUserPassword("",""); }, invalid_argument);

    obj.userId = 1;
    EXPECT_NO_THROW({
        EXPECT_FALSE(obj.changeUserPassword("9876", "abcdef"));
        ASSERT_TRUE(obj.changeUserPassword("123456", "abcdef"));
    });

    obj.userId = 5;
    EXPECT_NO_THROW({ EXPECT_FALSE(obj.changeUserPassword("", "")); });
    obj.changeUserPassword("", "123456");
}

TEST_F(UserSessionTest, createLogin)
{
    UserSession obj(db);
    EXPECT_TRUE(obj.createLogin("newuser@example.com", "123456", "New User"));
    EXPECT_FALSE(obj.createLogin("newuser@example.com", "123456", "New User"));
    EXPECT_TRUE(obj.createLogin("newuse2r@example.com", "123456", "New User"));
}

} //namespace
