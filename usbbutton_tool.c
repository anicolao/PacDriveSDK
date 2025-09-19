#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hidapi/hidapi.h>
#include <ctype.h>

#define VENDOR_ID 0xd209
#define PRODUCT_ID 0x1200
#define TEXT_CHUNK_SIZE 24
#define TEXT_BUFFER_SIZE (TEXT_CHUNK_SIZE * 2)

typedef struct {
    const char *name;
    unsigned char id;
} KeyCode;

KeyCode consumer_keys[] = {
    {"play_pause", 0xCD},
    {"stop", 0xB7},
    {"scan_next", 0xB5},
    {"scan_prev", 0xB6},
    {"volume_up", 0xE9},
    {"volume_down", 0xEA},
    {"mute", 0xE2},
    {NULL, 0}
};

void print_usage() {
    printf("Usage: usbbutton_tool <command> [options]\n");
    printf("\nCommands:\n");
    printf("  --configure           Configure the button's behavior and colors.\n");
    printf("  --set-color           Set the button's current color.\n");
    printf("  --get-state           Get the button's pressed/released state.\n");
    printf("\nOptions for --configure:\n");
    printf("  --permanent           Make the configuration permanent.\n");
    printf("  --released-color R,G,B  Set the color when the button is released.\n");
    printf("  --pressed-color R,G,B   Set the color when the button is pressed.\n");
    printf("  --mode <mode>         Set the operational mode. <mode> can be:\n");
    printf("                        'default': Send keys from --text on press.\n");
    printf("                        'alternate': Use --text1 on first press, --text2 on second.\n");
    printf("                        'extended': Use --keys to send multimedia/extended keys.\n");
    printf("  --text \"...\"          Text for default mode.\n");
    printf("  --text1 \"...\"         Text for alternate mode (first press).\n");
    printf("  --text2 \"...\"         Text for alternate mode (second press).\n");
    printf("  --keys \"key1,key2..\"  Comma-separated list of keys for extended mode.\n");
    printf("                        Available keys: play_pause, stop, scan_next, scan_prev,\n");
    printf("                                      volume_up, volume_down, mute\n");
    printf("  --verbose             Print the raw data packets being sent.\n");
}

void print_packet(const unsigned char* data, size_t length) {
    printf("Sending packet: ");
    for (size_t i = 0; i < length; i++) {
        printf("%02x ", data[i]);
    }
    printf("\n");
}

void parse_color(const char *color_str, unsigned char *r, unsigned char *g, unsigned char *b) {
    if (sscanf(color_str, "%hhu,%hhu,%hhu", r, g, b) != 3) {
        fprintf(stderr, "Error: Invalid color format. Use R,G,B.\n");
        exit(1);
    }
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

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    if (hid_init()) {
        fprintf(stderr, "Error: Failed to initialize HIDAPI.\n");
        return 1;
    }

    hid_device *handle = hid_open(VENDOR_ID, PRODUCT_ID, NULL);
    if (!handle) {
        fprintf(stderr, "Error: Unable to open device.\n");
        hid_exit();
        return 1;
    }

    if (strcmp(argv[1], "--configure") == 0) {
        unsigned char config_data[62] = {0};
        unsigned char command = 0x51;
        int verbose = 0;
        const char *text = NULL, *text1 = NULL, *text2 = NULL, *keys = NULL;
        int mode = 2; // default

        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "--permanent") == 0) command = 0x50;
            else if (strcmp(argv[i], "--released-color") == 0 && i + 1 < argc) parse_color(argv[++i], &config_data[2], &config_data[3], &config_data[4]);
            else if (strcmp(argv[i], "--pressed-color") == 0 && i + 1 < argc) parse_color(argv[++i], &config_data[5], &config_data[6], &config_data[7]);
            else if (strcmp(argv[i], "--text") == 0 && i + 1 < argc) text = argv[++i];
            else if (strcmp(argv[i], "--text1") == 0 && i + 1 < argc) text1 = argv[++i];
            else if (strcmp(argv[i], "--text2") == 0 && i + 1 < argc) text2 = argv[++i];
            else if (strcmp(argv[i], "--keys") == 0 && i + 1 < argc) keys = argv[++i];
            else if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
                i++;
                if (strcmp(argv[i], "alternate") == 0) mode = 0;
                else if (strcmp(argv[i], "extended") == 0) mode = 1;
                else if (strcmp(argv[i], "default") == 0) mode = 2;
                else { fprintf(stderr, "Invalid mode specified.\n"); return 1; }
            } else if (strcmp(argv[i], "--verbose") == 0) verbose = 1;
        }

        config_data[0] = mode;

        if (mode == 0) { // alternate
            encode_text_chunk(text1, &config_data[8], TEXT_CHUNK_SIZE);
            encode_text_chunk(text2, &config_data[8 + TEXT_CHUNK_SIZE], TEXT_CHUNK_SIZE);
        } else if (mode == 1) { // extended
            if (keys) encode_consumer_keys(keys, &config_data[8], TEXT_BUFFER_SIZE);
        } else { // default
            encode_text_chunk(text, &config_data[8], TEXT_BUFFER_SIZE);
        }

        unsigned char report_buf[65] = {0};
        report_buf[0] = 0x0;
        report_buf[1] = command;
        report_buf[2] = 0xdd;
        report_buf[3] = config_data[0];
        report_buf[4] = config_data[1];

        if (verbose) print_packet(report_buf, 5);
        if (hid_write(handle, report_buf, 5) == -1) {
            fprintf(stderr, "Error writing command to device.\n");
        } else {
            for (int i = 0; i < 15; i++) {
                memcpy(&report_buf[1], &config_data[2 + i * 4], 4);
                if (verbose) print_packet(report_buf, 5);
                if (hid_write(handle, report_buf, 5) == -1) {
                    fprintf(stderr, "Error writing data packet %d.\n", i);
                    break;
                }
            }
            printf("Configuration sent successfully.\n");
        }

    } else if (strcmp(argv[1], "--set-color") == 0) {
        unsigned char r = 0, g = 0, b = 0;
        if (argc > 2 && strcmp(argv[2], "--color") == 0 && argc > 3) parse_color(argv[3], &r, &g, &b);
        else print_usage();

        unsigned char report_buf[65] = {0};
        report_buf[0] = 0x0;
        report_buf[1] = 0x01;
        report_buf[2] = r;
        report_buf[3] = g;
        report_buf[4] = b;
        if (hid_write(handle, report_buf, 5) == -1) fprintf(stderr, "Error writing to device.\n");
        else printf("Color set successfully.\n");

    } else if (strcmp(argv[1], "--get-state") == 0) {
        unsigned char report_buf[65] = {0};
        report_buf[0] = 0x0;
        report_buf[1] = 0x02;
        if (hid_write(handle, report_buf, 5) == -1) fprintf(stderr, "Error writing to device.\n");
        else {
            unsigned char res[2];
            if (hid_read(handle, res, sizeof(res)) == -1) fprintf(stderr, "Error reading from device.\n");
            else printf("Button state: %s\n", res[0] ? "Pressed" : "Released");
        }
    } else {
        print_usage();
    }

    hid_close(handle);
    hid_exit();
    return 0;
}
