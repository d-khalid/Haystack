# Link executable to the core library
add_executable(haystack_cli main.cpp)
target_link_libraries(haystack_cli PRIVATE haystack_core)