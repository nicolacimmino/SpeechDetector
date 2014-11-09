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

// Hardware connections
#define AUDIO_IN_PIN A0
#define LED_GREEN_PIN 2
#define LED_RED_PIN 3
#define AMP_PWR_PIN A3

// Size of buffer used to sample audio
#define SAMPLES_BUFFER_SIZE 16

// Length of the running average filter
// for the complexity.
#define COMPLEXITY_FILTER_LEN 6

// Maximum size of a fingerprint for a word.
// Affects max duration of the spoken word.
#define MAX_FINGERPRINT_SIZE 128

// Minimum input level below which we consider
// stuff coming in as noise.
#define SPEECH_THRESHOLD 100

void setup()
{
  Serial.begin(115200); 
  
  pinMode(AUDIO_IN_PIN, INPUT);
  
  // Power up the mic pre-amp
  pinMode(AMP_PWR_PIN,OUTPUT);
  digitalWrite(AMP_PWR_PIN, HIGH);
  
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_RED_PIN, OUTPUT);
  digitalWrite(LED_GREEN_PIN, LOW);
  digitalWrite(LED_RED_PIN, LOW);
  
  // This sets the ADC analog reference to internally
  //  generated 1.1V to increase A/D resolution.
  analogReference(INTERNAL);
  
  // Let things settle down, there is apparently
  // a "pop" when powering things up, probalby
  // due to the mic decoupling cap.
  delay(2000);
}

// Latest speech sample.
int samplesBuffer[SAMPLES_BUFFER_SIZE];

// The complexity filter is implemented as a 
// circular buffer, this is the buffer and 
// the index.
int complexityFilter[COMPLEXITY_FILTER_LEN];
byte complexityFilterIndex=0;

// Fingerprints of the the two learned words and 
// their lengths.
byte referenceFingerprint_a[MAX_FINGERPRINT_SIZE];
byte referenceFingerprint_b[MAX_FINGERPRINT_SIZE];
byte fingerprintLength_a = 0;
byte fingerprintLength_b = 0;

// Current speech sample fingerprint.
byte fingerprint[MAX_FINGERPRINT_SIZE];


void loop()
{
  // During the learning phase we collect the fingeprints
  // of two spoken words. User needs for the time being
  // to follow the serial output to know when to speak.
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
    
    // Evaluate correlation against each reference fingerprint
    // technically this is the reciprocal of correlation but 
    // this siplifies our calculations.
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
    
    // Just for debug we show the nearest match and the correlation values
    Serial.print((correlation_a<correlation_b)?"A":"B");
    Serial.print(" ");
    Serial.print(correlation_a);
    Serial.print(" ");
    Serial.println(correlation_b);
    
    // Briefly flash the LED matching the spoken word.
    digitalWrite((correlation_a<correlation_b)?LED_RED_PIN:LED_GREEN_PIN, HIGH);
    delay(500);
    digitalWrite((correlation_a<correlation_b)?LED_RED_PIN:LED_GREEN_PIN, LOW);

  } 
}

/*
 * Returns the average level currently
 * present at the microphone input.
 * Currently performs an average as long
 * as SAMPLES_BUFFER_SIZE so it will be
 * consistent with sampled signal.
 */
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

/*
 * Waits for speech to be present at the
 * mic input and then evaluates up to MAX_FINGERPRINT_SIZE
 * speech blocks and stores them as current fingerprint.
 * If the speech ends before MAX_FINGERPRINT_SIZE is reached
 * the collection stops. Actual taken samples are returned.
 */
byte getVoiceFingerprint()
{
    // Wait for user to start speaking.
    while(averageInputLevel()<SPEECH_THRESHOLD)
    {
      delay(10);
    }
    
    byte fingerprintIndex=0;
    long lastUtteranceTime=millis();
    while(true)
    {
      
      // Collect a sample, calculate complexity and filter it.
      int maxEnergy=sampleAudio();     
      int complexity=calculateComplexity();
      complexity = filterComplexity(complexity);
      
      // There was no speech, don't take a fingerprint sample.
      // If the situation perists for more than 500mS the user
      // has finished to speak so we return.
      if(maxEnergy<SPEECH_THRESHOLD)
      {
        if(millis()-lastUtteranceTime>500)
        {
          return fingerprintIndex;
        }
        continue; 
      }
      
      lastUtteranceTime = millis();
      
      fingerprint[fingerprintIndex]=complexity;
      fingerprintIndex++;
      
      if(fingerprintIndex==MAX_FINGERPRINT_SIZE)
      {
        return fingerprintIndex;  
      }
   }  
}

/*
 * Takes SAMPLES_BUFFER_SIZE audio samples
 * and returns the max level seen.
 */
int sampleAudio()
{
  int maxEnergy=0;
  for(int ix=0;ix<SAMPLES_BUFFER_SIZE;ix++)
  {
    samplesBuffer[ix] = analogRead(AUDIO_IN_PIN);
    if(samplesBuffer[ix]>maxEnergy)
    {
      maxEnergy=samplesBuffer[ix];
    }
  } 
  return maxEnergy;
}

/* Calculates the complexity in the
 * data currently in the sample buffer.
 */
int calculateComplexity()
{
  int complexity=0;
  uint64_t totalEnergy=0;
  for(int ix=1;ix<SAMPLES_BUFFER_SIZE;ix++)
  {
    complexity+=abs(samplesBuffer[ix]-samplesBuffer[ix-1]);
    totalEnergy+=samplesBuffer[ix];
  }
  complexity=complexity/(totalEnergy/SAMPLES_BUFFER_SIZE);  
  
  return complexity;
}

/*
 * Apply filter to one complexity sample and 
 * return filter output.
 */
int filterComplexity(int complexity)
{
  // Store the new value in the circular buffer.
  complexityFilter[complexityFilterIndex]=complexity;
  complexityFilterIndex=(complexityFilterIndex+1)%COMPLEXITY_FILTER_LEN;
  
  // Compute average.
  int filteredComplexity=0;
  for(int ix=0;ix<COMPLEXITY_FILTER_LEN;ix++)
  {
    filteredComplexity+=complexityFilter[ix]; 
  }
  filteredComplexity=filteredComplexity/COMPLEXITY_FILTER_LEN;
  
  return filteredComplexity;
}

