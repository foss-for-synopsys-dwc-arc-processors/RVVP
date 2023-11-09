# Temporary disable all target platforms except basic (riscv-vp)

# file(GLOB sources_list LIST_DIRECTORIES true *)
# foreach(dir ${sources_list})
#    IF(IS_DIRECTORY ${dir})
#        add_subdirectory(${dir})
#    ELSE()
#        CONTINUE()
#    ENDIF()
# endforeach()

add_subdirectory(common)
add_subdirectory(basic)
