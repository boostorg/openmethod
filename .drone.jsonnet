# Copyright 2022 Peter Dimov
# Distributed under the Boost Software License, Version 1.0.
# https://www.boost.org/LICENSE_1_0.txt

local library = "openmethod";

local triggers =
{
    branch: [ "master", "develop", "feature/*", "fix/*" ]
};

local ubsan = { UBSAN: '1', UBSAN_OPTIONS: 'print_stacktrace=1' };
local asan = { ASAN: '1' };

local linux_pipeline(name, image, environment, packages = "", sources = [], arch = "amd64") =
{
    name: name,
    kind: "pipeline",
    type: "docker",
    trigger: triggers,
    platform:
    {
        os: "linux",
        arch: arch
    },
    steps:
    [
        {
            name: "everything",
            image: image,
            environment: environment,
            commands:
            [
                'set -e',
                'wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add -',
            ] +
            (if sources != [] then [ ('apt-add-repository "' + source + '"') for source in sources ] else []) +
            (if packages != "" then [ 'apt-get update', 'apt-get -y install ' + packages ] else []) +
            [
                'export LIBRARY=' + library,
                './.drone/drone.sh',
            ]
        }
    ]
};

local macos_pipeline(name, environment, xcode_version = "12.2", osx_version = "catalina", arch = "amd64") =
{
    name: name,
    kind: "pipeline",
    type: "exec",
    trigger: triggers,
    platform: {
        "os": "darwin",
        "arch": arch
    },
    node: {
        "os": osx_version
    },
    steps: [
        {
            name: "everything",
            environment: environment + { "DEVELOPER_DIR": "/Applications/Xcode-" + xcode_version + ".app/Contents/Developer" },
            commands:
            [
                'export LIBRARY=' + library,
                './.drone/drone.sh',
            ]
        }
    ]
};

local windows_pipeline(name, image, environment, arch = "amd64") =
{
    name: name,
    kind: "pipeline",
    type: "docker",
    trigger: triggers,
    platform:
    {
        os: "windows",
        arch: arch
    },
    "steps":
    [
        {
            name: "everything",
            image: image,
            environment: environment,
            commands:
            [
                'cmd /C .drone\\\\drone.bat ' + library,
            ]
        }
    ]
};

[
    # openmethod requires <charconv> (cxx17_hdr_charconv), which libstdc++ only
    # provides fully from GCC 11 / libstdc++-11. Clang on Linux uses the system
    # libstdc++, so the floor is the image, not the front-end. The old GCC 8/9/10
    # and Clang 7-12 jobs ran on Ubuntu 18.04/20.04 (libstdc++ < 11) and silently
    # executed *zero* openmethod tests. They are consolidated onto Ubuntu 24.04
    # with its default toolchain (gcc-13, clang-18); GCC 11-14 and Clang 13-18
    # already cover the intervening versions.
    linux_pipeline(
        "Linux 24.04 GCC 13 32/64",
        "cppalliance/droneubuntu2404:1",
        { TOOLSET: 'gcc', COMPILER: 'g++-13', CXXSTD: '17,20,2b', ADDRMD: '32,64' },
        "g++-13-multilib",
    ),

    linux_pipeline(
        "Linux 24.04 GCC 14 S390x",
        "cppalliance/droneubuntu2404:multiarch",
        { TOOLSET: 'gcc', COMPILER: 'g++-14', CXXSTD: '17,2a' },
        "g++-14-multilib",
        arch="s390x",
    ),

    linux_pipeline(
        "Linux 22.04 GCC 11* 32/64",
        "cppalliance/droneubuntu2204:1",
        { TOOLSET: 'gcc', COMPILER: 'g++', CXXSTD: '17,2a', ADDRMD: '32,64' },
    ),

    linux_pipeline(
        "Linux 22.04 GCC 12 32/64 C++17-20",
        "cppalliance/droneubuntu2204:1",
        { TOOLSET: 'gcc', COMPILER: 'g++-12', CXXSTD: '17,20', ADDRMD: '32,64' },
        "g++-12-multilib",
    ),

    linux_pipeline(
        "Linux 22.04 GCC 12 32/64 C++17-20",
        "cppalliance/droneubuntu2204:1",
        { TOOLSET: 'gcc', COMPILER: 'g++-12', CXXSTD: '2b', ADDRMD: '32,64' },
        "g++-12-multilib",
    ),

    linux_pipeline(
        "Linux 24.04 GCC 13 32/64 UBSAN C++17-20",
        "cppalliance/droneubuntu2404:1",
        { TOOLSET: 'gcc', COMPILER: 'g++-13', CXXSTD: '17,20', ADDRMD: '32,64' } + ubsan,
        "g++-13-multilib",
    ),

    linux_pipeline(
        "Linux 24.04 GCC 13 32/64 UBSAN",
        "cppalliance/droneubuntu2404:1",
        { TOOLSET: 'gcc', COMPILER: 'g++-13', CXXSTD: '2b', ADDRMD: '32,64' } + ubsan,
        "g++-13-multilib",
    ),

    linux_pipeline(
        "Linux 24.04 GCC 13 32 ASAN C++17-20",
        "cppalliance/droneubuntu2404:1",
        { TOOLSET: 'gcc', COMPILER: 'g++-13', CXXSTD: '17,20', ADDRMD: '32' } + asan,
        "g++-13-multilib",
    ),

    linux_pipeline(
        "Linux 24.04 GCC 13 32 ASAN",
        "cppalliance/droneubuntu2404:1",
        { TOOLSET: 'gcc', COMPILER: 'g++-13', CXXSTD: '2b', ADDRMD: '32' } + asan,
        "g++-13-multilib",
    ),

    linux_pipeline(
        "Linux 24.04 GCC 14 UBSAN C++17",
        "cppalliance/droneubuntu2404:1",
        { TOOLSET: 'gcc', COMPILER: 'g++-14', CXXSTD: '17' } + ubsan,
        "g++-14-multilib",
    ),

    linux_pipeline(
        "Linux 24.04 GCC 14 UBSAN C++20",
        "cppalliance/droneubuntu2404:1",
        { TOOLSET: 'gcc', COMPILER: 'g++-14', CXXSTD: '20' } + ubsan,
        "g++-14-multilib",
    ),

    linux_pipeline(
        "Linux 24.04 GCC 14 UBSAN",
        "cppalliance/droneubuntu2404:1",
        { TOOLSET: 'gcc', COMPILER: 'g++-14', CXXSTD: '2b' } + ubsan,
        "g++-14-multilib",
    ),

    linux_pipeline(
        "Linux 24.04 GCC 14 ASAN C++17",
        "cppalliance/droneubuntu2404:1",
        { TOOLSET: 'gcc', COMPILER: 'g++-14', CXXSTD: '17' } + asan,
        "g++-14-multilib",
    ),

    linux_pipeline(
        "Linux 24.04 GCC 14 ASAN C++20",
        "cppalliance/droneubuntu2404:1",
        { TOOLSET: 'gcc', COMPILER: 'g++-14', CXXSTD: '20' } + asan,
        "g++-14-multilib",
    ),

    linux_pipeline(
        "Linux 24.04 GCC 14 ASAN",
        "cppalliance/droneubuntu2404:1",
        { TOOLSET: 'gcc', COMPILER: 'g++-14', CXXSTD: '2b' } + asan,
        "g++-14-multilib",
    ),

    # Clang 7-12 ran on Ubuntu 20.04 (libstdc++ < 11, no <charconv>) and executed
    # zero tests; consolidated onto Ubuntu 24.04 with its default clang-18.
    linux_pipeline(
        "Linux 24.04 Clang 18",
        "cppalliance/droneubuntu2404:1",
        { TOOLSET: 'clang', COMPILER: 'clang++-18', CXXSTD: '17,20,2b' },
        "clang-18",
    ),

    linux_pipeline(
        "Linux 22.04 Clang 13",
        "cppalliance/droneubuntu2204:1",
        { TOOLSET: 'clang', COMPILER: 'clang++-13', CXXSTD: '17,20' },
        "clang-13",
    ),

    linux_pipeline(
        "Linux 22.04 Clang 14",
        "cppalliance/droneubuntu2204:1",
        { TOOLSET: 'clang', COMPILER: 'clang++-14', CXXSTD: '17,20,2b' },
        "clang-14",
    ),

    linux_pipeline(
        "Linux 22.04 Clang 15",
        "cppalliance/droneubuntu2204:1",
        { TOOLSET: 'clang', COMPILER: 'clang++-15', CXXSTD: '17,20,2b' },
        "clang-15",
    ),

    linux_pipeline(
        "Linux 24.04 Clang 16",
        "cppalliance/droneubuntu2404:1",
        { TOOLSET: 'clang', COMPILER: 'clang++-16', CXXSTD: '17,20,2b' },
        "clang-16",
    ),

    linux_pipeline(
        "Linux 24.04 Clang 17 UBSAN",
        "cppalliance/droneubuntu2404:1",
        { TOOLSET: 'clang', COMPILER: 'clang++-17', CXXSTD: '17,20,2b' } + ubsan,
        "clang-17",
    ),

    linux_pipeline(
        "Linux 24.04 Clang 17 ASAN",
        "cppalliance/droneubuntu2404:1",
        { TOOLSET: 'clang', COMPILER: 'clang++-17', CXXSTD: '17,20,2b' } + asan,
        "clang-17",
    ),

    linux_pipeline(
        "Linux 24.04 Clang 18 UBSAN",
        "cppalliance/droneubuntu2404:1",
        { TOOLSET: 'clang', COMPILER: 'clang++-18', CXXSTD: '17,20,2b' } + ubsan,
        "clang-18",
    ),

    linux_pipeline(
        "Linux 24.04 Clang 18 ASAN",
        "cppalliance/droneubuntu2404:1",
        { TOOLSET: 'clang', COMPILER: 'clang++-18', CXXSTD: '17,20,2b' } + asan,
        "clang-18",
    ),

    macos_pipeline(
        "MacOS 10.15 Xcode 12.2 UBSAN",
        { TOOLSET: 'clang', COMPILER: 'clang++', CXXSTD: '17,2a' } + ubsan,
    ),

    macos_pipeline(
        "MacOS 10.15 Xcode 12.2 ASAN",
        { TOOLSET: 'clang', COMPILER: 'clang++', CXXSTD: '17,2a' } + asan,
    ),

    macos_pipeline(
        "MacOS 12.4 Xcode 13.4.1 UBSAN",
        { TOOLSET: 'clang', COMPILER: 'clang++', CXXSTD: '17,20,2b', LINK: 'static' } + ubsan,
        xcode_version = "13.4.1", osx_version = "monterey", arch = "arm64",
    ),

    macos_pipeline(
        "MacOS 12.4 Xcode 13.4.1 ASAN",
        { TOOLSET: 'clang', COMPILER: 'clang++', CXXSTD: '17,20,2b' } + asan,
        xcode_version = "13.4.1", osx_version = "monterey", arch = "arm64",
    ),

    windows_pipeline(
        "Windows VS2019 msvc-14.2",
        "cppalliance/dronevs2019",
        { TOOLSET: 'msvc-14.2', CXXSTD: '17,20,latest', ADDRMD: '32,64' },
    ),

    windows_pipeline(
        "Windows VS2022 msvc-14.3",
        "cppalliance/dronevs2022:1",
        { TOOLSET: 'msvc-14.3', CXXSTD: '17,20,latest', ADDRMD: '32,64' },
    ),
]
