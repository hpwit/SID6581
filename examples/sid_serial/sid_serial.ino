#include "SID6581.h"
#define SID_CLOCK 25
#define SID_DATA 33
#define SID_LATCH 27
#define LIMIT 8000
#include "freertos/queue.h"
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"

static QueueHandle_t _serial_sid_queue;
static TaskHandle_t SIDSerialPlayerTaskHandle = NULL;
static TaskHandle_t SIDSerialPlayerTaskLock= NULL;
volatile uint32_t buffer_size=0;

struct serial_command {
  uint8_t data;
  uint8_t address;
  uint16_t duration;
};


void SIDSerialPlayerTask(void * parameters) {
      SIDSerialPlayerTaskLock  = xTaskGetCurrentTaskHandle();
  for(;;) {
    serial_command element;
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    Serial.println("We Start");
    while(1) {
      xQueueReceive(_serial_sid_queue, &element, portMAX_DELAY);
      sid.pushRegister(element.address/32,(element.address)%32,element.data);
      delayMicroseconds(element.duration);
      sid.feedTheDog();
    }
  }
}


void setup() {
  // put your setup code here, to run once:
 Serial.begin(115200);
  //Serial1.begin(115200,SERIAL_8N1, 34);
  sid.begin(SID_CLOCK,SID_DATA,SID_LATCH);
  _serial_sid_queue= xQueueCreate( 26000, sizeof( serial_command ) );
  xTaskCreate(
    SIDSerialPlayerTask,      /* Function that implements the task. */
    "NAME1",          /* Text name for the task. */
    2048,      /* Stack size in words, not bytes. */
    ( void * ) 1,    /* Parameter passed into the task. */
    tskIDLE_PRIORITY,/* Priority at which the task is created. */
    & SIDSerialPlayerTaskHandle);
  Serial.println("waiting for serial input");
}


int cmd=0;

void loop() {
  serial_command c;
  for(;;) {
    if(Serial.available()) { //if user typed something through the USB...
      uint8_t received=Serial.read();
      //Serial.println(received);
      switch(cmd) {
        case 0:
          c.address=received;
        break;
        case 1:
          c.data=received;
        break;
        case 2:
          c.duration=received;
        break;
        case 3:
          c.duration+=(received*256);
          if(buffer_size<LIMIT) {
            buffer_size++;
          }
          xQueueSend(_serial_sid_queue, &c, portMAX_DELAY);
          if(buffer_size == LIMIT) {
          Serial.println("Buffer full we start playing");
            xTaskNotifyGive(SIDSerialPlayerTaskLock);
            buffer_size++;
          }
        break;
      }
      cmd=(cmd+1)%4;
    }
  }
}
