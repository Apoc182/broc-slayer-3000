#define ROTATION_ENCODER 2 // This is where the input is fed.
#define REED_SWITCH 7 // button
#define PULSES_OPEN 363.333333342
#define DEBOUNCE_DELAY 50
#define DEBUG 0
#define MASTER_RELAY 12
#define DEAD_ROTATIONS 3
#define PULSES_PER_REVOLUTION 688.421052648

// 1. extract degrees into define and that should just work.
// 2. If degrees is out by more than ten, shutdown MASTER_RELAY. Make ten a define variable.
// 3. Cunt calculation. Make new potentiomere degrees added to the open and closed positions.

const int knifePins[24] = {27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48};
const int knifeCount = sizeof(knifePins) / sizeof(knifePins[0]);


int currentPulse = -1; // Variable for saving pulses count. This begins at negative 1 so when the loop starts, it can evalute from 0.
double currentPulseRemainder = 0.0;
bool active = false; // Variable for checking if the encoder is activeati
int reed_pulses_until_start = DEAD_ROTATIONS;
bool previousPulseState = false;
bool fullyRotated = false;

struct Button {
  int pin;
  int state;
  int lastState;
  unsigned long lastDebounceTime;
};

Button reed_button, rotation_button;

Button setupButton(int pin) {
  Button button;
  button.pin = pin;
  button.state = LOW;
  button.lastState = LOW;
  button.lastDebounceTime = 0;
  pinMode(pin, INPUT_PULLUP);
  return button;
}


bool debouncedDigitalRead(Button &button) {
  int reading = digitalRead(button.pin);
  bool hasRisen = false; // Default return value indicating no rising edge

  if (reading != button.lastState) {
    button.lastDebounceTime = millis();
  }

  if ((millis() - button.lastDebounceTime) > DEBOUNCE_DELAY) {
    if (reading != button.state) {
      // Button state has changed after debounce
      if (button.state == LOW && reading == HIGH) {
        // Button has transitioned from LOW to HIGH indicating a rising edge
        hasRisen = true;
      }
      button.state = reading;
    }
  }

  button.lastState = reading;
  return hasRisen;
}


void setup(){
    reed_button = setupButton(REED_SWITCH);

    // Set all the knife pins to output
    for(int i = 0; i < knifeCount; i++){
        pinMode(knifePins[i], OUTPUT);
        digitalWrite(knifePins[i], !DEBUG);
    }

    // Setup relay.
    pinMode(MASTER_RELAY, OUTPUT);
    digitalWrite(MASTER_RELAY, 0);

    Serial.begin(9600);
    Serial.println("Broc slayer 3000 engage");
}

void knifeLogic(){
  for(int i = 0; i < knifeCount; i++){
    if(knifeShouldBeOpen(i)){
        digitalWrite(knifePins[i], DEBUG);
    }else{
        digitalWrite(knifePins[i], !DEBUG);
    }
  }
}

bool knifeShouldBeOpen(int knifeIndex){
  double openPulse = (double) knifeIndex * (PULSES_PER_REVOLUTION / (double) knifeCount);
  double closePulse = round(fmod((openPulse + PULSES_OPEN), PULSES_PER_REVOLUTION));
  openPulse = round(openPulse);

  if(closePulse < openPulse){
    return ((currentPulse <= closePulse && fullyRotated) || currentPulse >= openPulse);
  }

  return currentPulse <= closePulse && currentPulse >= openPulse;

}

void loop(){ 

    // Assuming the counter should not start until the reed_switch goes high
    if(!active && !DEBUG){
        if(debouncedDigitalRead(reed_button)){
            reed_pulses_until_start -= 1;
            if(!reed_pulses_until_start){
              Serial.println("Main loop activated");
              digitalWrite(MASTER_RELAY, 1);
              active = true;            
            } 
        }else{
            return;
        }
    }


    // Now the main loop is active, we should not do anything until a pulse happens.
    bool pulse_receieved = digitalRead(ROTATION_ENCODER) == HIGH;


    if(pulse_receieved == previousPulseState){
        return;
    }

    previousPulseState = pulse_receieved;

    if(!pulse_receieved){
        return;
    }


    // If it has been received, increment the counter and evalate knives.
    currentPulse = currentPulse + 1;
    if(currentPulse > floor(PULSES_PER_REVOLUTION)){
      currentPulse = 0;
      fullyRotated = true;
      currentPulseRemainder += PULSES_PER_REVOLUTION - floor(PULSES_PER_REVOLUTION);
    }

    knifeLogic();

    if(currentPulseRemainder >= 1){
      currentPulseRemainder -= 1;
      currentPulse += 1;
      knifeLogic();
    }

    if(DEBUG){
      Serial.println("*");
      Serial.println("currentPulse:" + String(currentPulse));
      Serial.println("currentPulseRemainder:" + String(currentPulseRemainder));
    }


}
