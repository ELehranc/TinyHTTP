# 收集库的源文件
aux_source_directory(. SRC_LIST)
aux_source_directory(./http HTTP_LIST)
aux_source_directory(./mysqlDb DB_LIST)

# 生成动态库（将所有源文件编译到库中）
add_library(tinyhttp SHARED  ${SRC_LIST} ${HTTP_LIST} ${DB_LIST})

target_link_libraries(
    tinyhttp 
    netsocket 
    ${OPENSSL_SSL_LIBRARY} 
    ${OPENSSL_CRYPTO_LIBRARY}
    ${MYSQL_LIBRARY}  
    pthread
)