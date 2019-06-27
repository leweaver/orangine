# Installs PDB files in debug builds
function(install_target_pdb _target)
        set(TGT_NAME $<TARGET_PROPERTY:${_target},NAME>)
        if(CMAKE_BUILD_TYPE MATCHES Debug)
                get_target_property(TGT_TYPE ${_target} TYPE)
                if(TGT_TYPE MATCHES STATIC_LIBRARY)
                        set(OE_PDB_NAME "${_target}${CMAKE_DEBUG_POSTFIX}")
                        set_target_properties(${_target} PROPERTIES
                                COMPILE_PDB_NAME ${OE_PDB_NAME})
                        install(FILES $<TARGET_FILE_DIR:${TGT_NAME}>/${OE_PDB_NAME}.pdb DESTINATION lib)
                else()
                        install(FILES $<TARGET_FILE_DIR:${TGT_NAME}>/${_target}.pdb DESTINATION bin)
                endif()
        endif()
endfunction(install_target_pdb)
