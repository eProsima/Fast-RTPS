add_subdirectory(mutex_testing_tool)

set(USER_THREAD_NONBLOCKED_TEST UserThreadNonBlockedTest.cpp)

add_executable(user_thread_nonblocked_test ${USER_THREAD_NONBLOCKED_TEST})
target_compile_definitions(DDSSimpleCommunicationSubscriber PRIVATE
    $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">>:__DEBUG>
    $<$<BOOL:${INTERNAL_DEBUG}>:__INTERNALDEBUG> # Internal debug activated.
    )
target_link_libraries(user_thread_nonblocked_test mutex_testing_tool fastrtps fastcdr GTest::gtest)

STRING(REPLACE " " "\\ " MUTEX_PRELOAD_LIBRARY_FILE "$<TARGET_FILE:mutex_testing_tool_preload>")
add_gtest(NAME UserThreadNonBlockedTest COMMAND user_thread_nonblocked_test SOURCES ${USER_THREAD_NONBLOCKED_TEST}
    ENVIRONMENTS
    "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:mutex_testing_tool_preload>"
    "LD_PRELOAD=$<TARGET_FILE_NAME:mutex_testing_tool_preload>"
    LABELS "NoMemoryCheck"
    )
