get_filename_component(THEFORGE_ROOT_DIR "${Orangine_SOURCE_DIR}/../thirdparty/The-Forge" ABSOLUTE)
set(THEFORGE_OUTPUT_DIR "${THEFORGE_ROOT_DIR}/Examples_3/Unit_Tests" CACHE PATH "")

if(IS_DIRECTORY "${THEFORGE_OUTPUT_DIR}/PC Visual Studio 2017")
        set(THEFORGE_LIB_DIR "${THEFORGE_OUTPUT_DIR}/PC Visual Studio 2017/x64/${CMAKE_BUILD_TYPE}Dx" CACHE PATH "")
        set(THEFORGE_INCLUDE_DIR ${THEFORGE_ROOT_DIR} CACHE PATH "")

        message(STATUS "Found The-Forge: ${THEFORGE_ROOT_DIR}")
else()
        message(FATAL "Could not find valid The-Forge build directory")
endif()

set(THEFORGE_STATIC_LIBRARIES 
        "${THEFORGE_LIB_DIR}/RendererDX12.lib"
        "${THEFORGE_LIB_DIR}/gainputstatic.lib"
        "${THEFORGE_LIB_DIR}/OS.lib"
        )
set(THEFORGE_STATIC_LIBRARIES ${THEFORGE_STATIC_LIBRARIES} PARENT_SCOPE)

set(THEFORGE_DYNAMIC_LIBRARIES 
        "${THEFORGE_LIB_DIR}/dxcompiler.dll"
        "${THEFORGE_LIB_DIR}/dxil.dll"
        "${THEFORGE_LIB_DIR}/WinPixEventRunTime.dll"
        )
set(THEFORGE_DYNAMIC_LIBRARIES ${THEFORGE_DYNAMIC_LIBRARIES} PARENT_SCOPE)
