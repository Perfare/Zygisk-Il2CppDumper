cmake_minimum_required(VERSION 3.4.1)

set(CMAKE_CXX_STANDARD 14)

set(LIBRARY_NAME "whale")

enable_language(ASM)
SET(CMAKE_ASM_FLAGS "${CFLAGS}  -x assembler-with-cpp")

if (CMAKE_SYSTEM_NAME MATCHES "^Android")
    set(KERNEL "Linux")
    set(PLATFORM "Android")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -s -Wl,--gc-sections")
    add_definitions("-DWHALE_ANDROID_AUTO_LOAD")
    message("Building Whale for Android(${CMAKE_SYSTEM_PROCESSOR})...")
elseif (PLATFORM STREQUAL "IOS")
    set(KERNEL "Darwin")
    add_definitions("-DDARWIN")
    include_directories(/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk/usr/include)
    if (IOS_ARCH STREQUAL "arm64")
        set(CMAKE_SYSTEM_PROCESSOR "aarch64")
    elseif (IOS_ARCH MATCHES "arm")
        set(CMAKE_SYSTEM_PROCESSOR "arm")
    endif ()
    message("Building Whale for IOS(${CMAKE_SYSTEM_PROCESSOR})...")
else ()
    if (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
        set(KERNEL "Darwin")
        add_definitions("-DDARWIN")
    endif ()
    message("Building Whale for ${CMAKE_SYSTEM_NAME}(${CMAKE_SYSTEM_PROCESSOR})...")
endif ()


include_directories(src/libffi src/libffi/platform_include)

set(WHALE_SOURCES
        src/whale.cc
        src/interceptor.cc
        src/dbi/hook_common.cc
        src/platform/memory.cc
        src/assembler/assembler.cc
        src/assembler/memory_region.cc
        src/dbi/instruction_set.cc
        src/libffi/closures.c
        src/libffi/debug.c
        src/libffi/dlmalloc.c
        src/libffi/java_raw_api.c
        src/libffi/prep_cif.c
        src/libffi/raw_api.c
        src/libffi/types.c
        src/libffi/ffi_cxx.cc
        )

set(WHALE_LINUX_SOURCES
        src/platform/linux/elf_image.cc
        src/platform/linux/process_map.cc
        )


set(WHALE_DARWIN_SOURCES
        src/dbi/darwin/macho_import_hook.cc
        )

set(WHALE_ANDROID_ART
        src/android/art/native_on_load.cc
        src/android/art/art_runtime.cc
        src/android/art/art_symbol_resolver.cc
        src/android/art/java_types.cc
        src/android/art/well_known_classes.cc
        src/android/art/art_method.cc
        src/android/art/scoped_thread_state_change.cc
        src/android/art/art_jni_trampoline.cc
        )

set(WHALE_AARCH32
        src/dbi/arm/decoder_arm.cc
        src/dbi/arm/decoder_thumb.cc
        src/dbi/arm/inline_hook_arm.cc
        src/dbi/arm/instruction_rewriter_arm.cc
        src/libffi/arm/ffi_armv7.c
        src/libffi/arm/sysv_armv7.S
        )

set(WHALE_AARCH64
        src/dbi/arm64/decoder_arm64.cc
        src/dbi/arm64/instruction_rewriter_arm64.cc
        src/dbi/arm64/inline_hook_arm64.cc
        src/libffi/aarch64/ffi_arm64.c
        src/libffi/aarch64/sysv_arm64.S
        )

set(WHALE_DISASSEMBLER_X86_OR_X86_64
        src/dbi/x86/distorm/decoder.c
        src/dbi/x86/distorm/distorm.c
        src/dbi/x86/distorm/instructions.c
        src/dbi/x86/distorm/insts.c
        src/dbi/x86/distorm/mnemonics.c
        src/dbi/x86/distorm/operands.c
        src/dbi/x86/distorm/prefix.c
        src/dbi/x86/distorm/textdefs.c
        src/dbi/x86/distorm/wstring.c
        )

set(WHALE_X86
        src/assembler/x86/assembler_x86.cc
        src/assembler/x86/managed_register_x86.cc
        src/dbi/x86/inline_hook_x86.cc
        src/dbi/x86/intercept_syscall_x86.cc
        src/dbi/x86/instruction_rewriter_x86.cc
        src/libffi/x86/ffi_i386.c
        src/libffi/x86/sysv_i386.S
        )

set(WHALE_X86_64
        src/assembler/x86_64/assembler_x86_64.cc
        src/assembler/x86_64/managed_register_x86_64.cc
        src/dbi/x86_64/inline_hook_x86_64.cc
        src/dbi/x86_64/instruction_rewriter_x86_64.cc
        src/libffi/x86/ffiw64_x86_64.c
        src/libffi/x86/ffi64_x86_64.c
        src/libffi/x86/unix64_x86_64.S
        src/libffi/x86/win64_x86_64.S
        )


if (CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")

    set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -arch armv7")
    set(WHALE_SOURCES ${WHALE_SOURCES} ${WHALE_AARCH32})

elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^aarch64")

    set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -arch arm64")
    set(WHALE_SOURCES ${WHALE_SOURCES} ${WHALE_AARCH64})

elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^(i.86|x86?)$")

    set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -arch x86")
    set(WHALE_SOURCES ${WHALE_SOURCES} ${WHALE_X86} ${WHALE_DISASSEMBLER_X86_OR_X86_64})

elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^x86_64")

    set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -arch x86_64")
    set(WHALE_SOURCES ${WHALE_SOURCES} ${WHALE_X86_64} ${WHALE_DISASSEMBLER_X86_OR_X86_64})

endif ()


if (PLATFORM STREQUAL "Android")

    set(WHALE_SOURCES ${WHALE_SOURCES} ${WHALE_ANDROID_ART})

    if (CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")

        set(WHALE_SOURCES ${WHALE_SOURCES} ${WHALE_ANDROID_ART_AARCH32})

    elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^aarch64")

        set(WHALE_SOURCES ${WHALE_SOURCES} ${WHALE_ANDROID_ART_AARCH64})

    elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^(i.86|x86?)$")

        set(WHALE_SOURCES ${WHALE_SOURCES} ${WHALE_ANDROID_ART_X86})

    elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^x86_64")
        set(WHALE_SOURCES ${WHALE_SOURCES} ${WHALE_ANDROID_ART_X86_64})

    endif ()

endif ()

if (KERNEL STREQUAL "Linux")

    set(WHALE_SOURCES ${WHALE_SOURCES} ${WHALE_LINUX_SOURCES})

elseif (KERNEL STREQUAL "Darwin")
    set(WHALE_SOURCES ${WHALE_SOURCES} ${WHALE_DARWIN_SOURCES})
endif ()

if (CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm|aarch64)")
    include_directories(src/assembler/vixl)
endif ()

add_subdirectory(src/assembler/vixl)
include_directories(src)

add_definitions("-DWHALE_LIBRARY_NAME=\"lib${LIBRARY_NAME}.so\"")
add_library(${LIBRARY_NAME} STATIC ${WHALE_SOURCES})

target_include_directories(${LIBRARY_NAME} PUBLIC include)

if (CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm|aarch64)")
    target_link_libraries(${LIBRARY_NAME} vixl)
endif ()

if (PLATFORM STREQUAL "Android")
    target_link_libraries(${LIBRARY_NAME} log)
endif ()
