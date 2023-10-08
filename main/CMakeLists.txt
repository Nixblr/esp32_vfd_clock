idf_component_register(SRCS "backend.c" "http.c" "display.c" "http.c" "main.c" "vfd.c" "wifi_interface/wifiSta.c" "wifi_interface/wifiap.c" "wifi_interface/wifi_interface.c"
                    INCLUDE_DIRS "." "wifi_interface"
                    EMBED_FILES "../frontend/index.html" "../frontend/style.css" "../frontend/script.js")
