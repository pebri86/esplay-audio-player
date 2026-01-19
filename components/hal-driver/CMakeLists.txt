# Get all source files from the specified directories
# Globbing is used here to match the behavior of COMPONENT_SRCDIRS
file(GLOB_RECURSE SOURCES 
    "*.c"
)

# Register the component
idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS "include"
    REQUIRES esp_lcd esp_timer driver fatfs
)
