#include <OSCBoards.h>
#include <OSCBundle.h>
#include <OSCData.h>
#include <OSCMatch.h>
#include <OSCMessage.h>
#include <OSCTiming.h>

#include <WiFi.h>
//#include <ESP8266WiFi.h>
//#include <ESP8266mDNS.h>
//#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <SimpleTimer.h>

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

//WIFI setup
const char* ssid = "StudioFluora_2G";
const char* password = "studio!!!fluora111";

SimpleTimer timer;
bool otaHold = false;
bool oscConnected = false;
int timeSinceLastConnect = 0;
unsigned long oldTime = 0;
unsigned long newTime = 0;

//OSC setup
WiFiUDP Udp;
const IPAddress outIp(192, 168, 50, 207);     // remote IP of your computer
const unsigned int outPort = 9999;          // remote port to receive OSC
const unsigned int localPort = 8888;        // local port to listen for OSC packets (not used/tested)
int oscConnect = 0;
OSCErrorCode error;

byte mac[6];

//NEOPIXELS setup
#define PIN 15
#define NUM_LEDS 12

#define BRIGHTNESS 50

int blinky = 0;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);

byte neopix_gamma[] = {
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
  2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
  5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
  10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
  17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
  25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
  37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
  51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
  69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
  90, 92, 93, 95, 96, 98, 99, 101, 102, 104, 105, 107, 109, 110, 112, 114,
  115, 117, 119, 120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142,
  144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175,
  177, 180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213,
  215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252, 255
};


void setup() {
  // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("Booting");
  Serial.print("Attempting to connect to SSID: ");
  Serial.println(ssid);

  strip.setBrightness(BRIGHTNESS);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  pulseWhite(5);
  

  /**/

  //Serial.println("Before");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  //Serial.println(WiFi.waitForConnectResult());
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    //Serial.println("Connection Failed! Rebooting...");
    strip.setBrightness(BRIGHTNESS);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
    pulseRed(5);
    delay(5000);
    ESP.restart();
  }
  //Serial.println("After");

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    //Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    //Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    //Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    //Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("SUCCESS");
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  pulseGreen(5);

  Udp.begin(localPort);
  oscConnect = timer.setInterval(2500, sendTestOSC);

  WiFi.macAddress(mac);

  oldTime = millis();

}

void loop() {

  newTime = millis();

  if (!otaHold) { 
    OSCBundle bundle;
    int size = Udp.parsePacket();

    if (size > 0) {
      timer.disable(oscConnect);
      timeSinceLastConnect = 0;
      //Serial.println("Something");
      while (size--) {
        bundle.fill(Udp.read());
      }
      if (!bundle.hasError()) {
        // int numMsg = bundle.size();
        // Serial.print("Received Message: ");
        // Serial.println(numMsg);

        // for (int i = 0; i < numMsg; i++){
        //   char msgAddr[10];
        //   OSCMessage msg = bundle.getOSCMessage(i);
        //   msg.getAddress(msgAddr);
          
        //   Serial.print("Address: ");
        //   Serial.println(msgAddr);
	      // }

        // Serial.println();
        
        bundle.dispatch("/led", setPixels);
        bundle.dispatch("/ledAll", setAllPixels);
        bundle.dispatch("/ledRange", setPixelRange);
        bundle.dispatch("/ota", setOTA);
        bundle.dispatch("/ping", pingPixels);
        bundle.dispatch("/clear", clearPixels);
//        if ( timer.isEnabled(oscConnect) ) {
//          timer.toggle(oscConnect);
//        }
        
      }
      else {
        error = bundle.getError();
        OSCMessage msg("/error");
        msg.add(error); 
        Udp.beginPacket(outIp, outPort);
        msg.send(Udp);
        Udp.endPacket();
        msg.empty();
        Serial.print("error: ");
        Serial.println(error);
      }
    }
    else {
      timeSinceLastConnect += newTime - oldTime;
    }

    if (timeSinceLastConnect > 5000 && !timer.isEnabled(oscConnect) ) {
      timer.enable(oscConnect);
      //timer.restartTimer(oscConnect);
      //dontDie();
    }
  }
  else{
    ArduinoOTA.handle();
  }
  //Serial.println(size);
  //delay(5);
  timer.run();

  oldTime = newTime;

  //
  // Some example procedures showing how to display to the pixels:

}

void sendTestOSC() {
  Serial.println("testing OSC");
  OSCMessage msg("/test");
  msg.add(NUM_LEDS);
  msg.add(timer.getNumTimers());
  for(int i = 0; i < 6; i++){
   msg.add(int(mac[i])); 
  }
  Udp.beginPacket(outIp, outPort);
  msg.send(Udp);
  Udp.endPacket();
  msg.empty();
  if (strip.getPixelColor(0) < 6) {
    //dontDie();
    blinky += 1;
    blinky %= 2;
  }
}

void clearPixels(OSCMessage &msg) {
  //Serial.print("/clear: ");
  //Serial.println(msg.getInt(0));
  for (uint16_t i = 0; (i < msg.size() && i < strip.numPixels() ); i++) {
    strip.setPixelColor(i, 0, 0, 0 );
  }
  strip.show();
  //ledState = msg.getInt(0);
}

void setPixels(OSCMessage &msg) {
  //Serial.print("/led: ");
  //Serial.println(msg.getInt(0));
  for (uint16_t i = 0; (i < msg.size() && i < strip.numPixels() ); i++) {
    strip.setPixelColor(i, (uint32_t) msg.getInt(i) );
    //uint32_t pColor = ((uint32_t) msg.getInt(i));
    //strip.setPixelColor(i, green(pColor), red(pColor), blue(pColor) );
  }
  strip.show();
  //ledState = msg.getInt(0);
}

void setAllPixels(OSCMessage &msg) {
  Serial.print("/ledAll: ");
  Serial.println(msg.getInt(0));
  if(msg.size() > 0){
    uint32_t setColor = (uint32_t) msg.getInt(0);
    for (uint16_t i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, setColor);
    }
    strip.show();
  }
}

void setPixelRange(OSCMessage &msg) {
  //Serial.print("/ledAll: ");
  //Serial.println(msg.getInt(0));
  if(msg.size() > 1){
    int pixMin = msg.getInt(0);
    int pixMax = msg.getInt(1);
    int pixRange = (pixMax - pixMin);
    if(msg.size() > 2 && pixMax < strip.numPixels()){
      for (uint16_t i = 0; i < pixMin; i++) {
        strip.setPixelColor(i, strip.Color(0, 0, 0, 0 ));
      }
      if(msg.size() >= 2 + pixRange){
        for (uint16_t i = pixMin; i < pixRange; i++) {
          strip.setPixelColor(i, (uint32_t) msg.getInt(i - pixMin));
        }
      }
      else {
        uint32_t setColor = (uint32_t) msg.getInt(2);
        for (uint16_t i = pixMin; i < pixMax; i++) {
          strip.setPixelColor(i, setColor);
        }
      }
      for (uint16_t i = pixMax; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, strip.Color(0, 0, 0, 0 ));
      }
      strip.show();
    }
  }
}

void setOTA(OSCMessage &msg) {
  otaHold = true;
  //Serial.print("/ota: ");
  //Serial.println(msg.getInt(0));

  unsigned long sTime = millis();
  unsigned long cTime = millis();
  int yellowPix = 1;
  while (cTime < sTime + 60000) {
    cTime = millis();
    ArduinoOTA.handle();
    delay(20);
    for (uint16_t i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color(100 * yellowPix, 100 * yellowPix, 0, 0 ) );
    }
    strip.show();
    yellowPix += 1;
    yellowPix %= 2;
    ArduinoOTA.handle();
  }
  otaHold = false;
  for (uint16_t i = 0; (i < msg.size() && i < strip.numPixels() ); i++) {
    strip.setPixelColor(i, 0, 0, 0 );
  }
  strip.show();

}

void pingPixels(OSCMessage &msg) {
  Serial.print("/ping: ");
  int ping = msg.getInt(0);
  Serial.println(ping);
  //ledState = msg.getInt(0);

  //colorWipe(strip.Color(255, 0, 0), 50); // Red
  //colorWipe(strip.Color(0, 255, 0), 50); // Green
  //colorWipe(strip.Color(0, 0, 255), 50); // Blue
  // colorWipe(strip.Color(255, 255, 255, 255), 50); // White

  //whiteOverRainbow(20, 75, 5);
  if(ping) pulseWhite(5);
  //rainbowFade2White(3, 3, 1);
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

void pulseWhite(uint8_t wait) {
  for (int j = 0; j < 256 ; j++) {
    for (uint16_t i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color(neopix_gamma[j], neopix_gamma[j], neopix_gamma[j], neopix_gamma[j] ) );
    }
    delay(wait);
    strip.show();
  }

  for (int j = 255; j >= 0 ; j--) {
    for (uint16_t i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color(neopix_gamma[j], neopix_gamma[j], neopix_gamma[j], neopix_gamma[j] ) );
    }
    delay(wait);
    strip.show();
  }
}


void dontDie() {
  for (int j = 0; j < 256 ; j += 10) {
    for (uint16_t i = 0; i < 3; i++) {
      strip.setPixelColor(i, strip.Color(neopix_gamma[j], neopix_gamma[j], neopix_gamma[j], neopix_gamma[j] ) );
    }
    delay(2);
    strip.show();
  }
  for (uint16_t i = 3; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(0, 0, 0, 0 ) );
  }
  delay(2);
  strip.show();
}

void pulseGreen(uint8_t wait) {
  for (int j = 0; j < 256 ; j++) {
    for (uint16_t i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color(0, neopix_gamma[j], 0, 0 ) );
    }
    delay(wait);
    strip.show();
  }

  for (int j = 255; j >= 0 ; j--) {
    for (uint16_t i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color(0, neopix_gamma[j], 0, 0 ) );
    }
    delay(wait);
    strip.show();
  }
}

void pulseRed(uint8_t wait) {
  for (int j = 0; j < 256 ; j++) {
    for (uint16_t i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color(neopix_gamma[j], 0, 0, 0 ) );
    }
    delay(wait);
    strip.show();
  }

  for (int j = 255; j >= 0 ; j--) {
    for (uint16_t i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color(neopix_gamma[j], 0, 0, 0 ) );
    }
    delay(wait);
    strip.show();
  }
}


void rainbowFade2White(uint8_t wait, int rainbowLoops, int whiteLoops) {
  float fadeMax = 100.0;
  int fadeVal = 0;
  uint32_t wheelVal;
  int redVal, greenVal, blueVal;

  for (int k = 0 ; k < rainbowLoops ; k ++) {

    for (int j = 0; j < 256; j++) { // 5 cycles of all colors on wheel

      for (int i = 0; i < strip.numPixels(); i++) {

        wheelVal = Wheel(((i * 256 / strip.numPixels()) + j) & 255);

        redVal = red(wheelVal) * float(fadeVal / fadeMax);
        greenVal = green(wheelVal) * float(fadeVal / fadeMax);
        blueVal = blue(wheelVal) * float(fadeVal / fadeMax);

        strip.setPixelColor( i, strip.Color( redVal, greenVal, blueVal ) );

      }

      //First loop, fade in!
      if (k == 0 && fadeVal < fadeMax - 1) {
        fadeVal++;
      }

      //Last loop, fade out!
      else if (k == rainbowLoops - 1 && j > 255 - fadeMax ) {
        fadeVal--;
      }

      strip.show();
      delay(wait);
    }

  }



  delay(500);


  for (int k = 0 ; k < whiteLoops ; k ++) {

    for (int j = 0; j < 256 ; j++) {

      for (uint16_t i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, strip.Color(0, 0, 0, neopix_gamma[j] ) );
      }
      strip.show();
    }

    delay(2000);
    for (int j = 255; j >= 0 ; j--) {

      for (uint16_t i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, strip.Color(0, 0, 0, neopix_gamma[j] ) );
      }
      strip.show();
    }
  }

  delay(500);


}

void whiteOverRainbow(uint8_t wait, uint8_t whiteSpeed, uint8_t whiteLength ) {

  if (whiteLength >= strip.numPixels()) whiteLength = strip.numPixels() - 1;

  int head = whiteLength - 1;
  int tail = 0;

  int loops = 3;
  int loopNum = 0;

  static unsigned long lastTime = 0;


  while (true) {
    for (int j = 0; j < 256; j++) {
      for (uint16_t i = 0; i < strip.numPixels(); i++) {
        if ((i >= tail && i <= head) || (tail > head && i >= tail) || (tail > head && i <= head) ) {
          strip.setPixelColor(i, strip.Color(255, 255, 255, 255 ) );
        }
        else {
          strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
        }

      }

      if (millis() - lastTime > whiteSpeed) {
        head++;
        tail++;
        if (head == strip.numPixels()) {
          loopNum++;
        }
        lastTime = millis();
      }

      if (loopNum == loops) return;

      head %= strip.numPixels();
      tail %= strip.numPixels();
      strip.show();
      delay(wait);
    }
  }

}
void fullWhite() {

  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(255, 255, 255, 255 ) );
  }
  strip.show();
}


// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for (j = 0; j < 256 * 5; j++) { // 5 cycles of all colors on wheel
    for (i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for (j = 0; j < 256; j++) {
    for (i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3, 0);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3, 0);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0, 0);
}

uint8_t red(uint32_t c) {
  return (c >> 16);
}
uint8_t green(uint32_t c) {
  return (c >> 8);
}
uint8_t blue(uint32_t c) {
  return (c);
}


