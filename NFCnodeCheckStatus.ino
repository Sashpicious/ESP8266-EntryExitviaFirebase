//Include Lib for Arduino to Nodemcu
#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
#include <FirebaseESP8266.h>
#include <Servo.h>
#include <SPI.h>
#include <PN532_SPI.h>
#include <PN532.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define FIREBASE_HOST "xxxxxxxx" // Firebase host address
#define FIREBASE_AUTH "xxxxxxxxxxx" //Firebase Auth code

#define WIFI_SSID "xxxxxx" //Enter your wifi Name
#define WIFI_PASSWORD "xxxxx" // Enter your password

#define LED_PIN_G 16 // define LED pins i.e. GPIO16
#define LED_PIN_R 5



PN532_SPI pn532spi(SPI, D2);
PN532 nfc(pn532spi);

//Timezone
WiFiUDP ntpUDP;
const long utcOffsetInSeconds = 28800; //(UTC+8) Get Malaysia time
NTPClient timeClient(ntpUDP, "pool.ntp.org");


FirebaseJson json;
//Define FirebaseESP8266 data object
FirebaseData firebaseData;
FirebaseData firebaseData1;
FirebaseData firebaseData2;
FirebaseData firebaseData3;
FirebaseData firebaseData4;
String cardUID, userType, userName, userCar;

Servo servo;
int angle = 10;
void setup() {

  Serial.begin(115200);
  
  servo.attach(15);
  servo.write(angle);

  pinMode(LED_PIN_G, OUTPUT);
  pinMode(LED_PIN_R, OUTPUT);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);

  timeClient.begin();   //init timeClient
  timeClient.setTimeOffset(utcOffsetInSeconds);

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }

  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX);
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC);
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);

  // Set the max number of retry attempts to read from a card
  // This prevents us from waiting forever for a card, which is
  // the default behaviour of the PN532.
  //nfc.setPassiveActivationRetries(0xFF);
  nfc.setPassiveActivationRetries(10);

  // configure board to read RFID tags
  nfc.SAMConfig();


  Serial.println("Waiting for an ISO14443A card");
  Serial.println("\n-----------\n");
}

void motorFunction(){  // servo motor
// scan from 0 to 180 degrees
  for(angle = 10; angle < 100; angle++)  
    {                                  
      servo.write(angle);               
      delay(15);                   
    } 
    delay(2000);  
  // now scan back from 180 to 0 degrees
    for(angle = 100; angle > 10; angle--)    
    {                                
      servo.write(angle);           
      delay(15);       
    } 
}

void getTime(String hex_value){ // entry/exit time
  Firebase.setString(firebaseData, "/cardUID/"+hex_value+"/time", timeClient.getFormattedTime());
}

void permitPremium(int userStatus, String hex_value){ // allow premium user
  Firebase.get(firebaseData3, "/cardUID/"+hex_value+"/name"); // get username of card from firebase data
  userName = firebaseData3.stringData();
  Firebase.get(firebaseData4, "/cardUID/"+hex_value+"/numberPlate"); // get numberplate for user from firebase data
  userCar = firebaseData4.stringData();

  if(userStatus == 1){
    Firebase.setInt(firebaseData, "/cardUID/"+hex_value+"/status", 0);
    Serial.println("Goodbye "+userName+"!");
    Serial.println("-----------");
    digitalWrite(LED_PIN_G, HIGH);
    motorFunction();
    getTime(hex_value);
    delay(2000);
    digitalWrite(LED_PIN_G, LOW);
        
  }else{
    Firebase.setInt(firebaseData, "/cardUID/"+hex_value+"/status", 1);
    Serial.println("Premium user found!");
    Serial.println("Welcome to NFC Parking, "+userName);
    Serial.println("Number Plate: "+userCar);
    Serial.println("-----------");
    digitalWrite(LED_PIN_G, HIGH);
    motorFunction();
    getTime(hex_value);// passes cardUID to getTime
    delay(2000);
    digitalWrite(LED_PIN_G, LOW);
  }
  delay(1000);
}
      

void permitNormal(int userStatus, String hex_value){ // allow normal user
  Firebase.get(firebaseData3, "/cardUID/"+hex_value+"/name"); // get username of card from firebase data
  userName = firebaseData3.stringData();
  Firebase.get(firebaseData4, "/cardUID/"+hex_value+"/numberPlate"); // get numberplate for user from firebase data
  userCar = firebaseData4.stringData();

  if(userStatus == 1){
    Firebase.setInt(firebaseData, "/cardUID/"+hex_value+"/status", 0); // set userstatus to 0
    Serial.println("Goodbye "+userName+"!");
    Serial.println("-----------");
    digitalWrite(LED_PIN_G, HIGH);
    motorFunction();
    getTime(hex_value); // passes cardUID to getTime
    delay(2000);
    digitalWrite(LED_PIN_G, LOW);
        
  }else{
    Firebase.setInt(firebaseData, "/cardUID/"+hex_value+"/status", 1); // set userstatus to 1
    Serial.println("User found!");
    Serial.println("Welcome to NFC Parking, "+userName);
    Serial.println("Number Plate: "+userCar);
    Serial.println("-----------");
    digitalWrite(LED_PIN_G, HIGH);
    motorFunction();
    getTime(hex_value); // passes cardUID to getTime
    delay(2000);
    digitalWrite(LED_PIN_G, LOW);
  }
  delay(1000);
}
  

void checkEntryPremium(String hex_value){

   if(Firebase.getInt(firebaseData,"/cardUID/"+hex_value+"/status")){ //check status
     if(firebaseData.dataType()=="int"){
       int userStatus = firebaseData.intData();
       permitPremium(userStatus,hex_value);
     }
     else {
        Serial.println(firebaseData.errorReason());
        Serial.println("Fail En Check");
    }
   }
}

void checkEntryNormal(String hex_value){
  if(Firebase.getInt(firebaseData,"/cardUID/"+hex_value+"/status")){ //check status
     if(firebaseData.dataType()=="int"){
       int userStatus = firebaseData.intData();
       permitNormal(userStatus,hex_value);
     }else {
        Serial.println(firebaseData.errorReason());
        Serial.println("Fail En Check");
   }
  }
}

void checkUser(String hex_value){

  Firebase.get(firebaseData1, "/cardUID/"+hex_value+"/card");// compare cardUID with firebase data
    cardUID = firebaseData1.stringData();
    Firebase.get(firebaseData2, "/cardUID/"+hex_value+"/userType"); // compare userType (premium/normal) with firebase data
    userType = firebaseData2.stringData();
    

    // print the result based on whether a match was found or not
    if (cardUID == hex_value) {
      if (userType == "1"){
        checkEntryPremium(hex_value);
      }
      else {
       checkEntryNormal(hex_value);
      }
    } 

}

void nfcFunction(){
boolean success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength); yield();

  if (success) {

    Serial.println("Card Detected!");
    // Serial.print("UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
    // Serial.print("UID Value: ");

    String hex_value = "";
    char uidStr[uidLength * 3 + 1];

    for (uint8_t i=0; i < uidLength; i++) 

    {
      sprintf(uidStr + i * 3, "%02X:", uid[i]); // converts uid into xx:xx:xx:xx string
    }

    uidStr[uidLength * 3 - 1] = '\0';  // Replace the last colon with a null terminator
    // Serial.println(uidStr);
    hex_value += (String)uidStr;
    Serial.println("Card UID: "+ hex_value);
    Serial.println("-----------");

    checkUser(hex_value);
    }
    else
    yield();
    delay(1000);
}

void loop() {
  nfcFunction(); // keeps looping pn532 to check for cards
}
