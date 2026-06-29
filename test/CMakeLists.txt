enable_testing(true)
include_directories("../src")

add_executable(testdownloadhandlerimpl testdownloadhandlerimpl.cpp)
target_link_libraries(testdownloadhandlerimpl PRIVATE qbt_base)
add_test(NAME testdownloadhandlerimpl COMMAND testdownloadhandlerimpl)

add_executable(testcmdoptions testcmdoptions.cpp ../src/app/cmdoptions.cpp)
target_compile_definitions(testcmdoptions PRIVATE DISABLE_GUI)
target_link_libraries(testcmdoptions PRIVATE qbt_base)
add_test(NAME testcmdoptions COMMAND testcmdoptions)

add_executable(testwebapplicationsecurity testwebapplicationsecurity.cpp)
target_link_libraries(testwebapplicationsecurity PRIVATE qbt_webui qbt_base)
add_test(NAME testwebapplicationsecurity COMMAND testwebapplicationsecurity)

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure)
add_dependencies(check testdownloadhandlerimpl)
add_dependencies(check testcmdoptions)
add_dependencies(check testwebapplicationsecurity)

find_package(Qt6 QUIET COMPONENTS Test)
if (NOT TARGET Qt6::Test)
    message(WARNING "Qt6 Test component is not available; only standalone C++ checks will run.")
    return()
endif()

set(testFiles
    testalgorithm.cpp
    testbittorrenttrackerentry.cpp
    testconceptsexplicitlyconvertibleto.cpp
    testconceptsstringable.cpp
    testglobal.cpp
    testorderedset.cpp
    testpath.cpp
    testutilsbytearray.cpp
    testutilscompare.cpp
    testutilsdatetime.cpp
    testutilsgzip.cpp
    testutilsio.cpp
    testutilsnumber.cpp
    testutilsstring.cpp
    testutilsversion.cpp
)

foreach(testFile ${testFiles})
    get_filename_component(testFilename "${testFile}" NAME_WLE)

    add_executable("${testFilename}" "${testFile}")
    target_link_libraries("${testFilename}" PRIVATE Qt::Test qbt_base)
    add_test(NAME "${testFilename}" COMMAND "${testFilename}")

    add_dependencies(check "${testFilename}")
endforeach()
