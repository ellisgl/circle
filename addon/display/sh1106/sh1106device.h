// addon/display/sh1106/sh1106device.h
#ifndef SH1106DEVICE_H
#define SH1106DEVICE_H

#include <circle/i2cmaster.h>
#include <circle/util.h>
#include <circle/types.h>
#include <stdint.h>


class CSH1106Device {
public:
    // Constructor
    CSH1106Device(CI2CMaster& i2c, u8 address = 0x3C);

    // Write a single command
    void WriteCommand(u8 command);

    // Write a single data byte
    void WriteData(u8 data);

    // Set cursor position on the display
    void SetCursor(int x, int y);

    // Initialize the display
    bool Initialize();

    // Clear the display
    void Clear();

    // Draw a single character at a specific position (x, y)
    void DrawChar(char c, int x, int y);

    // Print a string at a specific position (x, y)
    void PrintText(const char* text, int x, int y);

    void PrintCenteredText(const char* text);

    void ClearRow(int y);

    // Set the display contrast level
    void SetContrast(u8 contrast);

    void SetDisplayOn(bool on);

    void SetInvertDisplay(bool invert);

    void StartHorizontalScroll(bool direction, u8 startPage, u8 endPage);

    void StartVerticalScroll(bool direction, u8 startRow, u8 endRow);

    void StopScroll();

    void UpdateBuffer(int page, int col, u8 value);

    void RefreshDisplay();

    void RefreshPage(int page);

    void DrawHorizontalLine(int x, int y, int width);

    void DrawVerticalLine(int x, int y, int height);

    void PrintFrameBufferToConsole();


private:
    CI2CMaster& m_I2CMaster;   // Reference to the I2C master
    u8 m_Address;         // I2C address of the SH1106 display

    int CalculateTextWidth(const char* text);


    void ClipAndWrap(int &x, int &y);

    // 8x8 font array for ASCII characters
    static const u8 Font8x8[][8];

    // 8 pages, 132 columns
    u8 frameBuffer[8][132];
};

#endif // SH1106DEVICE_H
