# Get all source files from the specified directories
# Globbing is used here to match the behavior of COMPONENT_SRCDIRS
file(GLOB_RECURSE SOURCES 
    "src/*.c"
    "src/*.cpp"
    "src/xmplite/*.c"
    "src/xmplite/loaders/*.c"
    "src/gme/gme/*.cpp"
    "src/gme/gme/*.c"
)

# Register the component
idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS "include" "src/xmplite" "src/gme/gme"
)

# Add preprocessor definitions (replacing CFLAGS/CXXFLAGS)
target_compile_definitions(${COMPONENT_LIB} PRIVATE 
    LIBXMP_CORE_PLAYER
    VGM_YM2612_NUKED
)
