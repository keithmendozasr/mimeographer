include_directories(../../include ../../googletest-src/googletest/include/)
add_executable(unit_test main.cpp DBConn.cpp ../../src/DBConn.cpp
    HandlerBase.cpp ../../src/HandlerBase.cpp
    UserSession.cpp ../../src/UserSession.cpp
    SummaryBuilder.cpp ../../src/SummaryBuilder.cpp
    StaticHandler.cpp ../../src/StaticHandler.cpp
    PrimaryHandler.cpp ../../src/PrimaryHandler.cpp
    UserHandler.cpp ../../src/UserHandler.cpp
    SiteTemplates.cpp ../../src/SiteTemplates.cpp)
target_link_libraries(unit_test folly proxygenlib proxygenhttpserver gtest glog
    pq gflags uuid crypto cmark boost_filesystem boost_system)

message("Set DB user/password for testing")
add_test(unit_test unit_test --dbUser= --dbPass=
    --dbName=mimeographer_unit_test --staticBase=../../staticfiles)
