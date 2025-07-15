
idf_component_register(
    SRCS main.c rgb_led.c wifi_app.c http_server.c DHT22.c     # list the source files of this component
    EMBED_FILES	webpage/app.css	webpage/app.js	webpage/index.html	webpage/favicon.ico	webpage/jquery-3.3.1.min.js
    INCLUDE_DIRS        # optional, add here public include directories
    PRIV_INCLUDE_DIRS   # optional, add here private include directories
    REQUIRES            # optional, list the public requirements (component names)
    PRIV_REQUIRES       # optional, list the private requirements
)
