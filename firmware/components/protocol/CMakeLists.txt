# ESP-IDF component registration for the protocol layer.
#
# This makes protocol/ a first-class ESP-IDF component for the (future) on-target
# build. The host build does NOT use this file — it compiles the same sources
# directly via firmware/CMakeLists.txt. Kept here so the on-target build is a
# drop-in once hardware arrives.
#
# Note: on ESP-IDF, sha256 can be backed by mbedtls instead of src/sha256.c;
# that swap happens here without touching call sites.

idf_component_register(
    SRCS
        "src/framing.c"
        "src/messages.c"
        "src/json.c"
        "src/sha256.c"
    INCLUDE_DIRS
        "include"
    REQUIRES
        # provides sudonit/hal/transport.h + the Wi-Fi transport backend
        sudonit_hal
)
