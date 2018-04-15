find_package(gflags REQUIRED)
add_executable (mimeographer main.cpp HandlerBase.cpp PrimaryHandler.cpp
    DBConn.cpp EditHandler.cpp UserSession.cpp StaticHandler.cpp
    SummaryBuilder.cpp UserHandler.cpp SiteTemplates.cpp)
target_link_libraries(mimeographer folly proxygenlib proxygenhttpserver gflags 
    pthread glog pq uuid crypto cmark boost_filesystem boost_system ${JSONCPP_LIBRARIES})
