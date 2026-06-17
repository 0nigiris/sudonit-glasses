# ESP-IDF component manifest for the protocol layer.
#
# Reserved for a future modular split. The current ESP-IDF build compiles these
# sources directly inside the `main` component (firmware/esp-idf/main) to avoid
# cross-component link plumbing, and the host build compiles them via
# firmware/CMakeLists.txt. This manifest lets protocol become a standalone
# component later (add `firmware/components` to EXTRA_COMPONENT_DIRS) without
# rewriting it.
#
# On ESP-IDF, sha256 can later be backed by mbedtls instead of src/sha256.c;
# that swap happens here without touching call sites.

idf_component_register(
    SRCS
        "src/framing.c"
        "src/messages.c"
        "src/json.c"
        "src/sha256.c"
    INCLUDE_DIRS
        "include"
        "../../include")
