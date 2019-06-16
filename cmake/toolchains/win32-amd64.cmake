MESSAGE("Using win32-amd64 toolchain")

set(OE_TARGET_64 ON)
set(OE_CMAKE_GENERATION_PATH cmake-build-debug-amd64)

GET_FILENAME_COMPONENT(OE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." REALPATH)
set(CMAKE_MODULE_PATH "${OE_ROOT}/cmake/modules")
list(APPEND CMAKE_PREFIX_PATH "${OE_ROOT}/thirdparty/amd64/lib/cmake")
