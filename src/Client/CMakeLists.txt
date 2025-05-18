cmake_minimum_required(VERSION 3.15)
project(TcpClientApp)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 包含系统头文件
include_directories(
    ${PROJECT_SOURCE_DIR}
    ${OPENSSL_INCLUDE_DIR}
)
# 查找 OpenSSL
find_package(OpenSSL REQUIRED)
if(NOT OPENSSL_FOUND)
    message(FATAL_ERROR "OpenSSL not found")
endif()

# 添加源文件（需要你补充实际的 .cpp 文件）
set(SOURCES
    main.cpp
    TcpClient.cpp  # 需要你实现
    Segment.cpp    # 需要你实现
    ProgressBar.cpp
    udp_client.cpp
    udp_server.cpp
    online_manager.cpp
)

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES})

# 链接库
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        OpenSSL::Crypto
        ws2_32
        iphlpapi
)

# MSVC 特定设置
if(MSVC)
    add_compile_options(/W4 /WX)
    add_compile_options(/DUNICODE /D_UNICODE)
    add_compile_options(/DWIN32_LEAN_AND_MEAN)
endif()