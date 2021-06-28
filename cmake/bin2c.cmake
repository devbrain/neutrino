function(BIN2C input_file RESULT_NAME )
    get_filename_component(INPUT_DIR ${input_file} DIRECTORY)
    get_filename_component(INPUT_NAME ${input_file} NAME_WE)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${INPUT_DIR})
    set (ONAME ${CMAKE_CURRENT_BINARY_DIR}/${INPUT_DIR}/${INPUT_NAME}.h)
    add_custom_command(
            OUTPUT ${ONAME}
            COMMAND $<TARGET_FILE:bin2c> ${CMAKE_CURRENT_SOURCE_DIR}/${input_file} ${ONAME} ${INPUT_NAME}
            DEPENDS bin2c
            MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/${input_file}
    )
    set(${RESULT_NAME} ${ONAME} PARENT_SCOPE)
endfunction()