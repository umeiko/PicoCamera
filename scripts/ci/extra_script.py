# -Wno-overloaded-virtual is C++-only; passing it via build_flags breaks C
# sources. arduino-pico CI adds it via compiler.cpp.extra_flags, so mirror
# that by appending to CXXFLAGS only.
Import("env")

env.Append(CXXFLAGS=["-Wno-overloaded-virtual"])
