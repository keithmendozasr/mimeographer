#include <iostream>
#include <string>
#include <typeinfo>

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
    DBConn testConn = { "testuser", "123456", "localhost", "mimeographer" };
};

TEST_F(DBConnTest, urlEncode)
{
    DBConn conn;
    EXPECT_EQ(conn.urlEncode(" "), string("%20"));
    EXPECT_EQ(conn.urlEncode("Hello"), string("Hello"));
    EXPECT_EQ(conn.urlEncode("Hello there"), string("Hello%20there"));
    EXPECT_EQ(conn.urlEncode("/blah %"), string("%2Fblah%20%25"));
}

TEST_F(DBConnTest, splitString)
{   
    DBConn conn;
    auto rslt = conn.splitString("abcdefghij", 10, 2);
    ASSERT_EQ(rslt.size(), 5);
    ASSERT_STREQ(rslt[0].c_str(), "ab");
    ASSERT_STREQ(rslt[2].c_str(), "ef");
    ASSERT_STREQ(rslt[4].c_str(), "ij");
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

TEST_F(DBConnTest, getArticleHeadlines)
{
    static const string leadline = 
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nulla auctor "
        "neque eget lobortis mollis. Morbi tempus eu felis eu auctor. Vestibulu"
        "m ante ipsum primis in faucibus orci luctus et ultrices posuere cubili"
        "a Curae; Cras tristique tincidunt arcu, eget";

    auto testData = testConn.getHeadlines();
    ASSERT_EQ(testData.size(), 10);
    ASSERT_EQ(testData[0][(int)DBConn::headlinepart::id], string("1"));
    ASSERT_EQ(testData[0][(int)DBConn::headlinepart::title], string("Test 1"));
    ASSERT_EQ(testData[0][(int)DBConn::headlinepart::leadline], leadline);

    ASSERT_EQ(testData[4][(int)DBConn::headlinepart::id], string("5"));
    ASSERT_EQ(testData[4][(int)DBConn::headlinepart::title], string("Test 5"));
    ASSERT_EQ(testData[4][(int)DBConn::headlinepart::leadline], leadline);
}

TEST_F(DBConnTest, getArticle)
{
    static const string content = "Lorem ipsum dolor sit amet, consectetur adipi"
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

    auto testData = testConn.getArticle("1");
    ASSERT_STREQ(get<0>(testData).c_str(), "Test 1");

    auto testContent = get<1>(testData);
    ASSERT_EQ(testContent.size(), 1);
    ASSERT_EQ(testContent[0], content);
}

TEST_F(DBConnTest, getUserInfo)
{

    ASSERT_NO_THROW({
        auto testData = testConn.getUserInfo("a@a.com");
        auto data = *testData;
        ASSERT_STREQ(data[(int)DBConn::UserParts::id].c_str(), "1");
        ASSERT_STREQ(data[(int)DBConn::UserParts::fullname].c_str(), "Test User");
        ASSERT_STREQ(data[(int)DBConn::UserParts::email].c_str(), "a@a.com");
        ASSERT_STREQ(data[(int)DBConn::UserParts::salt].c_str(), "VEOCBE1i2wM2tsrGwmLfsg8d74fv7M-AxsngFVcv2ow");
        ASSERT_STREQ(data[(int)DBConn::UserParts::password].c_str(), "kt56uQBSTP-bT4ybmGCgsmU48BBx__mcE61X7UsWxpE");
    });

    ASSERT_NO_THROW({
        auto testData = testConn.getUserInfo("asdf@example.com");
        ASSERT_FALSE(testData);
    });

    ASSERT_NO_THROW({
        auto testData = testConn.getUserInfo("off@example.com");
        ASSERT_FALSE(testData);
    });
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
        ASSERT_EQ(*testConn.getMappedUser(testUUID), testUserId);
        ASSERT_FALSE(testConn.getMappedUser("11111111-1111-1111-1111-111111111111"));
    });
}

} //namespace mimeographer
