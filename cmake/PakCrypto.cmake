set(_TENSHI_PAK_CRYPTO_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")

option(TENSHI_PAK_ENCRYPTION
    "Encrypt packed resources with XChaCha20-Poly1305"
    OFF
)

function(tenshi_configure_pak_crypto)
    if(TARGET tenshi_pak_crypto)
        return()
    endif()

    set(_crypto_sources
        "${_TENSHI_PAK_CRYPTO_ROOT}/src/Files/PakCrypto.cpp"
    )

    if(TENSHI_PAK_ENCRYPTION)
        if(TENSHI_PAK_KEY_SOURCE_DIR)
            set(_generated_dir "${TENSHI_PAK_KEY_SOURCE_DIR}")
        else()
            set(_generated_dir
                "${CMAKE_CURRENT_BINARY_DIR}/generated/pak-key"
            )
        endif()
        file(MAKE_DIRECTORY "${_generated_dir}")
        set(_all_key_parts_exist TRUE)
        foreach(_part RANGE 0 3)
            if(NOT EXISTS "${_generated_dir}/PakKeyPart${_part}.cpp")
                set(_all_key_parts_exist FALSE)
            endif()
        endforeach()

        if(TENSHI_PAK_KEY_SOURCE_DIR AND NOT _all_key_parts_exist)
            message(FATAL_ERROR
                "Missing generated key fragments in TENSHI_PAK_KEY_SOURCE_DIR."
            )
        elseif(NOT _all_key_parts_exist)
            find_package(Python3 REQUIRED COMPONENTS Interpreter)
            execute_process(
                COMMAND
                    "${Python3_EXECUTABLE}"
                    -c
                    "import secrets; print(secrets.token_hex(32))"
                RESULT_VARIABLE _key_generation_result
                OUTPUT_VARIABLE _key
                ERROR_VARIABLE _key_generation_error
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            if(NOT _key_generation_result EQUAL 0)
                message(FATAL_ERROR
                    "Failed to generate the packed-resource encryption key: "
                    "${_key_generation_error}"
                )
            endif()

            set(_hex_digits 0 1 2 3 4 5 6 7 8 9 A B C D E F)
            foreach(_part RANGE 0 3)
                set(_encoded_bytes "")
                foreach(_slot RANGE 0 7)
                    math(EXPR _byte_index "${_part} * 8 + ${_slot}")
                    math(EXPR _hex_offset "${_byte_index} * 2")
                    string(SUBSTRING "${_key}" "${_hex_offset}" 2 _key_byte_hex)
                    math(EXPR _key_byte "0x${_key_byte_hex}")
                    math(EXPR _mask "((${_byte_index} * 73 + 167) & 255)")
                    math(EXPR _encoded_byte "(${_key_byte} ^ ${_mask})")
                    math(EXPR _high_nibble "${_encoded_byte} / 16")
                    math(EXPR _low_nibble "${_encoded_byte} % 16")
                    list(GET _hex_digits "${_high_nibble}" _high_hex)
                    list(GET _hex_digits "${_low_nibble}" _low_hex)
                    string(APPEND _encoded_bytes "0x${_high_hex}${_low_hex}, ")
                endforeach()

                set(_key_part_file "${_generated_dir}/PakKeyPart${_part}.cpp")
                file(WRITE "${_key_part_file}"
                    "#include <array>\n"
                    "#include <cstdint>\n"
                    "#if defined(_MSC_VER)\n"
                    "#define TENSHI_PAK_NOINLINE __declspec(noinline)\n"
                    "#else\n"
                    "#define TENSHI_PAK_NOINLINE __attribute__((noinline))\n"
                    "#endif\n"
                    "namespace Tenshi::detail {\n"
                    "TENSHI_PAK_NOINLINE std::array<std::uint8_t, 8> pakKeyPart${_part}() {\n"
                    "    return { ${_encoded_bytes} };\n"
                    "}\n"
                    "}\n"
                )
            endforeach()
        endif()

        set(SODIUM_DISABLE_TESTS ON CACHE BOOL "" FORCE)
        if(NOT EXISTS "${_TENSHI_PAK_CRYPTO_ROOT}/external/libsodium-cmake/CMakeLists.txt"
        OR NOT EXISTS "${_TENSHI_PAK_CRYPTO_ROOT}/external/libsodium-cmake/libsodium/src/libsodium/include/sodium.h")
            message(FATAL_ERROR
                "Encryption requires the libsodium-cmake submodule and its nested "
                "libsodium submodule. Run: git submodule update --init --recursive"
            )
        endif()
        add_subdirectory(
            "${_TENSHI_PAK_CRYPTO_ROOT}/external/libsodium-cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/libsodium"
        )

        foreach(_part RANGE 0 3)
            list(APPEND _crypto_sources
                "${_generated_dir}/PakKeyPart${_part}.cpp"
            )
        endforeach()
        set(TENSHI_PAK_KEY_SOURCE_DIR "${_generated_dir}" PARENT_SCOPE)
    endif()

    add_library(tenshi_pak_crypto STATIC ${_crypto_sources})
    target_compile_features(tenshi_pak_crypto PUBLIC cxx_std_20)
    target_include_directories(tenshi_pak_crypto
        PRIVATE "${_TENSHI_PAK_CRYPTO_ROOT}/src"
    )

    if(TENSHI_PAK_ENCRYPTION)
        target_compile_definitions(tenshi_pak_crypto
            PRIVATE TENSHI_PAK_ENCRYPTION_ENABLED
        )
        target_link_libraries(tenshi_pak_crypto PRIVATE sodium)
    endif()
endfunction()
