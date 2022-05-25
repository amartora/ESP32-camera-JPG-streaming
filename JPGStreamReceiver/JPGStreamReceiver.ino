//TTGO T-WATCH 2020 V1
//Board: "ESP32 Dev Module"

#include <WiFi.h>
#include <JPEGDecoder.h>
#include <AsyncUDP.h>
#include "config.h"

#define MAX_PACKET_LEN 1403 //bytes. 1 byte ID, 1 byte fragment number, 1 byte total fragment number
#define DATA_LEN 1400
#define MAX_JPEG_LEN 10000 

#define WIFI_SSID "xxxxx"
#define WIFI_PASS "xxxxx"
#define UDP_PORT xxxx
char UDP_ADDRESS[] = "xxx.xxx.xxx.xxx";

AsyncUDP udp;
TTGOClass *ttgo;

TaskHandle_t wifiTask;
TaskHandle_t screenTask;

#define minimum(a,b)     (((a) < (b)) ? (a) : (b)) //for Bodmer's function

void renderJPEG(int xpos, int ypos);

//char *jpgArray = (char*)malloc(sizeof(char)*MAX_JPEG_LEN);
uint8_t *jpgArray = (uint8_t*)malloc(sizeof(uint8_t)*MAX_JPEG_LEN);

  uint8_t ID = 255;
  uint8_t currID = 0;
  uint8_t fragNum = 0;
  uint8_t maxFrags = 0;
  bool packetDoneFlag = 1; //should optimize these variables to not all be global later
  bool jpgDone = 0;
  int totalJPGBytes = 0;

void setup() {

  ttgo = TTGOClass::getWatch();
  ttgo->begin();  
  ttgo->openBL();
  Serial.begin(115200); 

  xTaskCreatePinnedToCore(
                    wifiTaskCode,   /* Task function. */
                    "wifiTask",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &wifiTask,      /* Task handle to keep track of created task */
                    0);          /* pin task to core 0 */                  
  delay(500); 

  xTaskCreatePinnedToCore(
                    screenTaskCode,   /* Task function. */
                    "screenTask",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &screenTask,      /* Task handle to keep track of created task */
                    1);          /* pin task to core 1 */
  delay(500); 
}


void wifiTaskCode( void * pvParameters ){
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to ");
  Serial.print(WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());

  if(udp.listen(UDP_PORT)) {
    Serial.println("UDP connected");
    udp.onPacket([](AsyncUDPPacket packet){ //defining the response to a UDP packet being recieved here
          
      currID = *(packet.data()); //data() is a pointer pointing to a uint8_t, so I derefernce here with *
      fragNum = *(packet.data()+1);
      maxFrags = *(packet.data()+2);
  
      if (packetDoneFlag == 1){ //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@IF WE ARE STARTING FRESH ON A NEW PACKET
        if (fragNum == 0 && maxFrags == 0){//if jpg is in one packet entirely
          memcpy(jpgArray, packet.data()+3, packet.length()-3);// copying data starting from arg B to arg A of size arg C
        }
        else if (fragNum == 0 && maxFrags > 0){//if this is first fragment out of multiple
          memcpy(jpgArray, packet.data()+3, packet.length()-3);// copying data starting from arg B to arg A of size arg C
          ID = currID;
          packetDoneFlag = 0;
        }
        else if (fragNum != 0){//we aren't starting on first frag, skip this frag and try again
            Serial.println("lost frag");
        }
      }     
      else{ //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@IF WE ARE CONTINUING A PACKET
        if (currID == ID){//only continue packet if IDs match
          
          memcpy(jpgArray + (DATA_LEN * fragNum), packet.data()+3, packet.length()-3);// copying data starting from arg B to arg A of size arg C
          
          if (fragNum == maxFrags){
            jpgDone = 1;
            packetDoneFlag = 1;
            totalJPGBytes = (DATA_LEN * maxFrags) + (packet.length()-3);
            Serial.print("PG bytes recieved: ");
            Serial.println(totalJPGBytes);
          }
        }
        else{//if IDs don't match, packet is lost, just go onto next packet
          packetDoneFlag = 1;
          Serial.println("lost frag");
        }
      }
    });
  }
  
  for(;;){
    vTaskDelay(10);
  } 
  
}

void screenTaskCode( void * pvParameters ){ 
  
  for(;;){
    if(jpgDone == 1){
      
      Serial.println("JPEG start");
      
      JpegDec.decodeArray(jpgArray, totalJPGBytes); //array (derefrenece form pointer here), arraysize
      
      renderJPEG(0, 40); //x and y offset

      jpgDone = 0;
        
    }
    else{
      vTaskDelay(10);
    }
  }
}

void loop() {
  
}

void renderJPEG(int xpos, int ypos) { //from Bodmer's JPEGDecoder example

  // retrieve infomration about the image
  uint16_t *pImg;
  uint16_t mcu_w = JpegDec.MCUWidth;
  uint16_t mcu_h = JpegDec.MCUHeight;
  uint32_t max_x = JpegDec.width;
  uint32_t max_y = JpegDec.height;

  // Jpeg images are draw as a set of image block (tiles) called Minimum Coding Units (MCUs)
  // Typically these MCUs are 16x16 pixel blocks
  // Determine the width and height of the right and bottom edge image blocks
  uint32_t min_w = minimum(mcu_w, max_x % mcu_w);
  uint32_t min_h = minimum(mcu_h, max_y % mcu_h);

  // save the current image block size
  uint32_t win_w = mcu_w;
  uint32_t win_h = mcu_h;

  // record the current time so we can measure how long it takes to draw an image
  uint32_t drawTime = millis();

  // save the coordinate of the right and bottom edges to assist image cropping
  // to the screen size
  max_x += xpos;
  max_y += ypos;

  // read each MCU block until there are no more
  while (JpegDec.read()) {
    
    // save a pointer to the image block
    pImg = JpegDec.pImage ;

    // calculate where the image block should be drawn on the screen
    int mcu_x = JpegDec.MCUx * mcu_w + xpos;  // Calculate coordinates of top left corner of current MCU
    int mcu_y = JpegDec.MCUy * mcu_h + ypos;

    // check if the image block size needs to be changed for the right edge
    if (mcu_x + mcu_w <= max_x) win_w = mcu_w;
    else win_w = min_w;

    // check if the image block size needs to be changed for the bottom edge
    if (mcu_y + mcu_h <= max_y) win_h = mcu_h;
    else win_h = min_h;

    // copy pixels into a contiguous block
    if (win_w != mcu_w)
    {
      uint16_t *cImg;
      int p = 0;
      cImg = pImg + win_w;
      for (int h = 1; h < win_h; h++)
      {
        p += mcu_w;
        for (int w = 0; w < win_w; w++)
        {
          *cImg = *(pImg + w + p);
          cImg++;
        }
      }
    }

    // calculate how many pixels must be drawn
    uint32_t mcu_pixels = win_w * win_h;

    ttgo->tft->startWrite();

    // draw image MCU block only if it will fit on the screen
    if (( mcu_x + win_w ) <= ttgo->tft->width() && ( mcu_y + win_h ) <= ttgo->tft->height())
    {

      // Now set a MCU bounding window on the TFT to push pixels into (x, y, x + width - 1, y + height - 1)
      ttgo->tft->setAddrWindow(mcu_x, mcu_y, win_w, win_h);

      // Write all MCU pixels to the TFT window
      while (mcu_pixels--) {
        // Push each pixel to the TFT MCU area
        ttgo->tft->pushColor(*pImg++);
      }

    }
    else if ( (mcu_y + win_h) >= ttgo->tft->height()) JpegDec.abort(); // Image has run off bottom of screen so abort decoding

    ttgo->tft->endWrite();
  }

  // calculate how long it took to draw the image
  drawTime = millis() - drawTime;

  // print the results to the serial port
  Serial.print(F(  "Total render time was    : ")); Serial.print(drawTime); Serial.println(F(" ms"));
  Serial.println(F(""));
}
