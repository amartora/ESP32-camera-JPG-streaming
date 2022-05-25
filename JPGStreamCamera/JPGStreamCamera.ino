//TTGO T-CAMERA
//Board: "ESP32 Wrover Module"

#include <esp_event_loop.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <string.h>

#include "camera_config.h"

#include <WiFi.h>
#include <WiFiUdp.h>

#define PACKET_SIZE 1403
#define DATA_SIZE 1400

WiFiUDP Udp;

const char *ssid = "xxxxx";
const char *password = "xxxxx";

const char * udpAddress = "xxx.xxx.xxx.xxx";
const uint16_t udpPort = xxxx;

uint8_t packet[PACKET_SIZE]; //1403 byte packet 1 byte ID, 1 byte frag #, 1 byte total frags, 1400 jpg bytes

uint8_t totalFrags = 0;
uint8_t fragNum = 0;
uint8_t ID = 0;

static esp_err_t init_camera()
{
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK)
    {
        Serial.println("Camera Init Failed");
        return err;
    }
    return ESP_OK;
}

void setup() {

  psramInit();
  init_camera();
  Serial.begin(115200);

  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.print(" connected at");
  Serial.println (WiFi.localIP());

  Udp.begin(WiFi.localIP(),udpPort);

}

void loop() {
 
  //Serial.println ("Taking picture...");
  camera_fb_t *fb = esp_camera_fb_get();
  
  Serial.print("Picture taken! Its size was: ");
  Serial.print(fb->len);
  Serial.println("bytes");
 
  totalFrags = ((fb->len) / DATA_SIZE); //if its exactly 1400 bytes this wont work... fix later

  for (int i = 0; i < totalFrags + 1; i++){ //totalFrags + 1 because totalFrags starts at 0

    uint16_t remainder = ((fb->len) % DATA_SIZE);
    uint16_t emptyBytes = DATA_SIZE - remainder;
    uint8_t emptyBytesOn = 0;

    fragNum = i;
    
    if (totalFrags == fragNum){
      emptyBytesOn = 1;
    }
    
    packet[0] = ID;
    packet[1] = fragNum;
    packet[2] = totalFrags;
    
    memcpy(&packet[3], fb->buf + (fragNum * DATA_SIZE), DATA_SIZE); 
    
    Udp.beginPacket(udpAddress,udpPort);
    Udp.write(packet, PACKET_SIZE - (emptyBytes * emptyBytesOn));
    Udp.endPacket();
    
    memset(packet, 0, sizeof(packet)); //clear the array by setting all to 0
    
    delay(10);
  }
    ID++;
    delay(200); //seems like the receiver takes about 200ms to render jpegs, so I made this delay 200ms
}
  
