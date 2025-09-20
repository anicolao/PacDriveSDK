#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hidapi/hidapi.h>
#include <ctype.h>

#define VENDOR_ID 0xd209
#define PRODUCT_ID 0x1200
#define KEY_BUFFER_SIZE 54
#define TEXT_CHUNK_SIZE 24

void print_usage() {
    printf("Usage: usbbutton_tool --configure [options]\n");
    printf("\nDescription:\n");
    printf("  Configures the USB Button's color and the key sequence it sends on press.\n");
    printf("  Note: Modifier keys (Ctrl, Shift, etc.) and multimedia keys are not supported\n");
    printf("        by the device's firmware via this configuration method.\n");
    printf("\nOptions:\n");
    printf("  --permanent           Make the configuration permanent.\n");
    printf("  --released-color R,G,B  Set the color when the button is released.\n");
    printf("  --pressed-color R,G,B   Set the color when the button is pressed.\n");
    printf("  --mode <mode>         'default' or 'alternate'.\n");
    printf("  --text \"...\"          Text for default mode (up to 54 chars).\n");
    printf("  --text1 \"...\"         Text for alternate mode (first press, up to 24 chars).\n");
    printf("  --text2 \"...\"         Text for alternate mode (second press, up to 24 chars).\n");
    printf("  --hex-codes \"C1,C2..\" For sending multi-key presses (chords). See examples.\n");
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

void encode_hex_codes(const char *hex_str, unsigned char *buffer, int max_len) {
    char *s = strdup(hex_str);
    char *token = strtok(s, ",");
    int i = 0;
    while (token != NULL && i < max_len) {
        buffer[i++] = (unsigned char)strtol(token, NULL, 16);
        token = strtok(NULL, ",");
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
    const char *text = NULL, *text1 = NULL, *text2 = NULL, *hex_codes = NULL;
    int mode = 2;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--permanent") == 0) command = 0x50;
        else if (strcmp(argv[i], "--released-color") == 0 && i + 1 < argc) sscanf(argv[++i], "%hhu,%hhu,%hhu", &config_buffer[2], &config_buffer[3], &config_buffer[4]);
        else if (strcmp(argv[i], "--pressed-color") == 0 && i + 1 < argc) sscanf(argv[++i], "%hhu,%hhu,%hhu", &config_buffer[5], &config_buffer[6], &config_buffer[7]);
        else if (strcmp(argv[i], "--text") == 0 && i + 1 < argc) text = argv[++i];
        else if (strcmp(argv[i], "--text1") == 0 && i + 1 < argc) text1 = argv[++i];
        else if (strcmp(argv[i], "--text2") == 0 && i + 1 < argc) text2 = argv[++i];
        else if (strcmp(argv[i], "--hex-codes") == 0 && i + 1 < argc) hex_codes = argv[++i];
        else if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
            i++;
            if (strcmp(argv[i], "alternate") == 0) mode = 0;
            else if (strcmp(argv[i], "default") == 0) mode = 2;
            else { fprintf(stderr, "Invalid mode specified. Only 'default' and 'alternate' are supported.\n"); return 1; }
        } else if (strcmp(argv[i], "--verbose") == 0) verbose = 1;
    }

    config_buffer[0] = mode;

    if (mode == 0) {
        encode_text_chunk(text1, &config_buffer[8], TEXT_CHUNK_SIZE);
        encode_text_chunk(text2, &config_buffer[8 + TEXT_CHUNK_SIZE], TEXT_CHUNK_SIZE);
    } else { // mode 2
        if (hex_codes) {
            encode_hex_codes(hex_codes, &config_buffer[8], KEY_BUFFER_SIZE);
        } else if (text) {
            encode_text_chunk(text, &config_buffer[8], KEY_BUFFER_SIZE);
        }
    }

    unsigned char report_buf[5] = {0};
    report_buf[0] = 0x0;
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

    unsigned char data_chunk[5] = {0};
    data_chunk[0] = 0x0;
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
