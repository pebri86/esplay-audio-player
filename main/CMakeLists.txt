# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES esp_lcd esp_timer hal-driver)
set(COMPONENT_PRIV_REQUIRES acodecs)

set(COMPONENT_SRCS "main.c mp3_metadata.c ui_player.c app_context.c audio_player.c")
set(COMPONENT_ADD_INCLUDEDIRS "include")

register_component()
