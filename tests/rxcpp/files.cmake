set(${PROJECT_NAME}_headers_include
)

set(${PROJECT_NAME}_headers
    ${${PROJECT_NAME}_headers_include}
)

#source_group("bluekyu" FILES ${${PROJECT_NAME}_headers_include})


set(${PROJECT_NAME}_sources_src
    "${PROJECT_SOURCE_DIR}/src/main.cpp"
    "${PROJECT_SOURCE_DIR}/src/main.hpp"
    "${PROJECT_SOURCE_DIR}/src/test_group_by.hpp"
    "${PROJECT_SOURCE_DIR}/src/test_nested_observable.hpp"
)

set(${PROJECT_NAME}_sources
    ${${PROJECT_NAME}_sources_src}
)

source_group("src" FILES ${${PROJECT_NAME}_sources_src})
