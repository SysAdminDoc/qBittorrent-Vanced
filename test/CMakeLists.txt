enable_testing(true)
find_package(Qt6 QUIET COMPONENTS Test)

if (NOT TARGET Qt6::Test)
    message(WARNING "Qt6 Test component is not available; the check target will fail with setup guidance.")
    add_custom_target(check
        COMMAND ${CMAKE_COMMAND} -E echo "Qt6 Test component is not available. Install qtbase with the testlib feature before running C++ tests."
        COMMAND ${CMAKE_COMMAND} -E false
    )
    return()
endif()

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure)

include_directories("../src")

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
