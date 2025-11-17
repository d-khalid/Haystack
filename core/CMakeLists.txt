add_library(haystack_core
        src/lexicon.cpp
)

# Include directories
target_include_directories(haystack_core
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE # Only available within the library
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
