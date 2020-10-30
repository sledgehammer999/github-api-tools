add_executable(mass_close_old_issues)

target_sources(mass_close_old_issues PRIVATE
    # headers
    issuegatherer.h
    issueupdater.h
    labelcreator.h
    labelgatherer.h
    postdownloader.h
    programoptions.h

    # sources
    issuegatherer.cpp
    issueupdater.cpp
    labelcreator.cpp
    labelgatherer.cpp
    main.cpp
    postdownloader.cpp
    programoptions.cpp
)

target_compile_features(mass_close_old_issues PRIVATE
    cxx_std_17
)

target_compile_definitions(mass_close_old_issues PRIVATE
    BOOST_BEAST_USE_STD_STRING_VIEW
)

target_compile_options(mass_close_old_issues PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -pthread
)

# for <nlohmann/json> and <HowardHinnant/date.h>
target_include_directories(mass_close_old_issues PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(mass_close_old_issues PRIVATE
    Boost::headers
    Boost::program_options
    OpenSSL::Crypto
    OpenSSL::SSL
    Threads::Threads
    $<$<BOOL:${MINGW}>:ws2_32>
)

target_link_options(mass_close_old_issues PRIVATE
    -pthread
    $<$<BOOL:${MINGW}>:-static>
)

set_target_properties(mass_close_old_issues PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set_target_properties(mass_close_old_issues PROPERTIES OUTPUT_NAME mcoi)
