# 收集测试程序的源文件（仅测试相关代码，不包含库的核心代码）
aux_source_directory(. TEST_SRC)  # 如test/main.cpp

# 生成测试可执行文件（NetSocketTest）
add_executable(TinyHttpTest ${TEST_SRC})

# 链接到前面生成的动态库（关键！让可执行文件使用库的功能）
target_link_libraries(TinyHttpTest netsocket tinyhttp pthread)