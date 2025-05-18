cmake_minimum_required(VERSION 3.15)
project(TcpServerApp)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 包含系统头文件
include_directories(
    ${PROJECT_SOURCE_DIR}
    ${OPENSSL_INCLUDE_DIR}
)

# 查找 OpenSSL（通过 vcpkg 提供）
find_package(OpenSSL REQUIRED)
if(NOT OPENSSL_FOUND)
    message(FATAL_ERROR "OpenSSL not found via vcpkg")
endif()

# 添加源文件（根据你的实际路径调整）
set(SOURCES
    main.cpp          # 包含 main() 的文件
    TcpServer.cpp     # TcpServer 类实现
    Segment.cpp       # DataSegment 和 HeadSegment 实现
    udp_client.cpp
    udp_server.cpp
    online_manager.cpp
)

# 创建可执行文件 - 直接指定源文件而不使用变量
add_executable(TcpServerApp 
    main.cpp
    TcpServer.cpp
    Segment.cpp
    udp_client.cpp
    udp_server.cpp
    online_manager.cpp
)

# 包含头文件路径
include_directories(
    ${PROJECT_SOURCE_DIR}
    ${OPENSSL_INCLUDE_DIR}
)

# 链接库
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        OpenSSL::Crypto  # OpenSSL 加密库
        ws2_32           # Windows Socket API
        iphlpapi         # 网络相关功能
)

# MSVC 编译器设置
if(MSVC)
    add_compile_options(/W4 /WX)                # 启用 Warnings as Errors
    add_compile_options(/DUNICODE /D_UNICODE)    # Unicode 支持
    add_compile_options(/DWIN32_LEAN_AND_MEAN)   # 快速编译 Windows 头文件
endif()