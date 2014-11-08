// SpeechDetector implements a detector for spoken words.
//
//  Copyright (C) 2014 Nicola Cimmino
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//   This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see http://www.gnu.org/licenses/.
//
#define AUDIO_IN_PIN A0

#define AMP_PWR_PIN A3

#define SAMPLES_BUFFER_SIZE 32

#define COMPLEXITY_FILTER_LEN 6

#define FINGERPRINT_SIZE 64

void setup()
{
  Serial.begin(115200); 
  pinMode(AUDIO_IN_PIN, INPUT);
  pinMode(AMP_PWR_PIN,OUTPUT);
  digitalWrite(AMP_PWR_PIN, HIGH);
  
  // This sets the ADC analog reference to internally
  //  generated 1.1V to increase A/D resolution.
  analogReference(INTERNAL);
}

int samplesBuffer[SAMPLES_BUFFER_SIZE];

int complexityFilter[COMPLEXITY_FILTER_LEN];

byte complexityFilterIndex=0;

long lastUtteranceTime = 0;

char lastPhoneme='\0';

byte referenceFingerprint_a[FINGERPRINT_SIZE];

byte referenceFingerprint_b[FINGERPRINT_SIZE];

byte fingerprint[FINGERPRINT_SIZE];

byte fingerprintLength_a = 0;

byte fingerprintLength_b = 0;

void loop()
{
    delay(15000);
    Serial.println("Learning....");
    fingerprintLength_a = getVoiceFingerprint();
    memcpy(referenceFingerprint_a, fingerprint, fingerprintLength_a);
    Serial.print("Fingerprint len:");
    Serial.println(fingerprintLength_a);
    delay(1000);
    Serial.println("Learning....");
    fingerprintLength_b = getVoiceFingerprint();
    memcpy(referenceFingerprint_b, fingerprint, fingerprintLength_b);
    Serial.print("Fingerprint len:");
    Serial.println(fingerprintLength_b);
    Serial.println("Listening....");
    
    while(true)
    {
      getVoiceFingerprint();
      int correlation_a=0;
      for(int ix=0;ix<fingerprintLength_a;ix++)
      {
         correlation_a+=abs(referenceFingerprint_a[ix]-fingerprint[ix]);
      }   
      correlation_a=correlation_a/fingerprintLength_a;
      
      int correlation_b=0;
      for(int ix=0;ix<fingerprintLength_b;ix++)
      {
         correlation_b+=abs(referenceFingerprint_b[ix]-fingerprint[ix]);
      }   
      correlation_b=correlation_b/fingerprintLength_b;
      
      Serial.print((correlation_a<correlation_b)?"A":"B");
      Serial.print(" ");
      Serial.print(correlation_a);
      Serial.print(" ");
      Serial.println(correlation_b);
    } 
}

int averageInputLevel()
{
  int inputLevel=0;
  for(int ix=0;ix<SAMPLES_BUFFER_SIZE;ix++)
  {
    inputLevel += analogRead(AUDIO_IN_PIN);
  }
  inputLevel=inputLevel/SAMPLES_BUFFER_SIZE;
  
  return inputLevel;
}

byte getVoiceFingerprint()
{
    while(averageInputLevel()<60);
    
    byte fingerprintIndex=0;
    int lastComplexity=0;
    lastUtteranceTime=millis();
    while(true)
    {
      // Sample 
      for(int ix=0;ix<SAMPLES_BUFFER_SIZE;ix++)
      {
        samplesBuffer[ix] = analogRead(AUDIO_IN_PIN);
      }
      
      // Evaluate complexity
      int complexity=0;
      uint64_t totalEnergy=0;
      int maxEnergy=0;
      for(int ix=1;ix<SAMPLES_BUFFER_SIZE;ix++)
      {
        complexity+=abs(samplesBuffer[ix]-samplesBuffer[ix-1]);
        totalEnergy+=samplesBuffer[ix];
        if(samplesBuffer[ix]>maxEnergy)
        {
          maxEnergy=samplesBuffer[ix];
        }
      }
      complexity=complexity/(totalEnergy/SAMPLES_BUFFER_SIZE);
      
      if(maxEnergy<40)
      {
        if(millis()-lastUtteranceTime>1500)
        {
          return fingerprintIndex;
        }
        continue; 
      }
      
      lastUtteranceTime = millis();
      
      complexityFilter[complexityFilterIndex]=complexity;
      complexityFilterIndex=(complexityFilterIndex+1)%COMPLEXITY_FILTER_LEN;
      
      int filteredComplexity=0;
      for(int ix=0;ix<COMPLEXITY_FILTER_LEN;ix++)
      {
        filteredComplexity+=complexityFilter[ix]; 
      }
      filteredComplexity=filteredComplexity/COMPLEXITY_FILTER_LEN;
      
      if(lastComplexity==0 || abs(lastComplexity-filteredComplexity)>0)
      {
        lastComplexity=filteredComplexity;
        fingerprint[fingerprintIndex]=filteredComplexity;
        fingerprintIndex++;
        if(fingerprintIndex==FINGERPRINT_SIZE)
        {
          return fingerprintIndex;  
        }
      }
   }  
}
