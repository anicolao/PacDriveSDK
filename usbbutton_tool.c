#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hidapi/hidapi.h>
#include <ctype.h>

#define VENDOR_ID 0xd209
#define PRODUCT_ID 0x1200
#define KEY_BUFFER_SIZE 54
#define TEXT_CHUNK_SIZE 24

typedef struct {
    const char *name;
    unsigned char id;
} KeyCode;

KeyCode consumer_keys[] = {
    {"play_pause", 0xCD}, {"stop", 0xB7}, {"scan_next", 0xB5},
    {"scan_prev", 0xB6}, {"volume_up", 0xE9}, {"volume_down", 0xEA},
    {"mute", 0xE2}, {NULL, 0}
};

void print_usage() {
    printf("Usage: usbbutton_tool --configure [options]\n");
    printf("\nOptions:\n");
    printf("  --permanent           Make the configuration permanent.\n");
    printf("  --released-color R,G,B  Set the color when the button is released.\n");
    printf("  --pressed-color R,G,B   Set the color when the button is pressed.\n");
    printf("  --mode <mode>         'default', 'alternate', 'extended', 'macro'\n");
    printf("  --text \"...\"          Text for default mode.\n");
    printf("  --text1 \"...\"         Text for alternate mode (first press).\n");
    printf("  --text2 \"...\"         Text for alternate mode (second press).\n");
    printf("  --keys \"key1,...\"     Keys for extended mode (e.g., volume_up,mute).\n");
    printf("  --macro \"...\"         Macro for macro mode. See EXAMPLES.md for format.\n");
    printf("  --verbose             Print the raw data packets being sent.\n");
}

void print_packet(const unsigned char* data, size_t length) {
    printf("Sending packet: ");
    for (size_t i = 0; i < length; i++) {
        printf("%02x ", data[i]);
    }
    printf("\n");
}

void encode_text_chunk(const char *text, unsigned char *buffer, int max_len) {
    for (int i = 0; i < max_len; i++) {
        if (text && i < strlen(text)) {
            char c = toupper(text[i]);
            if (c >= 'A' && c <= 'Z') buffer[i] = (unsigned char)(c - 'A' + 4);
            else if (c == ' ') buffer[i] = 0x2C;
            else buffer[i] = 0;
        } else {
            buffer[i] = 0;
        }
    }
}

void encode_consumer_keys(const char *keys_str, unsigned char *buffer, int max_len) {
    char *keys = strdup(keys_str);
    char *token = strtok(keys, ",");
    int i = 0;
    while (token != NULL && i < max_len) {
        int found = 0;
        for (int j = 0; consumer_keys[j].name != NULL; j++) {
            if (strcmp(token, consumer_keys[j].name) == 0) {
                buffer[i] = consumer_keys[j].id;
                found = 1;
                break;
            }
        }
        if (!found) fprintf(stderr, "Warning: Unknown key '%s'\n", token);
        token = strtok(NULL, ",");
        i++;
    }
    free(keys);
}

void encode_macro(const char *macro_str, unsigned char *buffer, int max_len) {
    char *s = strdup(macro_str);
    char *report_token = strtok(s, ",");
    int byte_index = 0;
    while(report_token != NULL && byte_index < max_len) {
        char *hex_token = strtok(report_token, ":");
        while(hex_token != NULL && byte_index < max_len) {
            buffer[byte_index++] = (unsigned char)strtol(hex_token, NULL, 16);
            hex_token = strtok(NULL, ":");
        }
        report_token = strtok(NULL, ",");
    }
    free(s);
}

char* find_config_interface_path() {
    struct hid_device_info *devs, *cur_dev;
    devs = hid_enumerate(VENDOR_ID, PRODUCT_ID);
    cur_dev = devs;
    char *path = NULL;
    while (cur_dev) {
        if (cur_dev->usage_page == 0) {
            path = strdup(cur_dev->path);
            break;
        }
        cur_dev = cur_dev->next;
    }
    hid_free_enumeration(devs);
    return path;
}

int main(int argc, char* argv[]) {
    if (argc < 2 || strcmp(argv[1], "--configure") != 0) {
        print_usage();
        return 1;
    }

    if (hid_init()) {
        fprintf(stderr, "Error: Failed to initialize HIDAPI.\n");
        return 1;
    }

    char *config_path = find_config_interface_path();
    if (config_path == NULL) {
        fprintf(stderr, "Error: Could not find configuration interface for the device.\n");
        hid_exit();
        return 1;
    }

    hid_device *handle = hid_open_path(config_path);
    if (!handle) {
        fprintf(stderr, "Error: Unable to open device config interface at %s\n", config_path);
        free(config_path);
        hid_exit();
        return 1;
    }
    free(config_path);

    unsigned char config_buffer[62] = {0};
    unsigned char command = 0x51;
    int verbose = 0;
    const char *text = NULL, *text1 = NULL, *text2 = NULL, *keys = NULL, *macro = NULL;
    int mode = 2;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--permanent") == 0) command = 0x50;
        else if (strcmp(argv[i], "--released-color") == 0 && i + 1 < argc) sscanf(argv[++i], "%hhu,%hhu,%hhu", &config_buffer[2], &config_buffer[3], &config_buffer[4]);
        else if (strcmp(argv[i], "--pressed-color") == 0 && i + 1 < argc) sscanf(argv[++i], "%hhu,%hhu,%hhu", &config_buffer[5], &config_buffer[6], &config_buffer[7]);
        else if (strcmp(argv[i], "--text") == 0 && i + 1 < argc) text = argv[++i];
        else if (strcmp(argv[i], "--text1") == 0 && i + 1 < argc) text1 = argv[++i];
        else if (strcmp(argv[i], "--text2") == 0 && i + 1 < argc) text2 = argv[++i];
        else if (strcmp(argv[i], "--keys") == 0 && i + 1 < argc) keys = argv[++i];
        else if (strcmp(argv[i], "--macro") == 0 && i + 1 < argc) macro = argv[++i];
        else if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
            i++;
            if (strcmp(argv[i], "alternate") == 0) mode = 0;
            else if (strcmp(argv[i], "extended") == 0) mode = 1;
            else if (strcmp(argv[i], "default") == 0) mode = 2;
            else if (strcmp(argv[i], "macro") == 0) mode = 3;
            else { fprintf(stderr, "Invalid mode specified.\n"); return 1; }
        } else if (strcmp(argv[i], "--verbose") == 0) verbose = 1;
    }

    config_buffer[0] = mode;
    if (mode == 3) { // Macro mode
        config_buffer[0] = 2; // Tell firmware it's a keyboard-style report
        if (macro) encode_macro(macro, &config_buffer[8], KEY_BUFFER_SIZE);
    } else if (mode == 0) { // Alternate mode
        encode_text_chunk(text1, &config_buffer[8], TEXT_CHUNK_SIZE);
        encode_text_chunk(text2, &config_buffer[8 + TEXT_CHUNK_SIZE], TEXT_CHUNK_SIZE);
    } else if (mode == 1) { // Extended mode
        if (keys) encode_consumer_keys(keys, &config_buffer[8], KEY_BUFFER_SIZE);
    } else { // Default mode
        encode_text_chunk(text, &config_buffer[8], KEY_BUFFER_SIZE);
    }

    // This section is a direct translation of the C++ DLL's writing logic.
    unsigned char report_buf[5] = {0}; // Report ID (0) + 4 bytes of data

    // First, send the command report
    report_buf[0] = 0x0; // Report ID
    report_buf[1] = command;
    report_buf[2] = 0xdd;
    report_buf[3] = config_buffer[0]; // Mode
    report_buf[4] = config_buffer[1]; // Spare

    if (verbose) print_packet(report_buf, 5);
    if (hid_write(handle, report_buf, 5) == -1) {
        fprintf(stderr, "Error writing command to device.\n");
        hid_close(handle);
        hid_exit();
        return 1;
    }

    // Now, send the 60 bytes of config data in 15 4-byte chunks
    unsigned char data_chunk[5] = {0}; // Report ID (0) + 4 bytes
    data_chunk[0] = 0x0; // Report ID

    for (int i = 0; i < 15; i++) {
        memcpy(&data_chunk[1], &config_buffer[2 + i * 4], 4);
        if (verbose) print_packet(data_chunk, 5);
        if (hid_write(handle, data_chunk, 5) == -1) {
            fprintf(stderr, "Error writing data packet %d.\n", i);
            break;
        }
    }

    printf("Configuration sent successfully.\n");

    hid_close(handle);
    hid_exit();
    return 0;
}
