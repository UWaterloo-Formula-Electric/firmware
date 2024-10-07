file(GLOB_RECURSE SOURCES "src/*.c")
file(GLOB_RECURSE HEADERS "inc/*.h")

idf_component_register(SRCS ${SOURCES}
                    INCLUDE_DIRS "inc"
                    REQUIRES common driver)