# fetchcontent_test

This folder contains a minimal project to verify that the `inifile` library can be consumed via CMake's `FetchContent` mechanism.

It simulates a typical usage scenario where the library is fetched directly from its Git repository without prior installation.

This test is used by CI to ensure that `inifile` integrates correctly when used as a FetchContent dependency, including compatibility with 
in-tree (local source) builds.