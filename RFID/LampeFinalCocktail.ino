/***************************************************
  Adafruit MQTT Library ESP8266 Example

  Must use ESP8266 Arduino from:
    https://github.com/esp8266/Arduino

  Works great with Adafruit's Huzzah ESP board & Feather
  ----> https://www.adafruit.com/product/2471
  ----> https://www.adafruit.com/products/2821

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Tony DiCola for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/
#include <SPI.h>
#include <MFRC522.h>
#include <FastLED.h>
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"


/************************* WiFi Access Point *********************************/

#define WLAN_SSID       "AYMERICO<3"
#define WLAN_PASS       ""

/************************* Adafruit.io Setup *********************************/

#define AIO_SERVER      "10.42.0.1"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    ""//"...your AIO username (see https://accounts.adafruit.com)..."
#define AIO_KEY         ""//"...your AIO key..."

#define RST_PIN         16      //D0    // Configurable, see typical pin layout above
#define SS_0_PIN        5       //D1    // Configurable, see typical pin layout above
#define RST_PIN         16      //D0    // Configurable, see typical pin layout above
#define SS_0_PIN        5       //D1    // Configurable, see typical pin layout above
#define LED_PIN     2 //D2
#define NUM_LEDS    30 //nombre de led
#define BRIGHTNESS  255
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];
#define NR_OF_WRITERS 1
#define nbrGlassPerTable 5
/************ Global State (you don't need to change this!) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
// or... use WiFiFlientSecure for SSL
//WiFiClientSecure client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/

// Setup a feed called 'potValue' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
//Adafruit_MQTT_Publish potValue = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/potValue");

// Setup a feed called 'ledBrightness' for subscribing to changes.

//Adafruit_MQTT_Subscribe ledBrightness = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "sound/bass");
Adafruit_MQTT_Subscribe ledBrightness = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "sound/snare");

/*************************** Sketch Code ************************************/


MFRC522 mfrc522[NR_OF_WRITERS];
byte ssPins[] = {SS_0_PIN}; //, SS_3_PIN, SS_4_PIN

MFRC522::MIFARE_Key key;
//Oui c'est moche de mettre ça ici...
//Preparing data for future writing
// In this sample we use the second sector,
// that is: sector #1, covering block #4 up to and including block #7
byte sector         = 1;
byte blockAddr      = 4;
byte trailerBlock   = 7;
MFRC522::StatusCode status;
byte buffer[18];
byte size = sizeof(buffer);

//byt cRGB[3]={0}
;




struct glassData
{
    byte tagID[4]={0};
    byte color[3]={0};
};

static glassData glassArray[nbrGlassPerTable] = {};
uint8_t glassPosition = 200; //Init at 200 to reset pos at the begin of the loop of RegisteringGlass

void ReadDataFromRFIDTag(MFRC522 mfrc522);
void TagReading();
void light();
void ReadDataFromRFIDTag(MFRC522 mfrc522);
void dump_byte_array(byte * buffer, byte bufferSize);
void RegisteringGlass(byte * buffer, MFRC522 mfrc522);
// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
void MQTT_connect();

//uint8_t ledPin = D6;
//uint16_t potAdcValue = 0;
uint16_t ledBrightValue = 0;

void setup() {
  Serial.begin(115200);
  delay(10);

  Serial.println(F("Adafruit MQTT demo"));

  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());

  // Setup MQTT subscription for ledBrightness feed.
  mqtt.subscribe(&ledBrightness);
  SPI.begin();        // Init SPI bus
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
    FastLED.setBrightness(  BRIGHTNESS );
    // Prepare the key (used both as key A and as key B)
    // using FFFFFFFFFFFFh which is the default at chip delivery from the factory
    // --> Les clés sont importantes pour que quand on place un tag, il n'est lu qu'une seul fois tant qu'on ne l'a pas retiré

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
    for (uint8_t writer = 0; writer < NR_OF_WRITERS; writer++) 
    {
        mfrc522[writer].PCD_Init(ssPins[writer], RST_PIN);
        mfrc522[writer].PCD_DumpVersionToSerial();// Show debug infos about the PCD
        // affichage check
        Serial.print(F("Setup writer "));
        Serial.print(writer);
        Serial.print(F(" on pin : "));
        Serial.println(ssPins[writer]);
    }

    //mfrc522.PCD_Init(); // Init MFRC522 card

   
    
    dump_byte_array(key.keyByte, MFRC522::MF_KEY_SIZE);
    

    

  
}

void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  // this is our 'wait for incoming subscription packets' busy subloop
  // try to spend your time here

  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(100))) {
    if (subscription == &ledBrightness) {
        //Serial.print(F("Got LED Brightness : "));
        ledBrightValue = atoi((char *)ledBrightness.lastread);
       //Serial.println(ledBrightValue);
//      analogWrite(ledPin, ledBrightValue);
        
    }
  }
//Serial.print(F("Out : "));
light();
TagReading();
  // Now we can publish stuff!
  /*uint16_t AdcValue = analogRead(A0);
  if((AdcValue > (potAdcValue + 7)) || (AdcValue < (potAdcValue - 7))){
    potAdcValue = AdcValue;
    Serial.print(F("Sending pot val "));
    Serial.print(potAdcValue);
    Serial.print("...");
    if (! potValue.publish(potAdcValue)) {
      Serial.println(F("Failed"));
    } else {
      Serial.println(F("OK!"));
    }
  }*/
  // ping the server to keep the mqtt connection alive
  // NOT required if you are publishing once every KEEPALIVE seconds
  /*
  if(! mqtt.ping()) {
    mqtt.disconnect();
  }
  */
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() 
{
  int8_t ret;
  // Stop if already connected.
  if (mqtt.connected()) 
  {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) 
       {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}

void TagReading()
{
    for (uint8_t writer = 0; writer < NR_OF_WRITERS; writer++)
    {
        if (mfrc522[writer].PICC_IsNewCardPresent() && mfrc522[writer].PICC_ReadCardSerial())
        { 
            ReadDataFromRFIDTag(mfrc522[writer]);
            /*Dump, Halt andStop encryption must be done before stopping communication*/
            // Dump the sector data
            //cRGB[0]=buffer[0];
           // FastLED.showColor((CRGB(buffer[0],buffer[1], buffer[2])), 255);
            int i;
            //if (inc==6){inc=0;}

       
            Serial.println(F("Current data in sector:"));
            mfrc522[writer].PICC_DumpMifareClassicSectorToSerial(&(mfrc522[writer].uid), &key, sector);
            Serial.println();

            RegisteringGlass(buffer, mfrc522[writer]);
            //Now, we want to know the glass is already registered
            // Halt PICC
            mfrc522[writer].PICC_HaltA();
            // Stop encryption on PCD
            mfrc522[writer].PCD_StopCrypto1();
        }
    }
}

void light ()
{
    int inc;

    for(inc=0;inc<6;inc++){leds[inc]=CRGB(glassArray[0].color[0]*ledBrightValue/255,glassArray[0].color[1]*ledBrightValue/255, glassArray[0].color[2]*ledBrightValue/255);}

    for(inc=6;inc<12;inc++){leds[inc]=CRGB(glassArray[1].color[0]*ledBrightValue/255,glassArray[1].color[1]*ledBrightValue/255, glassArray[1].color[2]*ledBrightValue/255);}

    for(inc=12;inc<18;inc++){leds[inc]=CRGB(glassArray[2].color[0]*ledBrightValue/255,glassArray[2].color[1]*ledBrightValue/255, glassArray[2].color[2]*ledBrightValue/255);}

    for(inc=18;inc<24;inc++){leds[inc]=CRGB(glassArray[3].color[0]*ledBrightValue/255,glassArray[3].color[1]*ledBrightValue/255, glassArray[3].color[2]*ledBrightValue/255);}

    for(inc=24;inc<30;inc++){leds[inc]=CRGB(glassArray[4].color[0]*ledBrightValue/255,glassArray[4].color[1]*ledBrightValue/255, glassArray[4].color[2]*ledBrightValue/255);}

    FastLED.show();
}

void dump_byte_array(byte * buffer, byte bufferSize) 
{
  for (byte i = 0; i < bufferSize; i++) 
  {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

void RegisteringGlass(byte * buffer, MFRC522 mfrc522)
{
    bool alreadyInTable = false;
    for (uint8_t i=0; i<nbrGlassPerTable; i++)
    {
        uint8_t k = 0;
        uint8_t n =0;
        Serial.println("Debug0");
        //Check si UId est la même
        for (uint8_t j=0; j<4; j++)
        {   
            Serial.print("glassArray: ");
            Serial.print(glassArray[i].tagID[j]);
            Serial.print(" TagUID: ");
            Serial.print(mfrc522.uid.uidByte[j]);
            if (glassArray[i].tagID[j] == mfrc522.uid.uidByte[j]) k++;
        }
        //Check si color est le même
        for (uint8_t j=0; j<3; j++)
        {
            if (glassArray[i].color[j] == buffer[j]) n++;
        }

        Serial.println(k);
        Serial.println(n);
        // si UID et color sont les mêmes, alors le verre est déjà dans la table
        if (k==4 && n == 3) 
        {
            alreadyInTable = true;
            Serial.println("TAG IS ALREADY IN TABLE");
        }
    }

    if (!alreadyInTable)    //if not registered, we register it in glassArray
    {   //After the readDataFromRFIDTag, there is the data of the tag in the buffer
        //So we can write it in the glassArray
       // inc++;
        //if (inc==6){inc=0;}
        //Loop the position in the glassArray
        glassPosition++;
        if (glassPosition>=nbrGlassPerTable)  glassPosition=0;
        for(int k = nbrGlassPerTable-1 ; k > 0; k--)//Pour chaque verre, sauf le premier car on va l'écraser
        {
            for (uint8_t j=0; j<4; j++)//On déplace les 4 octets de l'UID
            {
                glassArray[k].tagID[j] = glassArray[k-1].tagID[j];
            }
            for(uint8_t i=0; i<3; i++)//On déplace les 3 octets de couleur
            {
                glassArray[k].color[i] = glassArray[k-1].color[i];
            }
        }
        //Enregistre UID
        for (uint8_t j=0; j<4; j++) 
        {
            glassArray[0].tagID[j] = mfrc522.uid.uidByte[j];
        }
        //Enregistre la couleur
        for(uint8_t i=0; i<3; i++)//La boucle est clean, à la sortie, color[i]=buffer[i], plus besoin de debug
        {
            glassArray[0].color[i] = buffer[i];
        }
    }
    //Debug routine
    for (uint8_t i=0; i<nbrGlassPerTable; i++)
    {
        for (uint8_t j=0; j<3; j++)
        {
            Serial.print(glassArray[i].color[j]);
            Serial.print("+");
        }
        Serial.println("");
    }
}


void ReadDataFromRFIDTag(MFRC522 mfrc522)
{
    bool thereIsACard = true;
    // Select one of the cards
    /*if ( ! mfrc522.PICC_ReadCardSerial())
    {
        return;
    }*/
    // Show some details of the PICC (that is: the tag/card)
    Serial.print(F("Card UID:"));
    dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
    Serial.println();
    Serial.print(F("PICC type: "));
    MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
    Serial.println(mfrc522.PICC_GetTypeName(piccType));

    // Check for compatibility
    if (    piccType != MFRC522::PICC_TYPE_MIFARE_MINI
        &&  piccType != MFRC522::PICC_TYPE_MIFARE_1K
        &&  piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
        Serial.println(F("This sample only works with MIFARE Classic cards."));
        return;
    }
    // Authenticate using key A
    Serial.println(F("Authenticating using key A..."));
    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("PCD_Authenticate() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    }

    // Show the whole sector as it currently is
    Serial.println(F("Current data in sector:"));
    mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);
    Serial.println();

    // Read data from the block
    Serial.print(F("Reading data from block ")); Serial.print(blockAddr);
    Serial.println(F(" ..."));
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Read() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
    }
    Serial.print(F("Data in block ")); Serial.print(blockAddr); Serial.println(F(":"));
    dump_byte_array(buffer, 16); Serial.println();
    Serial.println();

}            
