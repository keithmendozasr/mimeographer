include_directories(../../include ../../googletest-src/googletest/include/)
add_executable(unit_test main.cpp DBConn.cpp ../../src/DBConn.cpp)
target_link_libraries(unit_test gtest glog)
add_test(NAME mimeographer_unit_test COMMAND unit_test)
