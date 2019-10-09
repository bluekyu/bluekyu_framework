set(${PROJECT_NAME}_headers_include
    "${PROJECT_SOURCE_DIR}/include/bluekyu/framework.hpp"
)

set(${PROJECT_NAME}_headers
    ${${PROJECT_NAME}_headers_include}
)

source_group("bluekyu" FILES ${${PROJECT_NAME}_headers_include})


set(${PROJECT_NAME}_sources_src
    "${PROJECT_SOURCE_DIR}/src/framework.cpp"
)

set(${PROJECT_NAME}_sources
    ${${PROJECT_NAME}_sources_src}
)

source_group("src" FILES ${${PROJECT_NAME}_sources_src})
