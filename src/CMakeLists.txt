find_package(gflags REQUIRED)
include_directories ( ../include )
add_executable (mimeographer main.cpp BlogHandler.cpp)
target_link_libraries(mimeographer folly proxygenlib proxygenhttpserver gflags pthread glog)
