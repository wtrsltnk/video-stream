cmake_minimum_required(VERSION 3.8)

project(video-stream)

include(cmake/CPM.cmake)
CPMAddPackage("gh:nlohmann/json@3.10.5")

add_executable(${PROJECT_NAME}
    main.cpp
    httpserver.cpp
    httpserver.h
    httprequest.cpp
    httprequest.h
    httpresponse.cpp
    httpresponse.h
    index.html
    styles.css
    scripts.js
)

target_compile_features(${PROJECT_NAME}
    PRIVATE cxx_auto_type
    PRIVATE cxx_nullptr
    PRIVATE cxx_lambdas
    PRIVATE cxx_std_17
)

target_link_libraries(${PROJECT_NAME}
    ws2_32
    nlohmann_json::nlohmann_json
)
