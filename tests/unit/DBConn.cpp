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

} //namespace mimeographer
