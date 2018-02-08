#include <iostream>
#include <string>

#include "params.h"
#include "DBConn.h"
#include "gtest/gtest.h"

using namespace std;

namespace mimeographer
{
class DBConnTest : public ::testing::Test 
{
protected:
    const char *testUUID = "4887ebff-f59e-4881-9a90-9bf4b80f415e";
    const int testUserId = 1;
    DBConn testConn = { FLAGS_dbUser, FLAGS_dbPass, FLAGS_dbHost,
        FLAGS_dbName };
};

TEST_F(DBConnTest, urlEncode)
{
    DBConn conn;
    EXPECT_EQ(conn.urlEncode(" "), string("%20"));
    EXPECT_EQ(conn.urlEncode("Hello"), string("Hello"));
    EXPECT_EQ(conn.urlEncode("Hello there"), string("Hello%20there"));
    EXPECT_EQ(conn.urlEncode("/blah %"), string("%2Fblah%20%25"));
}

TEST_F(DBConnTest, constructor)
{
    ASSERT_THROW({
        DBConn conn("testuser", "", "localhost", "");
        ASSERT_EQ(conn.conn, nullptr);
    }, DBConn::DBError);

    ASSERT_NO_THROW({
        DBConn("testuser", "123456", "localhost", "mimeographer");
    });

    {
        DBConn conn("testuser", "123456", "localhost", "mimeographer");
        ASSERT_NE(conn.conn, nullptr);
    }
}

TEST_F(DBConnTest, getHeadlines)
{
    static const string leadline = 
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Morbi interdu"
        "m enim ex, eget hendrerit neque fringilla at. Aenean dapibus leo et li"
        "gula sodales tincidunt. Nam sit amet mi vulputate, suscipit mi laoreet"
        ", tincidunt tortor. Pellentesque euismod amet.";

    DBConn::headline testData;
    ASSERT_NO_THROW({ testData = testConn.getHeadlines(); });
    ASSERT_EQ(testData.size(), 2);
    tuple<int, string, string> data = testData[0];
    
    ASSERT_EQ(get<0>(testData[0]), 1);
    ASSERT_EQ(get<1>(testData[0]), string("Test 1"));
    ASSERT_EQ(get<2>(testData[0]), leadline);

    ASSERT_EQ(get<0>(testData[1]), 2);
    ASSERT_EQ(get<1>(testData[1]), string("Test 2"));
    ASSERT_EQ(get<2>(testData[1]), string("Start of 1st paragraph"));
}

TEST_F(DBConnTest, getArticle)
{
    static const string content =
        "# Test 1\nLorem ipsum dolor sit amet, consectetur adipi"
        "scing elit. Nulla auctor neque eget lobortis mollis. Morbi tempus eu fe"
        "lis eu auctor. Vestibulum ante ipsum primis in faucibus orci luctus et "
        "ultrices posuere cubilia Curae; Cras tristique tincidunt arcu, eget dic"
        "tum sapien interdum eget. Donec iaculis dapibus magna, nec vulputate ip"
        "sum molestie quis. Proin egestas dui non ante scelerisque feugiat. Vest"
        "ibulum tempor, turpis vitae porttitor condimentum, sapien quam rutrum e"
        "rat, ut auctor dolor mi ac erat. Nullam aliquet ante risus, sit amet co"
        "nvallis sapien ullamcorper vitae. Ut aliquet id tortor sed suscipit. Pe"
        "llentesque rutrum leo a neque congue, id lacinia libero finibus. Fusce "
        "eleifend venenatis vulputate. Vestibulum vitae mauris a ex pretium posu"
        "ere id ac dui. Quisque neque dolor, gravida vel neque non, consequat im"
        "perdiet nunc. Fusce finibus, enim sed rutrum interdum, felis lorem tris"
        "tique dolor, quis pulvinar orci libero ut nisl. In hac habitasse platea"
        " dictumst. Nullam tempus vestibulum nisi eget cras amet.";

    string testData;
    ASSERT_NO_THROW({
        auto testData = testConn.getArticle("1");
        ASSERT_EQ(testData, content);
    });
}

TEST_F(DBConnTest, getUserInfo)
{
    const string login = "a@a.com";
    DBConn::UserRecord testData;
    ASSERT_NO_THROW({ testData = testConn.getUserInfo(login); });
    ASSERT_TRUE(testData);
    auto data = *testData;

    ASSERT_EQ(get<0>(data), 1);
    ASSERT_STREQ(get<1>(data).c_str(), "Test user");
    ASSERT_EQ(get<2>(data), login);
    ASSERT_STREQ(get<3>(data).c_str(), "VEOCBE1i2wM2tsrGwmLfsg8d74fv7M-AxsngFVcv2ow");
    ASSERT_STREQ(get<4>(data).c_str(), "kt56uQBSTP-bT4ybmGCgsmU48BBx__mcE61X7UsWxpE");

    ASSERT_NO_THROW({ testData = testConn.getUserInfo("asdf@example.com"); });
    ASSERT_FALSE(testData);

    ASSERT_NO_THROW({ testData = testConn.getUserInfo("off@example.com"); });
    ASSERT_FALSE(testData);
}

TEST_F(DBConnTest, saveSession)
{
    ASSERT_NO_THROW({ testConn.saveSession(testUUID); });

    // Test update last_seen
    ASSERT_NO_THROW({ testConn.saveSession(testUUID); });
}

TEST_F(DBConnTest, mapUuidToUser)
{
    ASSERT_NO_THROW({ testConn.mapUuidToUser(testUUID, testUserId); });

    // Test mapping already present
    ASSERT_NO_THROW({ testConn.mapUuidToUser(testUUID, testUserId); });
}

TEST_F(DBConnTest, getMappedUser)
{
    ASSERT_NO_THROW({
        auto session = testConn.getSessionInfo(testUUID);
        ASSERT_TRUE(session);
        ASSERT_EQ(get<1>(*session), testUserId);
    });

    ASSERT_NO_THROW({
        ASSERT_FALSE(testConn.getSessionInfo("11111111-1111-1111-1111-111111111111"));
    });
}

} //namespace mimeographer
