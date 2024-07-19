#include <WiFi.h>
#include <esp_wifi.h>
#include <ESP_I2S.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "esp_camera.h"
#include "camera_pins.h"

#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM

I2SClass I2S;

const char* ssid = "************";
const char* password = "***********";

I2SClass I2S;
/*Declaration function*/
void print_wakeup_reason();//print wake_up source 
void SDCard_enable();//SDCard begin 
void Microphone_enable();//Microphone begin 
void Camera_enable();//camera init 
void wifi_enable();//
void Deep_Sleep_enable();//start deep_sleep
void close_SDCard();
void close_Camera();
void close_Microphone();
void startCameraServer();
void setupLedFlash(int pin);



void setup() {
  Serial.begin(115200);
  while(!Serial);
}

void loop() {

  cameraconfig();

  delay(700);
  esp_camera_deinit();
  Serial.println("the camera is close");
  delay(500);
  Serial.println("ready into deep sleep");
  esp_deep_sleep_start();
  Serial.flush();
  delay(5000);
  // SD.end();
  // I2S.end();
  // Serial.println("the sd camera microphone is close")
}


void Camera_enable(){
   Serial.setDebugOutput(true);
  Serial.println();

/*config设置*/
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
  config.pixel_format = PIXFORMAT_JPEG;  // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;


  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if (config.pixel_format == PIXFORMAT_JPEG) {
    if (psramFound()) {
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);        // flip it back
    s->set_brightness(s, 1);   // up the brightness just a bit
    s->set_saturation(s, -2);  // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  if (config.pixel_format == PIXFORMAT_JPEG) {
    s->set_framesize(s, FRAMESIZE_QVGA);
  }


#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif
#if defined(CAMERA_MODEL_ESP32S3_EYE)
  s->set_vflip(s, 1);
#endif
// Setup LED FLash if LED pin is defined in camera_pins.h
#if defined(LED_GPIO_NUM)
  setupLedFlash(LED_GPIO_NUM);
#endif

  Serial.println("the camera open");
}

void SDCard_Function(){
    if(!SD.begin(21)){
      Serial.println("Card Mount Failed");
      return;
  }
  uint8_t cardType = SD.cardType();
  if(cardType == CARD_NONE){
      Serial.println("No SD card attached");
      return;
  }
  Serial.print("SD Card Type: ");
  if(cardType == CARD_MMC){
      Serial.println("MMC");
  } else if(cardType == CARD_SD){
      Serial.println("SDSC");
  } else if(cardType == CARD_SDHC){
      Serial.println("SDHC");
  } else {
      Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  Serial.println("the sc card is connecting");
}

void Microphone_Function(){
    // setup 42 PDM clock and 41 PDM data pins
  I2S.setPinsPdmRx(42, 41);
  // // start I2S at 16 kHz with 16-bits per sample
  if (!I2S.begin(I2S_MODE_PDM_RX, 16000, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO)) {
    Serial.println("Failed to initialize I2S!");
    while (1); // do nothing
  }
  Serial.println("microphone is open");
  // Stop reading after 5 seconds
  delay(500);
}

void wifi_enable(){
  WiFi.mode(WIFI_STA);//set wifi client mode
  esp_wifi_start();//enalbe wifi peripheral
  Serial.println("Wifi open!");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.println("address ip is : ");
  Serial.print(WiFi.localIP());
}

void Modem_Sleep_enalbe(){
  //close wifi 
  WiFi.mode(WIFI_OFF);

  //check whether or not
  if (WiFi.getMode() == WIFI_OFF) {
    Serial.println("WiFi is off");
  } else {
    Serial.println("WiFi is still on");
  }
}

void close_SDCard(){
  SD.end();
  Serial.println("the sd card already closed");
}
void close_Camera(){
  esp_camera_deinit();
  Serial.println("the camera already closed");
}
void close_Microphone(){
  I2S.end();
  Serial.println("the microphone already closed");
}
