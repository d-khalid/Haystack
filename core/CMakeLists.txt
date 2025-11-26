# NOTE: Most third-party libraries are to be added here

add_library(haystack_core
        src/lexicon.cpp
        src/isam_storage.cpp
        src/compound_key.cpp
        src/utils.cpp
)

# Include directories
target_include_directories(haystack_core
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE # Only available within the library
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link targets (third-party libs)
target_link_libraries(haystack_core PUBLIC pugixml)
target_link_libraries(haystack_core PUBLIC nlohmann_json::nlohmann_json)
target_link_libraries(haystack_core PUBLIC GTest::gtest)


