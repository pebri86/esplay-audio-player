# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES esp_lcd esp_timer driver fatfs)
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS "main.c sdcard.c lcd.c audio.c keypad.c mp3_metadata.c ui_player.c playlist.c app_context.c")
set(COMPONENT_ADD_INCLUDEDIRS "")

register_component()
