mkdir CMake-GCC-Build
cd CMake-GCC-Build

for build_type in Debug
do
    cmake .. -DCMAKE_BUILD_TYPE="${build_type}" -B "${build_type}"
    cd "${build_type}" && make -j12 -k
    cd ..
done
