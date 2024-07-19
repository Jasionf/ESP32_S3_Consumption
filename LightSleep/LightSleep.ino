#include<esp_sleep.h>
#include<esp_timer.h>
#include<esp_check.h>


/*时钟源*/
#define TIMER_WAKEUP_TIME_US (7 * 1000 * 1000)
RTC_DATA_ATTR int wakeup_count = 0;
static const char *TAG = "timer_wakeup";


/*GPIO高低电平*/
// static const char *TAG = "gpio_wakeup";
// #define BOOT_BUTTON_NUM 0
// #define GPIO_WAKEUP_NUM BOOT_BUTTON_NUM
// #define GPIO_WAKEUP_LEVEL 0


void setup() {
  Serial.begin(115200);
  delay(1000); //确保串口充分打印
}

void loop() {
   LightSleep_GPIO();
   delay(500);
}


void wait_gpio_inactive(){
  Serial.println("waiting for GPIO " + String(GPIO_WAKEUP_NUM) + " to go high");
  while(digitalRead(GPIO_WAKEUP_NUM) == GPIO_WAKEUP_LEVEL){
  Serial.println("the " + String(GPIO_WAKEUP_NUM) + "=" + String(GPIO_WAKEUP_LEVEL));
  }
}


void LightSleep_GPIO(){
   // 设置 ESP32 进入轻度睡眠
  
    // 设置引脚
    pinMode(GPIO_WAKEUP_NUM,GPIO_MODE_INPUT);
    Serial.println("the pin is config complete");
    if(example_register_gpio_wakeup() == ESP_OK){
    Serial.println("wake up ready");
    }
    Serial.println("ready into lightsleep");
    Serial.flush();
    esp_light_sleep_start();
  

    //打印唤醒源
    const char* wakeup_reason;
    switch (esp_sleep_get_wakeup_cause()) {
      case ESP_SLEEP_WAKEUP_TIMER:
        wakeup_reason = "timer";
        break;
      case ESP_SLEEP_WAKEUP_GPIO:
        wakeup_reason = "pin";
        break;
      case ESP_SLEEP_WAKEUP_UART:
        wakeup_reason = "uart";
        break;
      default:
        wakeup_reason = "unknown";
        break;}

    wakeup_count++;

    while(!Serial);
    Serial.println("wake up");
}
esp_err_t example_register_gpio_wakeup(void)
{
    /*Enable wake up from GPIO */
    ESP_RETURN_ON_ERROR(gpio_wakeup_enable(GPIO_WAKEUP_NUM, GPIO_WAKEUP_LEVEL == 0 ? GPIO_INTR_LOW_LEVEL : GPIO_INTR_HIGH_LEVEL),TAG, "Enable gpio wakeup failed");
    ESP_RETURN_ON_ERROR(esp_sleep_enable_gpio_wakeup(), TAG, "Configure gpio as wakeup source failed");

    /* Make sure the GPIO is inactive and it won't trigger wakeup immediately */
    wait_gpio_inactive();
    ESP_LOGI(TAG, "gpio wakeup source is ready");

    return ESP_OK;
}

esp_err_t example_register_timer_wakeup(){
    ESP_RETURN_ON_ERROR(esp_sleep_enable_timer_wakeup(TIMER_WAKEUP_TIME_US), TAG, "Configure timer as wakeup source failed");
    ESP_LOGI(TAG, "timer wakeup source is ready");
    return ESP_OK;
}

void LightSleep_Timer() {
    // 设置 ESP32 进入轻度睡眠
    esp_sleep_enable_timer_wakeup(TIMER_WAKEUP_TIME_US); // 设置定时器唤醒
    //判断定时器是否设置成功
    if(example_register_timer_wakeup() == ESP_OK){

    Serial.println("the timer is set complete");
    Serial.println("ready into light sleep");
    Serial.flush();//缓存

    esp_light_sleep_start(); // 进入轻度睡眠
    Serial.println("wake up !");
    }

    //醒来重新打开串口，可有可无，测试
    Serial.begin(115200);

    //打印唤醒源
    const char* wakeup_reason;
    switch (esp_sleep_get_wakeup_cause()) {
      case ESP_SLEEP_WAKEUP_TIMER:
        wakeup_reason = "timer";
        break;
      case ESP_SLEEP_WAKEUP_GPIO:
        wakeup_reason = "pin";
        break;
      case ESP_SLEEP_WAKEUP_UART:
        wakeup_reason = "uart";
        break;
      default:
        wakeup_reason = "unknown";
        break;}

    wakeup_count++;

    while(!Serial);

    Serial.printf("Wakeup #%d, reason: %s, slept for %d ms\n", wakeup_count, wakeup_reason);
}
    
