cmake_minimum_required(VERSION 3.20)

project(tenshi_packer LANGUAGES C CXX)

set(EXTERNAL_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/../../external"
)

set(liblha_BUILD_TESTS OFF CACHE BOOL "" FORCE)

include("${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/PakCrypto.cmake")
tenshi_configure_pak_crypto()

##################
#     liblha     #
##################

add_subdirectory(
    "${EXTERNAL_DIR}/liblha"
    "${CMAKE_CURRENT_BINARY_DIR}/liblha"
)

##################
#     Packer     #
##################

add_executable(packer_tool
    main.cpp
)

target_link_libraries(packer_tool
    PRIVATE
        liblha::liblha
        tenshi_pak_crypto
)
target_include_directories(packer_tool
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/../../src"
)

install(
    TARGETS packer_tool
    RUNTIME DESTINATION bin
)