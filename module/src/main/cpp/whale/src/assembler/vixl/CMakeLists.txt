set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -fdiagnostics-show-option -Wextra -Wredundant-decls -pedantic -Wwrite-strings -Wunused")

add_definitions(-DVIXL_CODE_BUFFER_MALLOC)

set(VIXL_SOURCES
        code-buffer-vixl.cc
        compiler-intrinsics-vixl.cc
        cpu-features.cc
        utils-vixl.cc)

set(VIXL_AARCH32
        aarch32/assembler-aarch32.cc
        aarch32/constants-aarch32.cc
        aarch32/instructions-aarch32.cc
        aarch32/location-aarch32.cc
        aarch32/macro-assembler-aarch32.cc
        aarch32/operands-aarch32.cc
        )

set(VIXL_AARCH64
        aarch64/assembler-aarch64.cc
        aarch64/cpu-aarch64.cc
        aarch64/cpu-features-auditor-aarch64.cc
        aarch64/decoder-aarch64.cc
        aarch64/instructions-aarch64.cc
        aarch64/instrument-aarch64.cc
        aarch64/logic-aarch64.cc
        aarch64/macro-assembler-aarch64.cc
        aarch64/operands-aarch64.cc
        aarch64/pointer-auth-aarch64.cc
        aarch64/simulator-aarch64.cc
        )


if (ENABLE_SIMULATOR)
    add_definitions(-DVIXL_INCLUDE_SIMULATOR_AARCH64)
    set(VIXL_SOURCES ${VIXL_SOURCES} ${VIXL_AARCH32} ${VIXL_AARCH64})
endif ()

if (CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
    add_definitions("-DVIXL_INCLUDE_TARGET_A32")
    add_definitions("-DVIXL_INCLUDE_TARGET_T32")
    set(VIXL_SOURCES ${VIXL_SOURCES} ${VIXL_AARCH32})
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^aarch64")
    set(VIXL_SOURCES ${VIXL_SOURCES} ${VIXL_AARCH64})
endif ()

if (CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm|aarch64)")
    add_library(vixl ${VIXL_SOURCES})
endif ()
