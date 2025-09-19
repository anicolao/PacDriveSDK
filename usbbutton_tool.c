#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hidapi/hidapi.h>
#include <ctype.h>

#define VENDOR_ID 0xd209
#define PRODUCT_ID 0x1200

void print_usage() {
    printf("Usage: usbbutton_tool <command> [options]\n");
    printf("\nCommands:\n");
    printf("  --configure           Configure the button's behavior and colors.\n");
    printf("  --set-color           Set the button's current color.\n");
    printf("  --get-state           Get the button's pressed/released state.\n");
    printf("\nOptions for --configure:\n");
    printf("  --permanent           Make the configuration permanent (default is temporary).\n");
    printf("  --released-color R,G,B  Set the color when the button is released (e.g., 0,255,0).\n");
    printf("  --pressed-color R,G,B   Set the color when the button is pressed (e.g., 255,0,0).\n");
    printf("  --text \"some text\"      The text to be typed when the button is pressed.\n");
    printf("  --verbose             Print the raw data packets being sent.\n");
    printf("\nOptions for --set-color:\n");
    printf("  --color R,G,B           The RGB color to set (e.g., 0,0,255).\n");
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
        fprintf(stderr, "Error: Invalid color format. Use R,G,B (e.g., 255,0,0).\n");
        exit(1);
    }
}

void encode_text(const char *text, unsigned char *buffer) {
    for (int i = 0; i < 54; i++) {
        if (i < strlen(text)) {
            char c = toupper(text[i]);
            if (c >= 'A' && c <= 'Z') {
                buffer[i] = (unsigned char)(c - 'A' + 4);
            } else if (c == ' ') {
                buffer[i] = 0x2C;
            } else {
                buffer[i] = 0; // Unsupported characters are ignored
            }
        } else {
            buffer[i] = 0; // Pad with zeros
        }
    }
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
        fprintf(stderr, "Error: Unable to open device. Make sure it's connected and you have the right permissions.\n");
        hid_exit();
        return 1;
    }

    if (strcmp(argv[1], "--configure") == 0) {
        unsigned char config_data[62] = {0};
        unsigned char command = 0x51; // Temporary by default
        int verbose = 0;

        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "--permanent") == 0) {
                command = 0x50;
            } else if (strcmp(argv[i], "--released-color") == 0 && i + 1 < argc) {
                parse_color(argv[++i], &config_data[2], &config_data[3], &config_data[4]);
            } else if (strcmp(argv[i], "--pressed-color") == 0 && i + 1 < argc) {
                parse_color(argv[++i], &config_data[5], &config_data[6], &config_data[7]);
            } else if (strcmp(argv[i], "--text") == 0 && i + 1 < argc) {
                encode_text(argv[++i], &config_data[8]);
            } else if (strcmp(argv[i], "--verbose") == 0) {
                verbose = 1;
            }
        }

        unsigned char report_buf[65] = {0}; // Report ID (0) + data

        // Send command and first 2 bytes of data
        report_buf[0] = 0x0; // Report ID
        report_buf[1] = command;
        report_buf[2] = 0xdd;
        report_buf[3] = config_data[0];
        report_buf[4] = config_data[1];

        if (verbose) print_packet(report_buf, 5);
        if (hid_write(handle, report_buf, 5) == -1) {
            fprintf(stderr, "Error writing command to device.\n");
        } else {
            // Send the remaining 60 bytes in 15 chunks of 4
            for (int i = 0; i < 15; i++) {
                memcpy(&report_buf[1], &config_data[2 + i * 4], 4);
                if (verbose) print_packet(report_buf, 5);
                if (hid_write(handle, report_buf, 5) == -1) {
                    fprintf(stderr, "Error writing data packet %d to device.\n", i);
                    break;
                }
            }
            printf("Configuration sent successfully.\n");
        }

    } else if (strcmp(argv[1], "--set-color") == 0) {
        unsigned char r = 0, g = 0, b = 0;
        int verbose = 0;

        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "--color") == 0 && i + 1 < argc) {
                 parse_color(argv[++i], &r, &g, &b);
            } else if (strcmp(argv[i], "--verbose") == 0) {
                verbose = 1;
            }
        }

        unsigned char report_buf[65] = {0};
        report_buf[0] = 0x0; // Report ID
        report_buf[1] = 0x01;
        report_buf[2] = r;
        report_buf[3] = g;
        report_buf[4] = b;

        if (verbose) print_packet(report_buf, 5);
        if (hid_write(handle, report_buf, 5) == -1) {
            fprintf(stderr, "Error writing to device.\n");
        } else {
            printf("Color set successfully.\n");
        }

    } else if (strcmp(argv[1], "--get-state") == 0) {
        unsigned char report_buf[65] = {0};
        report_buf[0] = 0x0; // Report ID
        report_buf[1] = 0x02;

        if (hid_write(handle, report_buf, 5) == -1) {
            fprintf(stderr, "Error writing to device.\n");
        } else {
            unsigned char res[2]; // Increased buffer size for safety
            if (hid_read(handle, res, sizeof(res)) == -1) {
                fprintf(stderr, "Error reading from device.\n");
            } else {
                printf("Button state: %s\n", res[0] ? "Pressed" : "Released");
            }
        }
    } else {
        print_usage();
    }

    hid_close(handle);
    hid_exit();
    return 0;
}
