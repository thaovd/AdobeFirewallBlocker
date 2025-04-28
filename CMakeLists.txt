cmake_minimum_required(VERSION 3.14)
project(AdobeFirewallBlocker)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows API dependencies
if(WIN32)
    set(PLATFORM_LIBS
        comctl32
        gdi32
        user32
        shell32
        advapi32
        ole32
        oleaut32
        uuid
    )
endif()

set(SOURCE_FILES
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/software_scanner.cpp
    src/software_scanner.h
    src/firewall_manager.cpp
    src/firewall_manager.h
    src/resources.rc
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_compile_definitions(${PROJECT_NAME} PRIVATE UNICODE _UNICODE)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PLATFORM_LIBS})

# Add Windows manifest for proper UI styling
set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE TRUE
    LINK_FLAGS "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\""
)
