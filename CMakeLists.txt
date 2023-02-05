cmake_minimum_required(VERSION 3.8)

project(video-stream)

include(cmake/CPM.cmake)
CPMAddPackage("gh:fmtlib/fmt#7.1.3")
CPMAddPackage("gh:nlohmann/json@3.10.5")

add_executable(${PROJECT_NAME}
    README.md
    htdocs/index.html
    htdocs/scripts.js
    htdocs/styles.css
    src/httprequest.cpp
    src/httprequest.h
    src/httpresponse.cpp
    src/httpresponse.h
    src/httprouter.cpp
    src/httprouter.h
    src/httpserver.cpp
    src/httpserver.h
    src/main.cpp
)

target_compile_features(${PROJECT_NAME}
    PRIVATE cxx_auto_type
    PRIVATE cxx_nullptr
    PRIVATE cxx_lambdas
    PRIVATE cxx_std_17
)

target_link_libraries(${PROJECT_NAME}
    ws2_32
    fmt
    nlohmann_json::nlohmann_json
)
