// addon/display/sh1106/sh1106device.cpp
#include "sh1106device.h"
#include "sh1106font_8x8.h"
#include <circle/util.h>
#include <circle/logger.h>
#include <circle/string.h>
#include "../../linux/kernel.h"

const int padding = 2;

// Constructor
CSH1106Device::CSH1106Device(CI2CMaster& i2c, u8 address)
    : m_I2CMaster(i2c), m_Address(address) {}


// Write a command to the SH1106
void CSH1106Device::WriteCommand(u8 command) {
    u8 buffer[2] = {0x00, command}; // 0x00 for command mode
    m_I2CMaster.Write(m_Address, buffer, sizeof(buffer));
}

// Write a data byte to the SH1106
void CSH1106Device::WriteData(u8 data) {
    u8 buffer[2] = {0x40, data}; // 0x40 for data mode
    m_I2CMaster.Write(m_Address, buffer, sizeof(buffer));
}

// Set the cursor position
void CSH1106Device::SetCursor(int x, int y) {
    WriteCommand(0xB0 + y); // Set page address
    WriteCommand(0x00 + (x & 0x0F)); // Lower nibble
    WriteCommand(0x10 + (x >> 4));    // Higher nibble
}

// Initialize the SH1106 display
bool CSH1106Device::Initialize() {
    static const u8 initCommands[] = {
        0xAE,       // Display OFF
        0xD5, 0x80, // Set Display Clock Divide Ratio/Oscillator Frequency
        0xA8, 0x3F, // Multiplex Ratio (1/64)
        0xD3, 0x00, // Display Offset
        0x40,       // Set Display Start Line
        0xAD, 0x8B, // Enable DC-DC Converter
        0xA1,       // Segment Remap
        0xC8,       // COM Output Scan Direction
        0xDA, 0x12, // COM Pins Hardware Configuration
        0x81, 0xCF, // Set Contrast Control
        0xD9, 0xF1, // Set Pre-charge Period
        0xDB, 0x40, // Set VCOM Deselect Level
        0xA4,       // Enable Display GDDR
        0xA6,       // Set Normal Display
        0xAF        // Display ON
    };

    for (u8 cmd : initCommands) {
        if (cmd == 0xAF) {
            // Before turning on the display, let's clear it first.
            Clear();
        }

        WriteCommand(cmd);
    }

    return true;
}

// Set the display ON/OFF
void CSH1106Device::SetDisplayOn(bool on) {
    // 0xAF = Display ON, 0xAE = Display OFF
    WriteCommand(on ? 0xAF : 0xAE);
}

// Set the display inversion
void CSH1106Device::SetInvertDisplay(bool invert) {
    // 0xA6 = Normal display, 0xA7 = Inverted display
    WriteCommand(invert ? 0xA7 : 0xA6);
}

/**
 * Start a horizontal scroll
 *
 * @param direction Scroll direction (true = left, false = right)
 * @param startPage Start page address
 * @param endPage   End page address
 */
void CSH1106Device::StartHorizontalScroll(bool direction, u8 startPage, u8 endPage) {
    WriteCommand(direction ? 0x27 : 0x26); // 0x27 = Left, 0x26 = Right
    WriteCommand(0x00);                    // Dummy byte
    WriteCommand(startPage);               // Start page address
    WriteCommand(0x00);                    // Time interval between each scroll step
    WriteCommand(endPage);                 // End page address
    WriteCommand(0x00);                    // Dummy byte
    WriteCommand(0xFF);                    // Dummy byte
    WriteCommand(0x2F);                    // Activate scrolling
}

void CSH1106Device::StartVerticalScroll(bool direction, u8 startRow, u8 endRow) {
    WriteCommand(direction ? 0x2A : 0x29); // 0x2A = Left, 0x29 = Right
    WriteCommand(0x00);                    // Dummy byte
    WriteCommand(startRow);                // Start row
    WriteCommand(endRow);                  // End row
    WriteCommand(0x01);                    // Scroll step (speed)
    WriteCommand(0x2F);                    // Activate scrolling
}

// Stop the scrolling
void CSH1106Device::StopScroll() {
    WriteCommand(0x2E); // Deactivate scrolling
}

// Clear the display
void CSH1106Device::Clear() {
    for (int page = 0; page < 8; ++page) {
        SetCursor(0, page);
        for (int col = 0; col < 132; ++col) {
            WriteData(0x00);
        }
    }
}

// Clear a single row
void CSH1106Device::ClearRow(int y) {
    // Ensure that y is a valid row.
    if (y < 0 || y >= 8) {
        return;
    }

    SetCursor(0, y);
    for (int col = 0; col < 132; ++col) {
        WriteData(0x00);
    }
}

// Calculate the width of a text string
int CalculateTextWidth(const char* text) {
    int width = 0;
    while (*text++) {
        width += 10; // 8 for character, 2 for padding
    }

    return width;
}

// Draw a single character
void CSH1106Device::DrawChar(char c, int x, int y) {
    if (c < ' ' || c > '~') {
        return; // Ignore unsupported characters
    }

    // Add the 2-pixel offset to the X position
    int adjustedX = x + padding;
    // Check if the character is within the display bounds
    if (adjustedX >= 128 || y < 0 || y >= 8) {
        return; // Skip if the character is out of bounds
    }

    // Set the starting cursor position
    SetCursor(adjustedX, y);
    const u8* bitmap = CSH1106DeviceFont8x8[c - ' '];

    // Write the character to the display
    for (int col = 0; col < 8; ++col) {
        int currentX = adjustedX + col;
        if (currentX >= 0 && currentX < 128) {
            // Only write within visible bounds
            WriteData(bitmap[col]);
        }
    }
}

// Print text at a given position
void CSH1106Device::PrintText(const char* text, int x, int y) {
int cursorX = x;
    int cursorY = y;

    while (*text) {
        ClipAndWrap(cursorX, cursorY);

        // Stop if we've reached the bottom of the screen
        if (cursorY >= 8) {
            break;
        }

        DrawChar(*text++, cursorX, cursorY);
        cursorX += 10; // Move to the next position
    }
}

// Print centered text (vertically and horizontally)
void CSH1106Device::PrintCenteredText(const char* text) {
    int textWidth = CalculateTextWidth(text);
     // Center alignment
    int x = (128 - textWidth) / 2;
    // Center vertically at the 4th page (range: 0-7)
    int y = 3;

    PrintText(text, x, y);
}

// Set the display contrast level
void CSH1106Device::SetContrast(u8 contrast) {
    WriteCommand(0x81); // Set Contrast Control
    WriteCommand(contrast);
}

void CSH1106Device::ClipAndWrap(int& x, int& y) {
    if (x + 8 + 2 > 128) {
        // 8 for character width, 2 for padding
        x = 0;            // Move to the start of the next line
        ++y;              // Advance to the next row (page)
    }

    if (y >= 8) {
        y = 7;            // Clamp to the bottom of the screen
    }
}

void CSH1106Device::UpdateBuffer(int page, int col, u8 value) {
    if (page >= 0 && page < 8 && col >= 0 && col < 132) {
        frameBuffer[page][col] = value;
    }
}

void CSH1106Device::RefreshDisplay() {
    for (int page = 0; page < 8; ++page) {
        SetCursor(0, page);
        for (int col = 0; col < 132; ++col) {
            WriteData(frameBuffer[page][col]);
        }
    }
}

// Refresh a single page
void CSH1106Device::RefreshPage(int page) {
    if (page < 0 || page >= 8) return;

    SetCursor(0, page);
    for (int col = 0; col < 132; ++col) {
        WriteData(frameBuffer[page][col]);
    }
}

// Draw a horizontal line
void CSH1106Device::DrawVerticalLine(int x, int yStart, int height) {
    if (x < 0 || x >= 128) return;

    for (int y = yStart; y < yStart + height && y < 8; ++y) {
        UpdateBuffer(y, x + 2, 0xFF); // Use the frame buffer
    }
    RefreshDisplay();
}


// Draw a horizontal line
void CSH1106Device::DrawHorizontalLine(int x, int y, int length) {
    if (y < 0 || y >= 8) {
        return; // Ensure within valid page
    }

    SetCursor(x, y);

    for (int i = 0; i < length && (x + i) < 128; ++i) {
        WriteData(0xFF); // Draw a solid line
    }
}

void CSH1106Device::PrintFrameBufferToConsole() {
    CString logMessage;
    for (int page = 0; page < 8; ++page) {
        for (int col = 0; col < 132; ++col) {
            CString formatted;
            formatted.Format("%02X ", frameBuffer[page][col]);
            logMessage.Append(formatted);
        }

        logMessage.Append("\n");
    }

    CLogger::Get()->Write("SH1106", LogNotice, logMessage);
}