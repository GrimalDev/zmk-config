if(CONFIG_CORNE_OLED)
  target_sources(app PRIVATE
    modules/corne_oled/render.c
    modules/corne_oled/screen.c
    modules/corne_oled/wpm_state.c
    modules/corne_oled/wpm_split.c
  )
endif()
