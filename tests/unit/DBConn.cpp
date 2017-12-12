#include <string>

#include "DBConn.h"
#include "gtest/gtest.h"

using namespace std;

namespace mimeographer
{
class DBConnTest : public ::testing::Test {};

TEST_F(DBConnTest, urlEncode)
{
    DBConn conn("user", "pass", "db", 9876);
    EXPECT_EQ(conn.urlEncode(" "), string("%20"));
    EXPECT_EQ(conn.urlEncode("Hello"), string("Hello"));
    EXPECT_EQ(conn.urlEncode("Hello there"), string("Hello%20there"));
    EXPECT_EQ(conn.urlEncode("/blah %"), string("%2Fblah%20%25"));
}

} //namespace mimeographer
