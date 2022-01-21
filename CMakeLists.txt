cmake_minimum_required(VERSION 3.8)

project(video-stream)

include(cmake/CPM.cmake)
CPMAddPackage("gh:fmtlib/fmt#7.1.3")
CPMAddPackage("gh:nlohmann/json@3.10.5")

add_executable(${PROJECT_NAME}
    main.cpp
    httpserver.cpp
    httpserver.h
    httprequest.cpp
    httprequest.h
    httpresponse.cpp
    httpresponse.h
    httprouter.cpp
    httprouter.h
    htdocs/index.html
    htdocs/styles.css
    htdocs/scripts.js
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
