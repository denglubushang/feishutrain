# 设置最低 CMake 版本要求
cmake_minimum_required(VERSION 3.14)

# 定义项目名称和语言
project(Server CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 收集源文件（假设所有源文件在当前目录）
file(GLOB SRC_FILES "*.cpp")

# 添加可执行目标
add_executable(${PROJECT_NAME} ${SRC_FILES})

# Windows 平台配置
if(WIN32)
    # 链接 Windows 系统库
    target_link_libraries(${PROJECT_NAME} PRIVATE
        Ws2_32.lib
        iphlpapi.lib
    )
endif()

# OpenSSL 配置
find_package(OpenSSL REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
)

# 可选：确保 OpenSSL 头文件路径正确
target_include_directories(${PROJECT_NAME} PRIVATE
    ${OPENSSL_INCLUDE_DIRS}
)

# 定义资源文件路径
set(KEY_FILE ${PROJECT_SOURCE_DIR}/key.txt)
set(PASSWORD_FILE ${PROJECT_SOURCE_DIR}/password.txt)

# 判断是否为多配置生成器（如 Visual Studio）
if(CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE)
    foreach(config ${CMAKE_CONFIGURATION_TYPES} ${CMAKE_BUILD_TYPE})
        # 复制 key.txt
        configure_file(
            ${KEY_FILE}
            ${CMAKE_CURRENT_BINARY_DIR}/${config}/key.txt
            COPYONLY
        )
        # 复制 password.txt
        configure_file(
            ${PASSWORD_FILE}
            ${CMAKE_CURRENT_BINARY_DIR}/${config}/password.txt
            COPYONLY
        )
    endforeach()
else()
    # 单配置构建（如 Makefile）：复制到构建目录根目录
    configure_file(
        ${KEY_FILE}
        ${CMAKE_CURRENT_BINARY_DIR}/key.txt
        COPYONLY
    )
    configure_file(
        ${PASSWORD_FILE}
        ${CMAKE_CURRENT_BINARY_DIR}/password.txt
        COPYONLY
    )
endif()