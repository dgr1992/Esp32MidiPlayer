#include "../../myauth.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <MIDI.h>
#include <PubSubClient.h>

IPAddress mqttBroker(192, 168, 178, 7);

#define PING_TIMEOUT 120 * 1000

bool playSongFlag = false;
String song;

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

void playMIDINote(byte channel, byte note, byte velocity);
void parser(String buffer);
void playNote(uint16_t note, uint8_t length, uint8_t einMesslaenge, int noteVerlaengern, int noteLaengeVerbinden));
void playSong(String input);
void mqttCallback(char* topic, byte* payload, unsigned int length);
void mqttReconnect();
bool istZahl(String zahl);
int notenLangeInMillisekunden(int laenge, int einMesslaenge, int noteVerlaengern, int noteLaengeVerbinden);


WiFiClient wiFiClient;

PubSubClient psClient(wiFiClient);

void wifiConnect()
{
  IPAddress staticIP(192, 168, 178, 203);
  IPAddress gateway(192, 168, 178, 1);
  IPAddress subnet(255, 255, 255, 0);
  IPAddress dns(192, 168, 178, 1);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  if (WiFi.config(staticIP, gateway, subnet, dns, dns) == false) {
    Serial.println("Configuration failed.");
  }

  // Attempt to connect to Wifi network:
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    if(i++>30)
      esp_restart();
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Subnet Mask: ");
  Serial.println(WiFi.subnetMask());
  Serial.print("Gateway IP: ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("DNS 1: ");
  Serial.println(WiFi.dnsIP(0));
  Serial.print("DNS 2: ");
  Serial.println(WiFi.dnsIP(1));

}



void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  String strTopic = topic;
  if(strTopic.equals("playmidi"))
  {
    payload[length] = '\0';
    Serial.println("play midi vom mqtt erkannt");
    playSong((char*)payload);
  }

}


void mqttReconnect() {
  // Loop until we're reconnected
  while (!psClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (psClient.connect("playmidi")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //psClient.publish("wled/861a06/api","%PL=10");
      // ... and resubscribe
      psClient.subscribe("playmidi");
    } else {
      Serial.print("failed, rc=");
      Serial.print(psClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup()
{
  //Set up serial output with standard MIDI baud rate

  Serial.begin(115200);
  Serial.println();

  wifiConnect();


  MIDI.begin(4);

  MIDI.sendProgramChange(0,1);
    
  psClient.setServer(mqttBroker, 1883);
  psClient.setBufferSize(4096);
  psClient.setCallback(mqttCallback);

  delay(500);

}



void playSong(String input)
{
    uint32_t timeout = millis() + 16 * 1000;

    Serial.println("PLAY ERKANNT");

    //Prüfe BPM
    uint8_t bpm = 240 //Rückwertskompatibilität
    if(input.startsWith("bpm"))
    {
      input.remove(0,3);
      String bpmString = input.c_str();
      if(istZahl(bpmString))
      {
        bpm = atoi(bpmString);
      }
      input.remove(0,bpmString.length());
    }

    // Prüfe das Instrument
    if(input.startsWith("piano"))
    {
      input.remove(0,6);
      MIDI.sendProgramChange(0,1);
    }

    if(input.startsWith("vibes"))
    {
      input.remove(0,6);
      MIDI.sendProgramChange(11,1);
    }

    if(input.startsWith("organ"))
    {
      input.remove(0,6);
      MIDI.sendProgramChange(19,1);
    }

    if(input.startsWith("guitar"))
    {
      input.remove(0,7);
      MIDI.sendProgramChange(30,1);
    }

    if(input.startsWith("brass"))
    {
      input.remove(0,6);
      MIDI.sendProgramChange(62,1);
    }

    delay(500);

    // ermittle Noten
    char * pch;

    pch = strtok((char*)input.c_str(), " ");

    while ((pch != NULL) && (millis() < timeout))
    {
      parser(pch, bpm);
      pch = strtok (NULL, " ");
    }
}

void loop()
{

  if(!WiFi.isConnected())
    esp_restart();

  if (!psClient.connected()) {
    mqttReconnect();
  }
  psClient.loop();  

  if(playSongFlag)
  {
    playSongFlag = false;
    playSong(song);
  }
}

void playMIDINote(byte channel, byte note, byte velocity)
{
    //MIDI channels 1-16 are really 0-15
    byte noteOnStatus=0x90 + (channel-1);  
    
    //Send notes to MIDI output:
    Serial1.write(noteOnStatus);
    Serial1.write(note);
    Serial1.write(velocity);
}


void playNote(uint16_t note, uint8_t length, uint8_t einMesslaenge, int noteVerlaengern, int noteLaengeVerbinden))
{
  MIDI.sendNoteOn(note, 127, 1);
  milisek = notenLangeInMillisekunden(length, einMesslaenge, noteVerlaengern, noteLaengeVerbinden);
  delay(milisek);
  MIDI.sendNoteOff(note, 0, 1);
}

void parser(String buffer, int8_t bpm)
{
  //Länge ganz Ton in millisekunden (1measurelength)
  int8_t einMesslaenge = ((60*4)/bpm) * 1000;

  // C', C, c, c'
  Serial.printf("Parser: %s\n", buffer.c_str());

  // Note
  char note = buffer.charAt(0);
  buffer.remove(0,1);

  // prüfe oktave
  int8_t oktaveOffset = 0;
  while(buffer.charAt(0) == '\'')
  {
    oktaveOffset++;
    buffer.remove(0,1);
  }

  // prüfe halbton
  int8_t halbton = 0;
  switch (buffer.charAt(0))
  {
    case '#':
      halbton++;
      buffer.remove(0,1);
      break;
    case 'b':
      halbton--;
      buffer.remove(0,1);
      break;
    default:
      break;
  }

  // Noten länge lesen wenn angegeben
  String str = buffer.c_str();
  if(istZahl(str))
  {
    uint8_t length = atoi(str);
    if(length == 0)
    {
      length = 4;
    }
    buffer.remove(0,str.length());
  }

  // Verlängerung der Note um 50%
  int8_t noteVerlaengern = 0;
  if(buffer.charAt(0) == ".")
  {
    noteVerlaenger = 1;
    buffer.remove(0,1);
  }
  
  // Note verbinden
  int8_t noteLaengeVerbinden = 0;
  if (buffer.charAt(0) == "_")
  {
    buffer.remove(0,1);

    char noteVerlaengerung = buffer.charAt(0);
    if(note == noteVerlaengerung)
    {
      buffer.remove(0,1);

      // Ermittle länge
      String str = buffer.c_str();
      if(istZahl(str))
      {
        noteLaengeVerbinden = atoi(str);
        if(noteLaengeVerbinden != 0)
        {
          buffer.remove(0,str.length());
        }
      }
    }
  }
  

  // Notenlaenge


  // Pause oder Note
  if(note == 'p' || note == 'P')
  {
    milisek = notenLangeInMillisekunden(length, einMesslaenge, noteVerlaengern, noteLaengeVerbinden);
    delay(milisek);
  }
  else
  {
    int8_t notenID = 0;

    switch(note)
    {
      case 'C':
        notenID = 48;
        if(halbton == 1)
          notenID++;    
        break;
      case 'D':
        notenID = 50;
        if(halbton == -1)
          notenID--;
        if(halbton == 1)
          notenID++;   
        break;
      case 'E':
          notenID = 52;
          if(halbton == -1)
            notenID--;     
        break;
      case 'F':
        notenID = 53;
        if(halbton == 1)
          notenID++;    
        break;
      case 'G':
        notenID = 55;
        if(halbton == -1)
          notenID--;
        if(halbton == 1)
          notenID++;   
        break;
      case 'A':
        notenID = 57;
        if(halbton == -1)
          notenID--;
        if(halbton == 1)
          notenID++;     
        break;
      case 'H':
      case 'B':
        notenID = 59;
        if(halbton == -1)
          notenID--;    
        break;

      case 'c':
        notenID = 60;
        if(halbton == 1)
          notenID++;  
        break;
      case 'd':
        notenID = 62;
        if(halbton == -1)
          notenID--;
        if(halbton == 1)
          notenID++;     
        break;
      case 'e':
        notenID = 64;
        if(halbton == -1)
          notenID--;  
        break;
      case 'f':
        notenID = 65;
        if(halbton == 1)
          notenID++;   
        break;
      case 'g':
        notenID = 67;
        if(halbton == -1)
          notenID--;
        if(halbton == 1)
          notenID++;  
        break;
      case 'a':
        notenID = 69;
        if(halbton == -1)
          notenID--;
        if(halbton == 1)
          notenID++;    
        break;
      case 'h':
      case 'b':
        midiNote = 71;
        if(istHalbton == -1)
          midiNote--;  
        break;
    }

    if(notenID != 0)
    {
      playNote(notenID-(oktaveOffset*12),length, einMesslaenge, noteVerlaengern, noteLaengeVerbinden);  
    }
  }
}

bool istZahl(String zahl){
  for(int index; i < zahl.length(); i++)
  {
    if(!isDigit())
    {
      return false;
    }
  }
  return true;
}

int notenLangeInMillisekunden(int laenge, int einMesslaenge, int noteVerlaengern, int noteLaengeVerbinden)
{
  int laengeMilisekunden = einMesslaenge/laenge;
  
  // Verlängerung 50%
  if(noteVerlaengern)
  {
    laengeMilisekunden += int(float(einMesslaenge/laenge) * 0.5);
  }

  // Note verbinden
  if(noteLaengeVerbinden > 0)
  {
    laengeMilisekunden += einMesslaenge/noteLaengeVerbinden;
  }

  return laengeMilisekunden;
}