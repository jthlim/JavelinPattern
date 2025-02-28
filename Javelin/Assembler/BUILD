load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "Assembler",
    srcs = [],
    visibility = ["//visibility:public"],
    deps = [":common"] + select({
        ":arm64_build": [":arm64"],
        ":x64_build": [":x64"],
        # "//conditions:default": [
        #     ":arm64",
        #     ":riscv",
        #     ":x64",
        # ],
    }),
)

cc_library(
    name = "common",
    srcs = [
        "BitUtility.cpp",
        "BitUtility.h",
        "JitForwardReferenceMap.cpp",
        "JitForwardReferenceMap.h",
        "JitHash.cpp",
        "JitHash.h",
        "JitLabelId.cpp",
        "JitLabelId.h",
        "JitLabelMap.cpp",
        "JitLabelMap.h",
        "JitLabelOffsetQueue.cpp",
        "JitLabelOffsetQueue.h",
        "JitMemoryManager.cpp",
        "JitMemoryManager.h",
        "JitVector.cpp",
        "JitVector.h",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "jasm_deps",
    srcs = [
        "BitUtility.cpp",
        "BitUtility.h",
        "JitHash.h",
        "JitLabelId.cpp",
        "JitLabelId.h",
        "arm64/ActionType.cpp",
        "arm64/ActionType.h",
        "arm64/ActionTypeTags.h",
        "arm64/RelEncoding.h",
        "riscv/ActionType.h",
        "riscv/ActionTypeTags.h",
        "riscv/RelEncoding.h",
        "x64/ActionType.h",
        "x64/ActionTypeTags.h",
    ],
    visibility = ["//Javelin/Tools/jasm:__subpackages__"],
)

cc_library(
    name = "arm64",
    srcs = [
        "JitForwardReferenceMap.h",
        "JitHash.h",
        "JitLabelId.h",
        "JitLabelMap.h",
        "JitMemoryManager.h",
        "JitVector.h",
        "arm64/ActionType.cpp",
        "arm64/ActionType.h",
        "arm64/ActionTypeTags.h",
        "arm64/Assembler.cpp",
        "arm64/Assembler.h",
        "arm64/RelEncoding.h",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "riscv",
    srcs = [
        "JitForwardReferenceMap.h",
        "JitHash.h",
        "JitLabelId.h",
        "JitLabelMap.h",
        "JitMemoryManager.h",
        "JitVector.h",
        "riscv/ActionType.h",
        "riscv/ActionTypeTags.h",
        "riscv/Assembler.cpp",
        "riscv/Assembler.h",
        "riscv/RelEncoding.h",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "x64",
    srcs = [
        "JitForwardReferenceMap.h",
        "JitHash.h",
        "JitLabelId.h",
        "JitLabelMap.h",
        "JitLabelOffsetQueue.h",
        "JitMemoryManager.h",
        "JitVector.h",
        "x64/ActionType.h",
        "x64/ActionTypeTags.h",
        "x64/Assembler.cpp",
        "x64/Assembler.h",
    ],
    visibility = ["//visibility:public"],
)

config_setting(
    name = "arm64_build",
    constraint_values = [
        "@platforms//cpu:arm64",
    ],
)

config_setting(
    name = "x64_build",
    constraint_values = [
        "@platforms//cpu:x86_64",
    ],
)
