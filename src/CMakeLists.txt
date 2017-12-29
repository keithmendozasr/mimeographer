find_package(gflags REQUIRED)
add_executable (mimeographer main.cpp HandlerBase.cpp PrimaryHandler.cpp
    DBConn.cpp )
target_link_libraries(mimeographer folly proxygenlib proxygenhttpserver gflags 
    pthread glog pq uuid)
