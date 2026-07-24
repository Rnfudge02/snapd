#include "classic.h"
#include "../libsnap-confine-private/cleanup-funcs.h"
#include "../libsnap-confine-private/infofile.h"
#include "../libsnap-confine-private/string-utils.h"
#include "../libsnap-confine-private/utils.h"
#include "config.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static const char *os_release = "/etc/os-release";
static const char *meta_snap_yaml = "/meta/snap.yaml";

sc_distro sc_classify_distro(void) {
    FILE *f SC_CLEANUP(sc_cleanup_file) = fopen(os_release, "r");
    if (f == NULL) {
        return SC_DISTRO_CLASSIC;
    }

    bool is_core = false;
    int core_version = 0;
    char buf[255] = {0};

    while (fgets(buf, sizeof buf, f) != NULL) {
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') {
            buf[len - 1] = '\0';
        }
        if (sc_streq(buf, "ID=\"ubuntu-core\"") || sc_streq(buf, "ID=ubuntu-core")) {
            is_core = true;
        } else if (sc_streq(buf, "VERSION_ID=\"16\"") || sc_streq(buf, "VERSION_ID=16")) {
            core_version = 16;
        } else if (sc_streq(buf, "VARIANT_ID=\"snappy\"") || sc_streq(buf, "VARIANT_ID=snappy")) {
            is_core = true;
        }
    }

    if (!is_core) {
        /* Since classic systems don't have a /meta/snap.yaml file the simple
           presence of that file qualifies as SC_DISTRO_CORE_OTHER. */
        if (access(meta_snap_yaml, F_OK) == 0) {
            is_core = true;
        }
    }

    sc_distro result;
    if (is_core) {
        result = (core_version == 16) ? SC_DISTRO_CORE16 : SC_DISTRO_CORE_OTHER;
    } else {
        result = SC_DISTRO_CLASSIC;
    }
    return result;
}

bool sc_is_debian_like(void) {
    FILE *f SC_CLEANUP(sc_cleanup_file) = fopen(os_release, "r");
    if (f == NULL) {
        return false;
    }
    const char *const id_keys_to_try[] = {
        "ID",      /* actual debian only sets ID */
        "ID_LIKE", /* distros based on debian */
    };
    size_t id_keys_to_try_len = SC_ARRAY_SIZE(id_keys_to_try);
    bool result = false;
    for (size_t i = 0; i < id_keys_to_try_len; i++) {
        if (fseek(f, 0L, SEEK_SET) == -1) {
            break;
        }
        char *id_val SC_CLEANUP(sc_cleanup_string) = NULL;
        struct sc_error *err SC_CLEANUP(sc_cleanup_error) = NULL;
        int rc = sc_infofile_get_key(f, id_keys_to_try[i], &id_val, &err);
        if (rc != 0) {
            /* only if sc_infofile_get_key failed */
            break;
        }
        if (sc_streq(id_val, "\"debian\"") || sc_streq(id_val, "debian")) {
            result = true;
            break;
        }
    }
    return result;
}
