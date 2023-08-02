# SynthPedal

    mkdir build
    cd build
    cmake -GNinja -DTOOLCHAIN_PREFIX=/opt/homebrew -DCMAKE_TOOLCHAIN_FILE=lib/libDaisy/cmake/toolchains/stm32h750xx.cmake ..
    ninja
