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
set(dbuser "")
set(dbpass "")
set(dbname "mimeographer_unit_test")
add_test(DBConn unit_test --dbUser=${dbuser} --dbPass=${dbpass}
    --dbName=${dbname} --staticBase=../../staticfiles --gtest_filter=DBConnTest.*)
add_test(HandlerBase unit_test --dbUser=${dbuser} --dbPass=${dbpass}
    --dbName=${dbname} --staticBase=../../staticfiles --gtest_filter=HandlerBaseTest.*)
add_test(UserSession unit_test --dbUser=${dbuser} --dbPass=${dbpass}
    --dbName=${dbname} --staticBase=../../staticfiles --gtest_filter=UserSessionTest.*)
add_test(SummaryBuilder unit_test --dbUser=${dbuser} --dbPass=${dbpass}
    --dbName=${dbname} --staticBase=../../staticfiles --gtest_filter=SummaryBuilderTest.*)
add_test(StaticHandler unit_test --dbUser=${dbuser} --dbPass=${dbpass}
    --dbName=${dbname} --staticBase=../../staticfiles --gtest_filter=StaticHandlerTest.*)
add_test(PrimaryHandler unit_test --dbUser=${dbuser} --dbPass=${dbpass}
    --dbName=${dbname} --staticBase=../../staticfiles --gtest_filter=PrimaryHandlerTest.*)
add_test(UserHandler unit_test --dbUser=${dbuser} --dbPass=${dbpass}
    --dbName=${dbname} --staticBase=../../staticfiles --gtest_filter=UserHandlerTest.*)
add_test(SiteTemplate unit_test --dbUser=${dbuser} --dbPass=${dbpass}
    --dbName=${dbname} --staticBase=../../staticfiles --gtest_filter=SiteTemplateTest.*)
