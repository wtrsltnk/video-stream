cmake_minimum_required(VERSION 3.8)

project(video-stream)

include(cmake/CPM.cmake)
CPMAddPackage("gh:fmtlib/fmt#7.1.3")
CPMAddPackage("gh:nlohmann/json@3.10.5")

add_executable(${PROJECT_NAME}
    htdocs/index.html
    htdocs/scripts.js
    htdocs/styles.css
    httprequest.cpp
    httprequest.h
    httpresponse.cpp
    httpresponse.h
    httprouter.cpp
    httprouter.h
    httpserver.cpp
    httpserver.h
    main.cpp
    README.md
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
