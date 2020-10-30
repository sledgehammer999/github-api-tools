add_executable(amend_title_and_apply_label)

target_sources(amend_title_and_apply_label PRIVATE
    # headers
    issueattributes.h
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

target_compile_features(amend_title_and_apply_label PRIVATE
    cxx_std_17
)

target_compile_definitions(amend_title_and_apply_label PRIVATE
    BOOST_BEAST_USE_STD_STRING_VIEW
)

target_compile_options(amend_title_and_apply_label PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -pthread
)

# for <nlohmann/json>
target_include_directories(amend_title_and_apply_label PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(amend_title_and_apply_label PRIVATE
    Boost::headers
    Boost::program_options
    OpenSSL::Crypto
    OpenSSL::SSL
    Threads::Threads
    $<$<BOOL:${MINGW}>:ws2_32>
)

target_link_options(amend_title_and_apply_label PRIVATE
    -pthread
    $<$<BOOL:${MINGW}>:-static>
)

set_target_properties(amend_title_and_apply_label PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set_target_properties(amend_title_and_apply_label PROPERTIES OUTPUT_NAME ataal)
