cmake_minimum_required(VERSION 3.20)

project(CourtCraftBackend LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# C data-structure modules (linked list, queue, priority queue, stack, validation)
set(C_MODULE_SOURCES
    src/c_modules/case_validation.c
    src/c_modules/case_list.c
    src/c_modules/hearing_queue.c
    src/c_modules/priority_queue.c
    src/c_modules/history_stack.c
)

# C++ object-oriented domain model (User/Role, Case, Hearing, Report)
set(OOP_SOURCES
    src/oop/domain.cpp
)

# Set vcpkg toolchain - adjust path as needed for your installation
set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../vcpkg/installed/x64-mingw-dynamic")

find_package(Crow CONFIG REQUIRED)

# MySQL Connector/C client
set(MYSQL_INCLUDE_DIR "C:/Program Files/MySQL/MySQL Server 8.0/include")
set(MYSQL_LIBRARY "C:/Program Files/MySQL/MySQL Server 8.0/lib/libmysql.lib")
set(MYSQL_DLL "C:/Program Files/MySQL/MySQL Server 8.0/lib/libmysql.dll")

add_executable(courtcraft
    src/main.cpp
    ${C_MODULE_SOURCES}
    ${OOP_SOURCES}
)

# Stand-alone test program for the C modules (no database needed)
add_executable(test_c_modules
    src/tests/test_c_modules.c
    ${C_MODULE_SOURCES}
)

# Stand-alone test program for the C++ OOP classes (no database needed)
add_executable(test_oop
    src/tests/test_oop.cpp
    ${OOP_SOURCES}
    src/c_modules/case_validation.c
)
target_link_libraries(test_oop PRIVATE Crow::Crow ws2_32 mswsock)

target_include_directories(courtcraft
    PRIVATE
    "${MYSQL_INCLUDE_DIR}"
)

target_link_libraries(courtcraft
    PRIVATE
    Crow::Crow
    ws2_32
    mswsock
    "${MYSQL_LIBRARY}"
)

# Copy MySQL DLL next to the executable
add_custom_command(TARGET courtcraft POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${MYSQL_DLL}"
    "$<TARGET_FILE_DIR:courtcraft>"
    
    COMMENT "Copying MySQL runtime DLL to build directory"
)