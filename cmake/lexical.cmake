get_target_property(LEMON_LEMPAR_LOCATION lexical::lemon SOURCE_DIR)

macro(lemon CVAR HVAR)
    foreach (SRC ${ARGN})
        get_filename_component(DST "${SRC}" PATH)
        get_filename_component(NAME "${SRC}" NAME_WE)
        set(C_ONAME ${CMAKE_CURRENT_BINARY_DIR}/${DST}/${NAME}.c)
        set(H_ONAME ${CMAKE_CURRENT_BINARY_DIR}/${DST}/${NAME}.h)
        set(LEMON_C_ONAME ${CMAKE_CURRENT_SOURCE_DIR}/${DST}/${NAME}.c)
        set(LEMON_H_ONAME ${CMAKE_CURRENT_SOURCE_DIR}/${DST}/${NAME}.h)
        set(LEMON_O_ONAME ${CMAKE_CURRENT_SOURCE_DIR}/${DST}/${NAME}.out)
        add_custom_command(
                OUTPUT ${C_ONAME} ${H_ONAME}
                BYPRODUCTS ${C_ONAME} ${H_ONAME}
                COMMAND cmake -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/${DST}"
                COMMAND $<TARGET_FILE:lexical::lemon> -s ${CMAKE_CURRENT_SOURCE_DIR}/${SRC} -T${LEMON_LEMPAR_LOCATION}/lempar.c
                COMMAND cmake -E copy ${LEMON_C_ONAME} ${C_ONAME}
                COMMAND cmake -E copy ${LEMON_H_ONAME} ${H_ONAME}
                COMMAND cmake -E remove ${LEMON_C_ONAME}
                COMMAND cmake -E remove ${LEMON_H_ONAME}
                COMMAND cmake -E remove ${LEMON_O_ONAME}
                DEPENDS lexical::lemon "${SRC}"
        )
        set(${CVAR} ${${CVAR}} ${C_ONAME})
        set(${HVAR} ${${HVAR}} ${H_ONAME})
    endforeach ()
endmacro()


macro(re2c VAR SRC)
    get_filename_component(DST "${SRC}" PATH)
    get_filename_component(NAME "${SRC}" NAME_WE)
    set(ONAME ${CMAKE_CURRENT_BINARY_DIR}/${DST}/${NAME}.c)
    add_custom_command(
            OUTPUT "${ONAME}"
            BYPRODUCTS ${ONAME}
            COMMAND cmake -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/${DST}"
            COMMAND $<TARGET_FILE:lexical::re2c> -s -o ${ONAME} "${CMAKE_CURRENT_SOURCE_DIR}/${SRC}"
            DEPENDS "${SRC}"
            DEPENDS lexical::re2c ${ARGN}
    )
    set(${VAR} ${${VAR}} ${ONAME})
endmacro()
