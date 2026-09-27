set(VCPKG_OSX_DEPLOYMENT_TARGET 15.0)
set(VCPKG_TARGET_ARCHITECTURE arm64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME iOS)

# CMake 4 removed compatibility modes older than 3.5. Some dependencies in
# the pinned vcpkg baseline (notably mbedTLS 2.28.8) still declare an older
# cmake_minimum_required(). Pass the compatibility floor to vcpkg CMake ports.
list(APPEND VCPKG_CMAKE_CONFIGURE_OPTIONS "-DCMAKE_POLICY_VERSION_MINIMUM=3.5")
