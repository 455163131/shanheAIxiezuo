# Unit test targets: shanhe_tests
# Reuses app source files (sseparser / bridge / httpllmclient / mockllmclient),
# does not link the ShanHeWriter executable (avoids main() symbol clash).

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
    # Stage 2 prompt-engine sources required by Task 13 state machine:
    ../src/textutils.cpp
    ../src/leakguard.cpp
    ../src/llmparams.cpp
    ../src/promptassembler.cpp
    ../src/configio.cpp
)

# Allow #include "bridge.h" / "sseparser.h" / "illmclient.h" etc. (all in src/)
target_include_directories(shanhe_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(shanhe_tests PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Test
)

add_test(NAME shanhe_tests COMMAND shanhe_tests)


# ---- TextUtils pure-function tests (Task 6) ----
qt_add_executable(test_textutils
    test_textutils.cpp
    ../src/textutils.cpp
)
target_include_directories(test_textutils PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)
target_link_libraries(test_textutils PRIVATE
    Qt6::Core
    Qt6::Test
)
add_test(NAME test_textutils COMMAND test_textutils)

# ---- LeakGuard three-tier prompt leak detection tests (Task 7) ----
qt_add_executable(test_leakguard
    test_leakguard.cpp
    ../src/leakguard.cpp
)
target_include_directories(test_leakguard PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)
target_link_libraries(test_leakguard PRIVATE
    Qt6::Core
    Qt6::Test
)
add_test(NAME test_leakguard COMMAND test_leakguard)

# ---- LlmParams constants + reasoner model detection tests (Task 8) ----
qt_add_executable(test_llmparams
    test_llmparams.cpp
    ../src/llmparams.cpp
)
target_include_directories(test_llmparams PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)
target_link_libraries(test_llmparams PRIVATE
    Qt6::Core
    Qt6::Test
)
add_test(NAME test_llmparams COMMAND test_llmparams)


# ---- PromptAssembler 6-segment assembly tests (Task 11) ----
qt_add_executable(test_promptassembler
    test_promptassembler.cpp
    ../src/promptassembler.cpp
    ../src/textutils.cpp
    ../src/leakguard.cpp
)
target_include_directories(test_promptassembler PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)
target_link_libraries(test_promptassembler PRIVATE
    Qt6::Core
    Qt6::Test
)
add_test(NAME test_promptassembler COMMAND test_promptassembler)
