cmake_minimum_required(VERSION 3.0)
project(tinyhttp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Wextra")

# 配置输出路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 配置头文件搜索路径
include_directories(${PROJECT_SOURCE_DIR}/include/http)
include_directories(${PROJECT_SOURCE_DIR}/include/mysqlDb)

# 查找OpenSSL库
# 直接指定 OpenSSL 路径和库文件
set(OPENSSL_INCLUDE_DIR /usr/include/openssl11)
set(OPENSSL_SSL_LIBRARY /usr/lib64/openssl11/libssl.so)
set(OPENSSL_CRYPTO_LIBRARY /usr/lib64/openssl11/libcrypto.so)
# 包含OpenSSL头文件
include_directories(${OPENSSL_INCLUDE_DIR})


# 配置MySQL Connector/C++
set(MYSQL_INCLUDE_DIR /usr/include/mysql-cppconn/jdbc)  # 头文件路径（保持不变）
set(MYSQL_LIB_DIR /usr/lib64)                           # 库文件路径
set(MYSQL_LIBRARY mysqlcppconn)                       # 库名
# 包含Mysql头文件
include_directories(${MYSQL_INCLUDE_DIR})

# 加载子目录
add_subdirectory(src)
add_subdirectory(test)

# 安装规则
install(TARGETS tinyhttp  
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"  
)