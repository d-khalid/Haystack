# NOTE: Most third-party libraries are to be added here

add_library(haystack_core
        src/lexicon.cpp
        src/data_index.cpp
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

target_sources(core
    PRIVATE
        src/lexicon.cpp
        src/utils.cpp
        src/data_index.cpp
        src/forward_index.cpp  # <--- add this
)
target_include_directories(core
    PUBLIC include
)


