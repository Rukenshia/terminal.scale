#ifndef IMAGE_LOADER_H
#define IMAGE_LOADER_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <PNGdec.h>
#include <LittleFS.h>
#include <FS.h>

// Maximum image width constant
#define MAX_IMAGE_WIDTH 320 // Adjust based on your display's maximum width

class ImageLoader
{
public:
    ImageLoader(TFT_eSPI &tftDisplay);

    // Initialize the file system
    bool begin();

    // Draw a PNG image from LittleFS at the specified coordinates
    bool drawPNG(const char *filename, int16_t x, int16_t y);

    // Get image dimensions without drawing
    bool getImageInfo(const char *filename, uint16_t &width, uint16_t &height);

    // List all PNG files in the data directory
    void listPNGFiles();

private:
    // Reference to the TFT display
    TFT_eSPI &tft;

    // PNG decoder instance
    PNG png;

    // File handle for PNG file access
    fs::File pngFile;

    // Draw coordinates for the image
    int16_t xPos, yPos;

    // Helper method to check if required image files exist
    bool checkImageFiles();

    // Callback for PNG decoding (static, will call instance method)
    static void pngDrawCallback(PNGDRAW *pDraw);

    // The actual draw function (instance method)
    void drawPNGLine(PNGDRAW *pDraw);

    // File access callbacks for PNGdec library
    static void *openFile(const char *filename, int32_t *size);
    static void closeFile(void *handle);
    static int32_t readFile(PNGFILE *handle, uint8_t *buffer, int32_t length);
    static int32_t seekFile(PNGFILE *handle, int32_t position);

    // Friend function to access private members from static callbacks
    friend void pngDrawCallbackWrapper(PNGDRAW *pDraw);
};

// Global instance that will be set during drawing operations
extern ImageLoader *currentImageLoader;

#endif // IMAGE_LOADER_H