# Third-party libraries are to be added here

# pugixml, for XML parsing
add_library(pugixml STATIC
            pugixml/pugixml.cpp)

target_include_directories(pugixml
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/pugixml)