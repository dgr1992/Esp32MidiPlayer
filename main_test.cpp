#include <cstring>
#include <string>
#include <stdint.h>
#include <stdio.h>
#include<time.h>

#define PING_TIMEOUT 120 * 1000
#define DEBUG true

const std::string WHITESPACE = " \n\r\t\f\v";
bool playSongFlag = false;
std::string song;

void parser(std::string buffer, int bpm);
void playNote(int note, int length, int einMesslaenge, int noteVerlaengern, int noteLaengeVerbinden);
void playSong(std::string input);
bool istZahl(std::string zahl);
int notenLangeInMillisekunden(int laenge, int einMesslaenge, int noteVerlaengern, int noteLaengeVerbinden);
int extrahiereZahl(std::string* text);
std::string ltrim(const std::string& s);
int isCapital(char x);

int main()
{
    std::string midinotes = "bpm128 piano G2 g'2 G#2 Gb2 G#2. G#2_G4";
    playSong(midinotes);
}

void playSong(std::string input)
{
    printf("Play erkannt\n");

    //Prüfe BPM
    int bpm = 240; //Rückwertskompatibilität
    if(input.rfind("bpm", 0) == 0)
    {
      input.erase(0,3);
      
      int tmp = extrahiereZahl(&input);
      if(tmp > 0)
      {
        bpm = tmp;
      }
    }

    input = ltrim(input);

    // Prüfe das Instrument
    if(input.rfind("piano", 0) == 0)
    {
      input.erase(0,6);
      //MIDI.sendProgramChange(0,1);
    }

    if(input.rfind("vibes", 0) == 0)
    {
      input.erase(0,6);
      //MIDI.sendProgramChange(11,1);
    }

    if(input.rfind("organ", 0) == 0)
    {
      input.erase(0,6);
      //MIDI.sendProgramChange(19,1);
    }

    if(input.rfind("guitar", 0) == 0)
    {
      input.erase(0,7);
      //MIDI.sendProgramChange(30,1);
    }

    if(input.rfind("brass", 0) == 0)
    {
      input.erase(0,6);
      //MIDI.sendProgramChange(62,1);
    }

    // ermittle Noten
    char * pch;

    pch = strtok((char*)input.c_str(), " ");

    while ((pch != NULL))
    {
      parser(pch, bpm);
      pch = strtok (NULL, " ");
    }
}

void playNote(int note, int length, int einMesslaenge, int noteVerlaengern, int noteLaengeVerbinden)
{
  printf("Play: %i, 127, 1\n", note);
  int milisek = notenLangeInMillisekunden(length, einMesslaenge, noteVerlaengern, noteLaengeVerbinden);
  printf("Playtime [ms]: %i\n", milisek);
  printf("Stop: %i, 0, 1\n\n", note);
}

void parser(std::string buffer, int bpm)
{
  //Länge ganz Ton in millisekunden (1measurelength)
  int einMesslaenge = ((float)(60*4)/(float)bpm) * 1000;
  
  printf("Parser: %s\n",buffer.c_str());
  
  // Note
  char note = buffer[0];
  buffer.erase(0,1);

  // prüfe oktave
  int oktaveOffset = 0;
  while(buffer[0] == '\'')
  {
    oktaveOffset++;
    buffer.erase(0,1);
  }

  // prüfe halbton
  int halbton = 0;
  switch (buffer[0])
  {
    case '#':
      halbton++;
      buffer.erase(0,1);
      break;
    case 'b':
      halbton--;
      buffer.erase(0,1);
      break;
    default:
      break;
  }

  // Noten länge lesen wenn angegeben
  int length = extrahiereZahl(&buffer);
  if(length == 0)
  {
    length = 4;
  }

  // Verlängerung der Note um 50%
  int noteVerlaengern = 0;
  if(buffer[0] == '.')
  {
    noteVerlaengern = 1;
    buffer.erase(0,1);
  }
  
  // Note verbinden
  int noteLaengeVerbinden = 0;
  if (buffer[0] == '_')
  {
    buffer.erase(0,1);

    char noteVerlaengerung = buffer[0];
    if(note == noteVerlaengerung)
    {
      buffer.erase(0,1);

      // Ermittle länge
      noteLaengeVerbinden = extrahiereZahl(&buffer);
      if(length == 0)
      {
        length = 4;
      }
    }
  }

  // Pause oder Note
  if(note == 'p' || note == 'P')
  {
    int milisek = notenLangeInMillisekunden(length, einMesslaenge, noteVerlaengern, noteLaengeVerbinden);
  }
  else
  {
    int notenID = 0;

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
        notenID = 71;
        if(halbton == -1)
          notenID--;  
        break;
    }

    if(notenID != 0)
    {
      if(isCapital(note))
      {
        playNote(notenID-(oktaveOffset*12),length, einMesslaenge, noteVerlaengern, noteLaengeVerbinden);
      }
      else
      {
        playNote(notenID+(oktaveOffset*12),length, einMesslaenge, noteVerlaengern, noteLaengeVerbinden);
      }
    }
  }
}

bool istZahl(std::string zahl){
  std::string::const_iterator it = zahl.begin();
  while(it != zahl.end() && std::isdigit(*it))
  {
    it++;
  }
  return !zahl.empty() && it == zahl.end();
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

int extrahiereZahl(std::string* text)
{
  int zahl = 0;
  int charPositionLaengeEnde = 0;

  //Suche index wo zahl auf hört
  std::string::const_iterator it = (*text).begin();
  while(it != (*text).end() && std::isdigit(*it))
  {
    it++;
    charPositionLaengeEnde++;
  }

  //Prüfe ob zahl erkannt
  if(charPositionLaengeEnde > 0)
  {
    zahl = atoi((*text).substr(0, charPositionLaengeEnde).c_str());
    if(zahl == 0)
    {
      zahl = 4;
    }
    (*text).erase(0,charPositionLaengeEnde);
  }

  return zahl;
}

std::string ltrim(const std::string& s)
{
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

int isCapital(char x)
{
  if (x >='A' && x <= 'Z')
  { 
    return 1;
  }
  else
  {
    return 0;
  }
}