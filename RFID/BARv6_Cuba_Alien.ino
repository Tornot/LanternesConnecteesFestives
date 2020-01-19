/**
 * ----------------------------------------------------------------------------
 * This is a MFRC522 library example; see https://github.com/miguelbalboa/rfid
 * for further details and other examples.
 *
 * NOTE: The library file MFRC522.h has a lot of useful info. Please read it.
 *
 * Released into the public domain.
 * ----------------------------------------------------------------------------
 * This sample shows how to read and write data blocks on a MIFARE Classic PICC
 * (= card/tag).
 *
 * BEWARE: Data will be written to the PICC, in sector #1 (blocks #4 to #7).
 *
 *
 * Typical pin layout used:
 * -----------------------------------------------------------------------------------------
 *             MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
 *             Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
 * Signal      Pin          Pin           Pin       Pin        Pin              Pin
 * -----------------------------------------------------------------------------------------
 * RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
 * SPI SS      SDA(SS)      10            53        D10        10               10
 * SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
 * SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
 * SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
 *
 */
/* BARv3 implémente la possibilité d'utiliser plusieurs modules RC522 à la fois.
 * The PCD (short for Proximity Coupling Device): NXP MFRC522 Contactless Reader IC.
 * The PICC (short for Proximity Integrated Circuit Card): A card or tag using the ISO 14443A interface, eg Mifare or NTAG203.
 */

#include <SPI.h>
#include <MFRC522.h>
/* PINOUT for NodeMCU*/

#define RST_PIN         16      //D0    // Configurable, see typical pin layout above
#define SS_0_PIN        5       //D1    // Configurable, see typical pin layout above
#define SS_1_PIN        4       //D2    // see SD3 on board
#define SS_2_PIN        0       //D3
#define SS_3_PIN        2       //D4
/*PINOUT for Arduino MEGA*/
/*
#define RST_PIN     12       
#define SS_0_PIN    11      
#define SS_1_PIN    10      
#define SS_2_PIN    9      
#define SS_3_PIN    8     
*/
// Number of RFiD writers used
#define NR_OF_WRITERS   3

MFRC522 mfrc522[NR_OF_WRITERS];
byte ssPins[] = {SS_0_PIN, SS_1_PIN, SS_2_PIN, SS_3_PIN}; //, SS_3_PIN, SS_4_PIN

MFRC522::MIFARE_Key key;
//Oui c'est moche de mettre ça ici...
//Preparing data for future writing
// In this sample we use the second sector,
// that is: sector #1, covering block #4 up to and including block #7
byte sector         = 1;
byte blockAddr      = 4;

byte dataBlockCocktail_0[]    = {
    0x00, 0x00, 0xCC, 0x00,  //  pure RED
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
};
byte dataBlockCocktail_1[]    = {
    0xDC, 0xFF, 0x00, 0x00,  //  pure GREEN
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
};

byte trailerBlock   = 7;
MFRC522::StatusCode status;
byte buffer[18];
byte size = sizeof(buffer);

void ReadDataFromRFIDTag(MFRC522 mfrc522);
void WriteDataToRFIDTag(MFRC522 mfrc522);
/**
 * Initialize.
 */
void setup() {
    Serial.begin(9600); // Initialize serial communications with the PC
    while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
    SPI.begin();        // Init SPI bus

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

    Serial.println(F("Scan a MIFARE Classic PICC to demonstrate read and write."));
    Serial.print(F("Using key (for A and B):"));
    dump_byte_array(key.keyByte, MFRC522::MF_KEY_SIZE);
    Serial.println();

    Serial.println(F("BEWARE: Data will be written to the PICC, in sector #1"));


}

void loop()
{
    for (uint8_t writer = 0; writer < NR_OF_WRITERS; writer++)
    {
        if (mfrc522[writer].PICC_IsNewCardPresent() && mfrc522[writer].PICC_ReadCardSerial())
        {
            ReadDataFromRFIDTag(mfrc522[writer]);

            switch (writer)
            {
                case 0:
                    WriteDataToRFIDTag(mfrc522[writer], dataBlockCocktail_0);
                    break;
                case 1:
                    WriteDataToRFIDTag(mfrc522[writer], dataBlockCocktail_1);
                    break;
            }

            /*Dump, Halt andStop encryption must be done before stopping communication*/
            // Dump the sector data
            Serial.println(F("Current data in sector:"));
            mfrc522[writer].PICC_DumpMifareClassicSectorToSerial(&(mfrc522[writer].uid), &key, sector);
            Serial.println();

            // Halt PICC
            mfrc522[writer].PICC_HaltA();
            // Stop encryption on PCD
            mfrc522[writer].PCD_StopCrypto1();
        }
    }
}

/**
   Helper routine to dump a byte array as hex values to Serial.
*/
void dump_byte_array(byte * buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

void ReadDataFromRFIDTag(MFRC522 mfrc522)
{
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

void WriteDataToRFIDTag(MFRC522 mfrc522, byte dataBlock[])
{
    // Authenticate using key B
    Serial.println(F("Authenticating again using key B..."));
    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("PCD_Authenticate() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    }

    // Write data to the block
    Serial.print(F("Writing data into block ")); Serial.print(blockAddr);
    Serial.println(F(" ..."));
    dump_byte_array(dataBlock, 16); Serial.println();
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr, dataBlock, 16);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Write() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
    }
    Serial.println();

    // Read data from the block (again, should now be what we have written)
    Serial.print(F("Reading data from block ")); Serial.print(blockAddr);
    Serial.println(F(" ..."));
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Read() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
    }
    Serial.print(F("Data in block ")); Serial.print(blockAddr); Serial.println(F(":"));
    dump_byte_array(buffer, 16); Serial.println();

    // Check that data in block is what we have written
    // by counting the number of bytes that are equal
    Serial.println(F("Checking result..."));
    byte count = 0;
    for (byte i = 0; i < 16; i++) {
        // Compare buffer (= what we've read) with dataBlock (= what we've written)
        if (buffer[i] == dataBlock[i])
            count++;
    }
    Serial.print(F("Number of bytes that match = ")); Serial.println(count);
    if (count == 16) {
        Serial.println(F("Success :-)"));
    } else {
        Serial.println(F("Failure, no match :-("));
        Serial.println(F("  perhaps the write didn't work properly..."));
    }
    Serial.println();
}
