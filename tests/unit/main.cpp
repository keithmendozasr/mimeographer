#include <glog/logging.h>
#include <gflags/gflags.h>
#include "gtest/gtest.h"

DEFINE_string(dbHost, "localhost", "DB server host");
DEFINE_string(dbUser, "", "DB login");
DEFINE_string(dbPass, "", "DB password");
DEFINE_string(dbName, "mimeographer", "Database name");
DEFINE_int32(dbPort, 5432, "DB server port");

int main(int argc, char **argv)
{
    google::InitGoogleLogging(argv[0]);
    ::testing::InitGoogleTest(&argc, argv);
    google::ParseCommandLineFlags(&argc, &argv, true);

    return RUN_ALL_TESTS();
}
