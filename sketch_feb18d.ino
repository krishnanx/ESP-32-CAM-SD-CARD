#define CAMERA_MODEL_AI_THINKER // Has PSRAM
#include "esp_camera.h"
#include "FS.h"
#include "SD_MMC.h"
#include "camera_pins.h"
#define SD_CS_PIN       5
#include <rom/rtc.h>
File videoFile;
int frameCounter = 0;
__attribute__((section(".stack")))
uint8_t stack[4096 * 2];

void shutdownHandler() {
  // This function is called before the device resets or shuts down
  Serial.println("Shutdown handler called. Closing the video file.");
  videoFile.close();
  Serial.println("Video file closed");
  delay(500);  // Add a small delay to ensure the file is closed before shutdown
}
void createNewVideoFile() {
  // Generate a random name for the new video file
  char fileName[30];  // Adjust the size based on your requirements
  sprintf(fileName, "/sdcard/videos/video_%lu.avi", millis());

  // Close the existing file if it's still open
  if (videoFile) {
    videoFile.close();
    Serial.println("Closed existing file before creating a new one.");
  }

  // Create and open the new video file
  videoFile = SD_MMC.open(fileName, FILE_WRITE);

  if (!videoFile) {
    Serial.println("Error opening new video file");
    return;
  }

  frameCounter = 0;  // Reset frame counter for the new file
  Serial.println("New video file created successfully.");
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  // Initialize the camera
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG; // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_LATEST;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  

  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 12;
    config.fb_count = 2;
  } else {
    Serial.println("PSRAM not found. Using FRAMESIZE_SVGA for reliability.");
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // Initialize SD card
  if (!SD_MMC.begin("/sdcard", true)) {
    Serial.println("Card Mount Failed");
    return;
  }

  /*// Generate a random name for the video file
  char fileName[30];  // Adjust the size based on your requirements
  sprintf(fileName, "/sdcard/videos/video_%lu.avi", millis());

  // Create and open the video file
  videoFile = SD_MMC.open(fileName, FILE_WRITE);

  if (!videoFile) {
    Serial.println("Error opening video file");
    return;
  }*/
  createNewVideoFile();  // Create the initial video file
  esp_register_shutdown_handler(&shutdownHandler);
  // Write the AVI header
  writeAVIHeader();
}
int Times=0;
void loop() {
  // Capture a frame
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }
  Serial.println("Picture taken");

  // Write the frame to the video file
  size_t bytesWritten = videoFile.write(fb->buf, fb->len);
  if (bytesWritten != fb->len) {
    Serial.println("Error writing frame to video file");
  } else {
    Serial.println("Frame written to video file");
  }

  // Return the frame buffer to the camera driver
  esp_camera_fb_return(fb);
  Times=Times + 1;
  Serial.println(Times);
  frameCounter++;
  // Check if it's time to close the current file and create a new one
  if (frameCounter >= 1800) {
    Serial.println("Closing file.......");
    videoFile.close();      // Close the current file
    Serial.println("Creating a new file......");
    createNewVideoFile();   // Create a new file
  }
  
  // Delay between frames (adjust as needed)
  delay(33);
  /*if (Times == 1800 ){
    Serial.println("File is closing now....");
    videoFile.close();
  }*/
}

void writeAVIHeader() {
 // AVI Header structure
  struct AviHeader {
    // RIFF chunk
    uint32_t riff_id;
    uint32_t riff_size;
    uint32_t avi_id;

    // LIST chunk (header)
    uint32_t hdrl_id;
    uint32_t hdrl_size;
    uint32_t avih_id;
    uint32_t avih_size;

    // AVI Main Header
    uint32_t dwMicroSecPerFrame;
    uint32_t dwMaxBytesPerSec;
    uint32_t dwPaddingGranularity;
    uint32_t dwFlags;
    uint32_t dwTotalFrames;
    uint32_t dwInitialFrames;
    uint32_t dwStreams;
    uint32_t dwSuggestedBufferSize;  // Removed redeclaration
    uint32_t dwWidth;
    uint32_t dwHeight;
    uint32_t dwReserved[4];

    // LIST chunk (stream)
    uint32_t strl_id;
    uint32_t strl_size;
    uint32_t strh_id;
    uint32_t strh_size;

    // AVI Stream Header
    uint32_t fccType;
    uint32_t fccHandler;
    uint32_t dwScale;
    uint32_t dwRate;
    uint32_t dwStart;
    uint32_t dwLength;
    //uint32_t dwSuggestedBufferSize;  // Removed redeclaration
    uint32_t dwQuality;
    uint32_t dwSampleSize;

    // Bitmap Info Header
    uint32_t biSize;
    uint32_t biWidth;
    uint32_t biHeight;
    uint32_t biPlanes;
    uint32_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    uint32_t biXPelsPerMeter;
    uint32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;

    // End of AVI header
  };

  // Define the FOURCC macro for creating four-character codes
  #ifndef FOURCC
  #define FOURCC(a, b, c, d) ((uint32_t)(a) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))
  #endif

  // AVI Header data
  AviHeader aviHeader = {
    .riff_id = FOURCC('R', 'I', 'F', 'F'),
    .riff_size = 0,  // To be filled later
    .avi_id = FOURCC('A', 'V', 'I', ' '),
    .hdrl_id = FOURCC('L', 'I', 'S', 'T'),
    .hdrl_size = sizeof(uint32_t) + sizeof(aviHeader.avih_id) + aviHeader.avih_size,
    .avih_id = FOURCC('a', 'v', 'i', 'h'),
    .avih_size = 56,
    .dwMicroSecPerFrame = 33333,  // Adjust based on your frame rate (15 fps in this case)
    .dwMaxBytesPerSec = 2500000,  // Adjust based on your desired bitrate
    .dwPaddingGranularity = 0,
    .dwFlags = 0x10,  // AVIF_TRUSTCKTYPE
    .dwTotalFrames = 36000,  // To be filled later
    .dwInitialFrames = 0,
    .dwStreams = 1,
    .dwSuggestedBufferSize = 640 * 480 * 3,  // To be filled later
    .dwWidth = 640,  // Adjust based on your video resolution
    .dwHeight = 480,  // Adjust based on your video resolution
    .dwReserved = {0},
  };

  // Set the file size fields
 // Calculate riff_size
  aviHeader.riff_size = sizeof(aviHeader) - 2 * sizeof(uint32_t) + aviHeader.hdrl_size + sizeof(uint32_t);  // Assuming hdrl_size includes the size of LIST header
  //aviHeader.hdrl_size = sizeof(uint32_t) + sizeof(aviHeader.avih_id) + aviHeader.avih_size;
  // Assuming aviHeader.avih_size is 56, and the aviHeader.avih_id is also 4 bytes
  //aviHeader.dwTotalFrames = 0xFFFFFFFF;  // Unknown total frames
  //aviHeader.dwSuggestedBufferSize = aviHeader.dwWidth * aviHeader.dwHeight * 3;  // Assuming 3 bytes per pixel for JPEG
  // Seek to the beginning of the file
  videoFile.seek(0);

  // Write the AVI header to the file
  videoFile.write((uint8_t*)&aviHeader, sizeof(aviHeader));
}
