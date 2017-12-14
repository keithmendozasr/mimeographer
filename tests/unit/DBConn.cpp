#include <string>

#include "DBConn.h"
#include "gtest/gtest.h"

using namespace std;

namespace mimeographer
{
class DBConnTest : public ::testing::Test {};

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

TEST_F(DBConnTest, getArticleHeadlines)
{
    static const string leadline = 
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nulla auctor "
        "neque eget lobortis mollis. Morbi tempus eu felis eu auctor. Vestibulu"
        "m ante ipsum primis in faucibus orci luctus et ultrices posuere cubili"
        "a Curae; Cras tristique tincidunt arcu, eget";

    DBConn conn("testuser", "123456", "localhost", "mimeographer");
    auto testData = conn.getHeadlines();
    ASSERT_EQ(testData.size(), 10);
    ASSERT_EQ(testData[0][0], string("1"));
    ASSERT_EQ(testData[0][1], string("Test 1"));
    ASSERT_EQ(testData[0][2], leadline);

    ASSERT_EQ(testData[4][0], string("5"));
    ASSERT_EQ(testData[4][1], string("Test 5"));
    ASSERT_EQ(testData[4][2], leadline);
}

} //namespace mimeographer
