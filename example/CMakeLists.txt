add_executable(example
    example.cpp
)

target_link_libraries(example PRIVATE
    encoding_util
)
