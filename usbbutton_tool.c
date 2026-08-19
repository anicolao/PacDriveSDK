#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <errno.h>
#include <hidapi/hidapi.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wchar.h>

#define VENDOR_ID 0xd209
#define PRODUCT_ID 0x1200
#define CONFIG_SIZE 64
#define REPORT_SIZE 5
#define KEYS_PER_SEQUENCE 24
#define KEYS_PER_ROW 6
#define PRIMARY_OFFSET 10
#define SECONDARY_OFFSET 34

#define MODE_ALTERNATE 0
#define MODE_EXTENDED 1
#define MODE_BOTH 2

struct key_name {
    const char *name;
    uint8_t value;
};

/* The device stores modifiers as 0x70..0x77, not HID 0xe0..0xe7. */
static const struct key_name key_names[] = {
    {"enter", 0x28}, {"return", 0x28}, {"esc", 0x29},
    {"escape", 0x29}, {"back", 0x2a}, {"backspace", 0x2a},
    {"tab", 0x2b}, {"space", 0x2c}, {"minus", 0x2d},
    {"equal", 0x2e}, {"leftbracket", 0x2f}, {"rightbracket", 0x30},
    {"backslash", 0x31}, {"semicolon", 0x33}, {"apostrophe", 0x34},
    {"grave", 0x35}, {"comma", 0x36}, {"period", 0x37},
    {"dot", 0x37}, {"slash", 0x38}, {"capslock", 0x39},
    {"printscreen", 0x46}, {"scrolllock", 0x47}, {"pause", 0x48},
    {"insert", 0x49}, {"home", 0x4a}, {"pageup", 0x4b},
    {"delete", 0x4c}, {"del", 0x4c}, {"end", 0x4d},
    {"pagedown", 0x4e}, {"right", 0x4f}, {"left", 0x50},
    {"down", 0x51}, {"up", 0x52}, {"numlock", 0x53},
    {"numslash", 0x54}, {"numstar", 0x55}, {"numminus", 0x56},
    {"numplus", 0x57}, {"numenter", 0x58}, {"numperiod", 0x63},
    {"application", 0x65}, {"menu", 0x65},
    {"lctrl", 0x70}, {"leftctrl", 0x70}, {"ctrl", 0x70},
    {"control", 0x70}, {"lshift", 0x71}, {"leftshift", 0x71},
    {"shift", 0x71}, {"lalt", 0x72}, {"leftalt", 0x72},
    {"alt", 0x72}, {"lwin", 0x73}, {"leftwin", 0x73},
    {"win", 0x73}, {"gui", 0x73}, {"meta", 0x73},
    {"rctrl", 0x74}, {"rightctrl", 0x74},
    {"rshift", 0x75}, {"rightshift", 0x75},
    {"ralt", 0x76}, {"rightalt", 0x76},
    {"rwin", 0x77}, {"rightwin", 0x77},
};

static void usage(FILE *stream) {
    fprintf(stream,
        "Usage:\n"
        "  usbbutton_tool --list\n"
        "  usbbutton_tool --read [--output FILE] [--verbose]\n"
        "  usbbutton_tool --configure [options]\n\n"
        "Configure options:\n"
        "  --permanent | --temporary      Store in flash or RAM (default: temporary)\n"
        "  --mode extended|alternate|both Action mode (default: extended)\n"
        "  --primary KEYS                 Primary 4x6 key grid\n"
        "  --secondary KEYS               Secondary 4x6 key grid\n"
        "  --released-color R,G,B         Released LED color\n"
        "  --pressed-color R,G,B          Pressed LED color\n"
        "  --input FILE                   Start from a 64-byte .ubn/config backup\n"
        "  --output FILE                  Save the resulting 64-byte configuration\n"
        "  --dry-run                      Build/print config without opening the device\n"
        "  --no-verify                    Do not read back and compare after programming\n"
        "  --verbose                      Print paths, decoded config, and HID reports\n\n"
        "Key syntax:\n"
        "  Separate keys in a row with ',' or '+'. Use ';' to start a new row.\n"
        "  Example: --primary 'ctrl+w'\n"
        "  Names include letters, digits, F1..F12, ctrl, shift, alt, win, enter,\n"
        "  arrows, and common punctuation. Raw values may be written as 0xNN.\n"
        "  HID modifiers 0xe0..0xe7 are translated to private codes 0x70..0x77.\n");
}

static const char *mode_name(uint8_t mode) {
    switch (mode) {
    case MODE_ALTERNATE: return "alternate";
    case MODE_EXTENDED: return "extended";
    case MODE_BOTH: return "both";
    default: return "unknown";
    }
}

static void print_bytes(const char *label, const uint8_t *data, size_t size) {
    printf("%s", label);
    for (size_t i = 0; i < size; i++)
        printf("%s%02x", i ? " " : "", data[i]);
    putchar('\n');
}

static void print_config(const uint8_t config[CONFIG_SIZE]) {
    printf("command: 0x%02x (%s)\n", config[0],
           config[0] == 0x50 ? "permanent" :
           config[0] == 0x51 ? "temporary" : "other");
    printf("mode: %s (0x%02x)\n", mode_name(config[2]), config[2]);
    printf("released color: %u,%u,%u\n", config[4], config[5], config[6]);
    printf("pressed color: %u,%u,%u\n", config[7], config[8], config[9]);
    for (size_t row = 0; row < 4; row++) {
        char label[32];
        snprintf(label, sizeof(label), "primary row %zu: ", row + 1);
        print_bytes(label, &config[PRIMARY_OFFSET + row * KEYS_PER_ROW], KEYS_PER_ROW);
    }
    for (size_t row = 0; row < 4; row++) {
        char label[32];
        snprintf(label, sizeof(label), "secondary row %zu: ", row + 1);
        print_bytes(label, &config[SECONDARY_OFFSET + row * KEYS_PER_ROW], KEYS_PER_ROW);
    }
}

static bool parse_u8(const char *text, uint8_t *value) {
    char *end = NULL;
    errno = 0;
    unsigned long parsed = strtoul(text, &end, 0);
    if (errno || end == text || *end != '\0' || parsed > 255)
        return false;
    *value = (uint8_t)parsed;
    return true;
}

static bool parse_color(const char *text, uint8_t color[3]) {
    unsigned int r, g, b;
    char extra;
    if (sscanf(text, "%u,%u,%u%c", &r, &g, &b, &extra) != 3 ||
        r > 255 || g > 255 || b > 255)
        return false;
    color[0] = (uint8_t)r;
    color[1] = (uint8_t)g;
    color[2] = (uint8_t)b;
    return true;
}

static void normalize_name(char *text) {
    char *write = text;
    for (char *read = text; *read; read++) {
        if (*read == '-' || *read == '_' || isspace((unsigned char)*read))
            continue;
        *write++ = (char)tolower((unsigned char)*read);
    }
    *write = '\0';
}

static bool parse_key(char *token, uint8_t *value) {
    normalize_name(token);
    if (!*token || strcmp(token, "none") == 0 || strcmp(token, "empty") == 0) {
        *value = 0;
        return true;
    }
    if (strlen(token) == 1 && token[0] >= 'a' && token[0] <= 'z') {
        *value = (uint8_t)(token[0] - 'a' + 4);
        return true;
    }
    if (strlen(token) == 1 && token[0] >= '1' && token[0] <= '9') {
        *value = (uint8_t)(token[0] - '1' + 0x1e);
        return true;
    }
    if (strcmp(token, "0") == 0) {
        *value = 0x27;
        return true;
    }
    if (token[0] == 'f') {
        char *end = NULL;
        long number = strtol(token + 1, &end, 10);
        if (end != token + 1 && *end == '\0' && number >= 1 && number <= 12) {
            *value = (uint8_t)(0x3a + number - 1);
            return true;
        }
    }
    for (size_t i = 0; i < sizeof(key_names) / sizeof(key_names[0]); i++) {
        if (strcmp(token, key_names[i].name) == 0) {
            *value = key_names[i].value;
            return true;
        }
    }
    if (strncmp(token, "0x", 2) == 0 && parse_u8(token, value)) {
        if (*value >= 0xe0 && *value <= 0xe7)
            *value = (uint8_t)(*value - 0x70);
        return true;
    }
    return false;
}

static bool parse_sequence(const char *text, uint8_t output[KEYS_PER_SEQUENCE],
                           char *error, size_t error_size) {
    memset(output, 0, KEYS_PER_SEQUENCE);
    if (!text || !*text)
        return true;
    char *copy = strdup(text);
    if (!copy) {
        snprintf(error, error_size, "out of memory");
        return false;
    }
    size_t position = 0;
    char *token = copy;
    for (char *cursor = copy;; cursor++) {
        char delimiter = *cursor;
        if (delimiter != ',' && delimiter != '+' && delimiter != ';' && delimiter != '\0')
            continue;
        *cursor = '\0';
        if (position >= KEYS_PER_SEQUENCE) {
            snprintf(error, error_size, "sequence has more than 24 cells");
            free(copy);
            return false;
        }
        if (!parse_key(token, &output[position])) {
            snprintf(error, error_size, "unknown key '%s'", token);
            free(copy);
            return false;
        }
        position++;
        if (delimiter == ';' && position % KEYS_PER_ROW)
            position += KEYS_PER_ROW - position % KEYS_PER_ROW;
        if (delimiter == '\0')
            break;
        token = cursor + 1;
    }
    free(copy);
    return true;
}

static bool load_config(const char *path, uint8_t config[CONFIG_SIZE]) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "Unable to open %s: %s\n", path, strerror(errno));
        return false;
    }
    size_t count = fread(config, 1, CONFIG_SIZE, file);
    int trailing = fgetc(file);
    bool okay = count == CONFIG_SIZE && trailing == EOF && !ferror(file);
    fclose(file);
    if (!okay)
        fprintf(stderr, "%s must contain exactly 64 bytes\n", path);
    return okay;
}

static bool save_config(const char *path, const uint8_t config[CONFIG_SIZE]) {
    FILE *file = fopen(path, "wb");
    if (!file) {
        fprintf(stderr, "Unable to create %s: %s\n", path, strerror(errno));
        return false;
    }
    bool okay = fwrite(config, 1, CONFIG_SIZE, file) == CONFIG_SIZE;
    if (fclose(file) != 0)
        okay = false;
    if (!okay)
        fprintf(stderr, "Unable to write %s: %s\n", path, strerror(errno));
    return okay;
}

static void sleep_ms(long milliseconds) {
    struct timespec delay = {
        .tv_sec = milliseconds / 1000,
        .tv_nsec = (milliseconds % 1000) * 1000000L,
    };
    while (nanosleep(&delay, &delay) == -1 && errno == EINTR) {}
}

static void list_devices(void) {
    struct hid_device_info *devices = hid_enumerate(VENDOR_ID, PRODUCT_ID);
    if (!devices) {
        printf("No %04x:%04x HID interfaces found.\n", VENDOR_ID, PRODUCT_ID);
        return;
    }
    for (struct hid_device_info *device = devices; device; device = device->next) {
        printf("path=%s usage_page=0x%04hx usage=0x%04hx interface=%d",
               device->path, device->usage_page, device->usage, device->interface_number);
        if (device->product_string)
            printf(" product=\"%ls\"", device->product_string);
        putchar('\n');
    }
    hid_free_enumeration(devices);
}

static char *find_config_path(bool verbose) {
    struct hid_device_info *devices = hid_enumerate(VENDOR_ID, PRODUCT_ID);
    char *path = NULL;
    size_t candidates = 0;
    for (struct hid_device_info *device = devices; device; device = device->next) {
        candidates++;
        if (verbose)
            printf("candidate path=%s usage_page=0x%04hx usage=0x%04hx interface=%d\n",
                   device->path, device->usage_page, device->usage,
                   device->interface_number);
        if (!path && device->usage_page == 0x01 && device->usage == 0x00)
            path = strdup(device->path);
    }
    if (!path && candidates == 1 && devices)
        path = strdup(devices->path);
    hid_free_enumeration(devices);
    return path;
}

static bool write_report(hid_device *handle, const uint8_t payload[4], bool verbose) {
    uint8_t report[REPORT_SIZE] = {0, payload[0], payload[1], payload[2], payload[3]};
    if (verbose)
        print_bytes("write: ", report, sizeof(report));
    int written = hid_write(handle, report, sizeof(report));
    if (written != (int)sizeof(report)) {
        fprintf(stderr, "HID write failed (%d): %ls\n", written, hid_error(handle));
        return false;
    }
    return true;
}

static bool write_config(hid_device *handle, const uint8_t config[CONFIG_SIZE], bool verbose) {
    for (size_t offset = 0; offset < CONFIG_SIZE; offset += 4) {
        if (!write_report(handle, &config[offset], verbose)) {
            fprintf(stderr, "Failed while writing configuration bytes %zu..%zu\n",
                    offset, offset + 3);
            return false;
        }
        sleep_ms(5);
    }
    return true;
}

static bool read_packet(hid_device *handle, uint8_t payload[4], bool verbose) {
    uint8_t report[REPORT_SIZE] = {0};
    int count = hid_read_timeout(handle, report, sizeof(report), 1000);
    if (count < 0) {
        fprintf(stderr, "HID read failed: %ls\n", hid_error(handle));
        return false;
    }
    if (count == 0) {
        fprintf(stderr, "Timed out waiting for configuration data\n");
        return false;
    }
    if (verbose)
        print_bytes("read:  ", report, (size_t)count);
    if (count == 4) {
        memcpy(payload, report, 4);
        return true;
    }
    if (count == 5 && report[0] == 0) {
        memcpy(payload, &report[1], 4);
        return true;
    }
    fprintf(stderr, "Unexpected HID input report length %d\n", count);
    return false;
}

static bool read_config(hid_device *handle, uint8_t config[CONFIG_SIZE], bool verbose) {
    const uint8_t request[4] = {0x59, 0xdd, 0x00, 0x00};
    if (!write_report(handle, request, verbose))
        return false;
    size_t offset = 0;
    unsigned int empty_packets = 0;
    while (offset < CONFIG_SIZE) {
        uint8_t payload[4];
        if (!read_packet(handle, payload, verbose))
            return false;
        if (offset == 0 && payload[0] == 0 && payload[1] == 0 &&
            payload[2] == 0 && payload[3] == 0) {
            if (++empty_packets > 4) {
                fprintf(stderr, "Device returned too many empty configuration packets\n");
                return false;
            }
            continue;
        }
        memcpy(&config[offset], payload, 4);
        offset += 4;
    }
    return true;
}

static hid_device *open_device(bool verbose) {
    char *path = find_config_path(verbose);
    if (!path) {
        fprintf(stderr,
                "Could not find the USB Button configuration interface "
                "(%04x:%04x, usage page 0x01, usage 0x00).\n",
                VENDOR_ID, PRODUCT_ID);
        list_devices();
        return NULL;
    }
    if (verbose)
        printf("opening configuration interface: %s\n", path);
    hid_device *handle = hid_open_path(path);
    if (!handle)
        fprintf(stderr, "Unable to open %s: %ls\n", path, hid_error(NULL));
    free(path);
    return handle;
}

int main(int argc, char **argv) {
    enum { COMMAND_NONE, COMMAND_LIST, COMMAND_READ, COMMAND_CONFIGURE } command = COMMAND_NONE;
    bool permanent = false, dry_run = false, verify = true, verbose = false;
    const char *input_path = NULL, *output_path = NULL;
    const char *primary_text = NULL, *secondary_text = NULL;
    bool have_mode = false, have_released = false, have_pressed = false;
    uint8_t mode = MODE_EXTENDED, released[3] = {0}, pressed[3] = {0};

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--list") == 0) command = COMMAND_LIST;
        else if (strcmp(argv[i], "--read") == 0) command = COMMAND_READ;
        else if (strcmp(argv[i], "--configure") == 0) command = COMMAND_CONFIGURE;
        else if (strcmp(argv[i], "--permanent") == 0) permanent = true;
        else if (strcmp(argv[i], "--temporary") == 0) permanent = false;
        else if (strcmp(argv[i], "--dry-run") == 0) dry_run = true;
        else if (strcmp(argv[i], "--no-verify") == 0) verify = false;
        else if (strcmp(argv[i], "--verbose") == 0) verbose = true;
        else if (strcmp(argv[i], "--input") == 0 && i + 1 < argc) input_path = argv[++i];
        else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) output_path = argv[++i];
        else if (strcmp(argv[i], "--primary") == 0 && i + 1 < argc) primary_text = argv[++i];
        else if (strcmp(argv[i], "--secondary") == 0 && i + 1 < argc) secondary_text = argv[++i];
        else if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
            const char *name = argv[++i];
            have_mode = true;
            if (strcmp(name, "alternate") == 0) mode = MODE_ALTERNATE;
            else if (strcmp(name, "extended") == 0) mode = MODE_EXTENDED;
            else if (strcmp(name, "both") == 0) mode = MODE_BOTH;
            else {
                fprintf(stderr, "Invalid mode '%s'\n", name);
                return 2;
            }
        } else if (strcmp(argv[i], "--released-color") == 0 && i + 1 < argc) {
            have_released = parse_color(argv[++i], released);
            if (!have_released) {
                fprintf(stderr, "Invalid released color\n");
                return 2;
            }
        } else if (strcmp(argv[i], "--pressed-color") == 0 && i + 1 < argc) {
            have_pressed = parse_color(argv[++i], pressed);
            if (!have_pressed) {
                fprintf(stderr, "Invalid pressed color\n");
                return 2;
            }
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            usage(stdout);
            return 0;
        } else {
            fprintf(stderr, "Unknown or incomplete option '%s'\n", argv[i]);
            usage(stderr);
            return 2;
        }
    }

    if (command == COMMAND_NONE) {
        usage(stderr);
        return 2;
    }
    if (hid_init() != 0) {
        fprintf(stderr, "Failed to initialize HIDAPI\n");
        return 1;
    }
    if (command == COMMAND_LIST) {
        list_devices();
        hid_exit();
        return 0;
    }

    uint8_t config[CONFIG_SIZE] = {0};
    config[0] = permanent ? 0x50 : 0x51;
    config[1] = 0xdd;
    config[2] = MODE_EXTENDED;
    config[3] = 0x11;
    memset(&config[4], 0xff, 6);

    if (command == COMMAND_CONFIGURE) {
        if (input_path && !load_config(input_path, config)) {
            hid_exit();
            return 1;
        }
        config[0] = permanent ? 0x50 : 0x51;
        config[1] = 0xdd;
        if (have_mode) config[2] = mode;
        if (have_released) memcpy(&config[4], released, 3);
        if (have_pressed) memcpy(&config[7], pressed, 3);
        char error[128];
        if (primary_text &&
            !parse_sequence(primary_text, &config[PRIMARY_OFFSET], error, sizeof(error))) {
            fprintf(stderr, "Invalid primary sequence: %s\n", error);
            hid_exit();
            return 2;
        }
        if (secondary_text &&
            !parse_sequence(secondary_text, &config[SECONDARY_OFFSET], error, sizeof(error))) {
            fprintf(stderr, "Invalid secondary sequence: %s\n", error);
            hid_exit();
            return 2;
        }
        if (output_path && !save_config(output_path, config)) {
            hid_exit();
            return 1;
        }
        if (verbose || dry_run)
            print_config(config);
        if (dry_run) {
            hid_exit();
            return 0;
        }
    }

    hid_device *handle = open_device(verbose);
    if (!handle) {
        hid_exit();
        return 1;
    }

    int result = 0;
    if (command == COMMAND_READ) {
        if (!read_config(handle, config, verbose))
            result = 1;
        else {
            print_config(config);
            if (output_path && !save_config(output_path, config))
                result = 1;
        }
    } else {
        if (!write_config(handle, config, verbose))
            result = 1;
        else if (verify) {
            sleep_ms(2500);
            uint8_t actual[CONFIG_SIZE];
            if (!read_config(handle, actual, verbose))
                result = 1;
            else if (memcmp(&config[2], &actual[2], 60) != 0) {
                fprintf(stderr, "Configuration readback did not match bytes 2..61\n");
                if (verbose) print_config(actual);
                result = 1;
            }
        }
        if (result == 0)
            printf("Configuration programmed%s.\n", verify ? " and verified" : "");
    }

    hid_close(handle);
    hid_exit();
    return result;
}
