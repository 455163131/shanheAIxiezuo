# 单元测试目标：shanhe_tests
# 复用 app 的源文件（sseparser / bridge / httpllmclient / mockllmclient），
# 不链接 ShanHeWriter 可执行文件，避免 main() 符号冲突。

qt_add_executable(shanhe_tests
    test_shanhe.cpp
    fakellmclient.h
    ../src/sseparser.cpp
    ../src/bridge.cpp
    ../src/httpllmclient.cpp
    ../src/mockllmclient.cpp
    ../src/projectstore.cpp
    ../src/personas.cpp
    ../src/windowscredentialstore.cpp
)

# 让测试能 #include "bridge.h" / "sseparser.h" / "illmclient.h" 等（均在 src/）
target_include_directories(shanhe_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(shanhe_tests PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Test
)

add_test(NAME shanhe_tests COMMAND shanhe_tests)
