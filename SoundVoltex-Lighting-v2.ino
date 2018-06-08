#include "PluggableUSB.h"
#include "HID.h"
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PIN 7
#define SWITCH 0

#define NUMBER_OF_SINGLE 8
#define NUMBER_OF_RGB 5

Adafruit_NeoPixel strip = Adafruit_NeoPixel(120, PIN, NEO_GRB + NEO_KHZ800);

int brightnessFactor = 20; // 5 - 35, bright - dim (35 = off)

typedef struct {
  uint8_t brightness;
} SingleLED;

typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} RGBLed;

uint8_t dataSingle[NUMBER_OF_SINGLE];
uint8_t dataRGB[NUMBER_OF_RGB];


void light_update(SingleLED* single_leds, RGBLed* rgb_leds) {
  for(int i = 0; i < NUMBER_OF_SINGLE; i++) {
    // YOUR CODE HERE
    // led_stuff(single_leds[i].brightness);
  }
  for(int i = 0; i < NUMBER_OF_RGB; i++) {
    // YOUR CODE HERE
    // rgb_stuff(rgb_leds[i].r, rgb_leds[i].g, rgb_leds[i].b);
  }
}

void setColor(uint32_t c, uint16_t from, uint16_t to){
  for(uint16_t i = from; i < to; i ++){
    strip.setPixelColor(i, c);
  }
  strip.show();
}

void off(){
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, 0);
  }
  strip.show();
}

void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

void clearPixel(){
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, 0);
  }
}

void setPixel(uint32_t c, uint16_t at){
  strip.setPixelColor(at, c);
}

void showPixel(){
  strip.show();
}

void showBrightnessLevel(){
    clearPixel();
    setPixel(strip.Color(90, 0, 0), 116);
    setPixel(strip.Color(0, 90, 5), 84);
    setPixel(strip.Color(50, 50, 00), 100);
    setPixel(strip.Color(50, 50, 50), brightnessFactor + 80);
    showPixel();
}

void setup() {
  delay(400);
  
  // Switch
  pinMode(SWITCH,INPUT_PULLUP);
  
  // LED
  //pinMode(LED_BUILTIN, OUTPUT);
  strip.begin();
  colorWipe(strip.Color(27, 2, 1), 0);
  colorWipe(strip.Color(0, 0, 0), 0);
  strip.show(); // Initialize all pixels to 'off'
}

void loop() {
  // USB module does the hard work
}

// ******************************
// don't need to edit below here

#define NUMBER_OF_LIGHTS (NUMBER_OF_SINGLE + NUMBER_OF_RGB*3)
#if NUMBER_OF_LIGHTS > 63
  #error You must have less than 64 lights
#endif

union {
  struct {
    SingleLED singles[NUMBER_OF_SINGLE];
    RGBLed rgb[NUMBER_OF_RGB];
  } leds;
  uint8_t raw[NUMBER_OF_LIGHTS];
} led_data;

static const uint8_t PROGMEM _hidReportLEDs[] = {
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x00,                    // USAGE (Undefined)
    0xa1, 0x01,                    // COLLECTION (Application)
    // Globals
    0x95, NUMBER_OF_LIGHTS,        //   REPORT_COUNT
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,              //   LOGICAL_MAXIMUM (255)
    0x05, 0x0a,                    //   USAGE_PAGE (Ordinals)
    // Locals
    0x19, 0x01,                    //   USAGE_MINIMUM (Instance 1)
    0x29, NUMBER_OF_LIGHTS,        //   USAGE_MAXIMUM (Instance n)
    0x91, 0x02,                    //   OUTPUT (Data,Var,Abs)
    // BTools needs at least 1 input to work properly
    0x19, 0x01,                    //   USAGE_MINIMUM (Instance 1)
    0x29, 0x01,                    //   USAGE_MAXIMUM (Instance 1)
    0x81, 0x03,                    //   INPUT (Cnst,Var,Abs)
    0xc0                           // END_COLLECTION
};

// This is almost entirely copied from NicoHood's wonderful RawHID example
// Trimmed to the bare minimum
// https://github.com/NicoHood/HID/blob/master/src/SingleReport/RawHID.cpp
class HIDLED_ : public PluggableUSBModule {

  uint8_t epType[1];
  
  public:
    HIDLED_(void) : PluggableUSBModule(1, 1, epType) {
      epType[0] = EP_TYPE_INTERRUPT_IN;
      PluggableUSB().plug(this);
    }

    int getInterface(uint8_t* interfaceCount) {
      *interfaceCount += 1; // uses 1
      HIDDescriptor hidInterface = {
        D_INTERFACE(pluggedInterface, 1, USB_DEVICE_CLASS_HUMAN_INTERFACE, HID_SUBCLASS_NONE, HID_PROTOCOL_NONE),
        D_HIDREPORT(sizeof(_hidReportLEDs)),
        D_ENDPOINT(USB_ENDPOINT_IN(pluggedEndpoint), USB_ENDPOINT_TYPE_INTERRUPT, USB_EP_SIZE, 16)
      };
      return USB_SendControl(0, &hidInterface, sizeof(hidInterface));
    }
    
    int getDescriptor(USBSetup& setup)
    {
      // Check if this is a HID Class Descriptor request
      if (setup.bmRequestType != REQUEST_DEVICETOHOST_STANDARD_INTERFACE) { return 0; }
      if (setup.wValueH != HID_REPORT_DESCRIPTOR_TYPE) { return 0; }
    
      // In a HID Class Descriptor wIndex contains the interface number
      if (setup.wIndex != pluggedInterface) { return 0; }
    
      return USB_SendControl(TRANSFER_PGM, _hidReportLEDs, sizeof(_hidReportLEDs));
    }
    
    bool setup(USBSetup& setup)
    {
      if (pluggedInterface != setup.wIndex) {
        return false;
      }
    
      uint8_t request = setup.bRequest;
      uint8_t requestType = setup.bmRequestType;
    
      if (requestType == REQUEST_DEVICETOHOST_CLASS_INTERFACE)
      {
        return true;
      }
    
      if (requestType == REQUEST_HOSTTODEVICE_CLASS_INTERFACE) {
        if (request == HID_SET_REPORT) {
          if(setup.wValueH == HID_REPORT_TYPE_OUTPUT && setup.wLength == NUMBER_OF_LIGHTS){
            USB_RecvControl(led_data.raw, NUMBER_OF_LIGHTS);
            light_update(led_data.leds.singles, led_data.leds.rgb);
            return true;
          }
        }
      }
    
      return false;
    }
};

HIDLED_ HIDLeds;
