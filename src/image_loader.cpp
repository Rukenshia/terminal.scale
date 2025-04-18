#include "image_loader.h"

// Global pointer to the current image loader instance for use in callbacks
ImageLoader *currentImageLoader = nullptr;

// Constructor
ImageLoader::ImageLoader(TFT_eSPI &tftDisplay) : tft(tftDisplay)
{
    xPos = 0;
    yPos = 0;
}

// Initialize the file system
bool ImageLoader::begin()
{
    // First try to mount without formatting
    if (!LittleFS.begin(false))
    {
        Serial.println("LittleFS mount failed! Trying with formatting...");
        // If mounting fails, format and try again
        if (!LittleFS.begin(true))
        {
            Serial.println("LittleFS mount failed even after formatting!");
            return false;
        }
        Serial.println("LittleFS formatted and mounted successfully");
    }
    else
    {
        Serial.println("LittleFS mounted successfully");
    }

    // Check if our image files exist
    if (!checkImageFiles())
    {
        Serial.println("WARNING: Some image files are missing in LittleFS");
    }

    return true;
}

// Helper method to check if the required image files exist
bool ImageLoader::checkImageFiles()
{
    // Note: In LittleFS, files don't have a leading slash
    const char *requiredFiles[] = {"/up.png", "/dot.png", "/down.png"};
    bool allFilesExist = true;

    for (const char *filename : requiredFiles)
    {
        if (LittleFS.exists(filename))
        {
            fs::File file = LittleFS.open(filename, "r");
            if (file)
            {
                file.close();
            }
            else
            {
                Serial.printf("ERROR: File exists but cannot be opened: %s\n", filename);
                allFilesExist = false;
            }
        }
        else
        {
            Serial.printf("ERROR: File does not exist: %s\n", filename);
            allFilesExist = false;
        }
    }

    return allFilesExist;
}

// Draw a PNG image from LittleFS at the specified coordinates
bool ImageLoader::drawPNG(const char *filename, int16_t x, int16_t y)
{
    Serial.printf("Drawing PNG: %s at position (%d, %d)\n", filename, x, y);

    // Check if file exists first
    if (!LittleFS.exists(filename))
    {
        Serial.printf("ERROR: File does not exist: %s\n", filename);
        return false;
    }

    // Store the draw position for use by the callback
    xPos = x;
    yPos = y;

    // Set the global instance pointer to this instance for the callbacks
    currentImageLoader = this;

    // Open the PNG file using our callback functions
    int result = png.open(filename, openFile, closeFile, readFile, seekFile, pngDrawCallback);

    if (result != PNG_SUCCESS)
    {
        Serial.printf("PNG file open failed: %d\n", result);
        return false;
    }

    // Get dimensions of the image
    uint16_t width = png.getWidth();
    uint16_t height = png.getHeight();

    // Process and decode the PNG file
    result = png.decode(nullptr, 0);

    // Close the file when done
    png.close();

    // Reset the global pointer
    currentImageLoader = nullptr;

    if (result != PNG_SUCCESS)
    {
        Serial.printf("PNG decode failed: %d\n", result);
        return false;
    }

    return true;
}

// Get image dimensions without drawing
bool ImageLoader::getImageInfo(const char *filename, uint16_t &width, uint16_t &height)
{
    // Check if file exists first
    if (!LittleFS.exists(filename))
    {
        Serial.printf("ERROR: File does not exist: %s\n", filename);
        width = 0;
        height = 0;
        return false;
    }

    // Set the global instance pointer to this instance for the callbacks
    currentImageLoader = this;

    // Open the PNG file using our callback functions but don't render it
    int result = png.open(filename, openFile, closeFile, readFile, seekFile, nullptr);

    if (result != PNG_SUCCESS)
    {
        Serial.printf("PNG file open failed: %d\n", result);
        currentImageLoader = nullptr;
        width = 0;
        height = 0;
        return false;
    }

    // Get dimensions of the image
    width = png.getWidth();
    height = png.getHeight();

    // Close the file when done
    png.close();

    // Reset the global pointer
    currentImageLoader = nullptr;

    return true;
}

// List all PNG files in the data directory
void ImageLoader::listPNGFiles()
{
    Serial.println("PNG files in LittleFS:");

    fs::File root = LittleFS.open("/");
    if (!root || !root.isDirectory())
    {
        Serial.println("Failed to open root directory");
        return;
    }

    fs::File file = root.openNextFile();
    int count = 0;

    while (file)
    {
        const char *filename = file.name();
        size_t filenameLength = strlen(filename);

        // Check if this is a PNG file (ends with .png or .PNG)
        if (filenameLength > 4)
        {
            const char *extension = filename + filenameLength - 4;
            if (strcasecmp(extension, ".png") == 0)
            {
                Serial.printf("  %s - %d bytes\n", filename, file.size());
                count++;
            }
        }

        file = root.openNextFile();
    }

    if (count == 0)
    {
        Serial.println("No PNG files found");
    }

    root.close();
}

// Static callback function for PNG decoder
void ImageLoader::pngDrawCallback(PNGDRAW *pDraw)
{
    if (currentImageLoader)
    {
        currentImageLoader->drawPNGLine(pDraw);
    }
}

// Instance method to draw a line of the PNG image
void ImageLoader::drawPNGLine(PNGDRAW *pDraw)
{
    // Allocate a line buffer for the image line
    uint16_t lineBuffer[MAX_IMAGE_WIDTH];
    uint8_t maskBuffer[1 + MAX_IMAGE_WIDTH / 8];

    // Get a line of pixels from the PNG decoder
    png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0xffffffff);

    if (png.getAlphaMask(pDraw, maskBuffer, 255))
    {

        // Write the line to the TFT display
        tft.pushMaskedImage(xPos, yPos + pDraw->y, pDraw->iWidth, 1, lineBuffer, maskBuffer);
    }
}

// File system callback functions

// Open file callback
void *ImageLoader::openFile(const char *filename, int32_t *size)
{
    if (!currentImageLoader)
        return nullptr;

    // Try to open the file
    currentImageLoader->pngFile = LittleFS.open(filename, "r");

    if (!currentImageLoader->pngFile)
    {
        Serial.printf("Failed to open file: %s\n", filename);
        return nullptr;
    }

    // Get the file size
    if (size)
        *size = currentImageLoader->pngFile.size();

    return &currentImageLoader->pngFile;
}

// Close file callback
void ImageLoader::closeFile(void *handle)
{
    if (currentImageLoader && handle == &currentImageLoader->pngFile)
    {
        currentImageLoader->pngFile.close();
    }
}

// Read file callback
int32_t ImageLoader::readFile(PNGFILE *handle, uint8_t *buffer, int32_t length)
{
    if (!currentImageLoader)
        return 0;
    if (!handle || handle->fHandle == nullptr)
        return 0;

    fs::File *file = (fs::File *)handle->fHandle;
    return file->read(buffer, length);
}

// Seek file callback
int32_t ImageLoader::seekFile(PNGFILE *handle, int32_t position)
{
    if (!currentImageLoader)
        return 0;
    if (!handle || handle->fHandle == nullptr)
        return 0;

    fs::File *file = (fs::File *)handle->fHandle;
    return file->seek(position);
}