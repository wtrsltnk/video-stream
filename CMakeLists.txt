cmake_minimum_required(VERSION 3.8)

project(video-stream)

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
    )

target_link_libraries(${PROJECT_NAME}
    ws2_32
    )
