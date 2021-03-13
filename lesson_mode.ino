/*
 * ARDUINO PIANO by Raphaël Velt, 13 March 2020
 */

/* In this version, LEDs light up to show you which notes to play next */

/* That's how many keys the piano has */
const int NOTE_COUNT = 6;
/* That's how many samples I'm averaging the luminosity value on */
const int AVG_LENGTH = 64;
/* The size of a big array where I'm storing instantaneous values of luminosity for each key */
const int SENSOR_VALUE_LENGTH = NOTE_COUNT * AVG_LENGTH;
/* The threshold to detect a key being activated (20% less luminosity than rolling average) */
const double DOWN_THRESHOLD = .8;
/* The threshold to detect a key being uactivated (5% more luminosity less than rolling average) */
const double UP_THRESHOLD = 1.05;

/* MUSIC SCORES */ 

/* MARY HAD A LITTLE LAMB *
const int MUSIC_LENGTH = 26;
int music[MUSIC_LENGTH] = {2, 1, 0, 1, 2, 2, 2, 1, 1, 1, 2, 4, 4, 2, 1, 0, 1, 2, 2, 2, 2, 1, 1, 2, 1, 0};
/* */

/* TWINKLE TWINKLE LITTLE STAR */
/* I tried using sizeof() but it would overrun the array */
const int MUSIC_LENGTH = 41;
int music[MUSIC_LENGTH] = {0, 0, 4, 4, 5, 5, 4, 3, 3, 2, 2, 1, 1, 0, 4, 4, 3, 3, 2, 2, 1, 4, 4, 3, 3, 2, 2, 1, 0, 0, 4, 4, 5, 5, 3, 3, 2, 2, 1, 1, 0};
/* */

/* Keeps track of which note is meant to be played */
int current_note = -1;

/* The frequencies for notes C, D, E, F, G and A */
int frequencies[NOTE_COUNT] = {262, 294, 330, 350, 392, 440};
/* The pins where photocells are connected. In my case, A0 for C, A1 for D, etc. */
int pins[NOTE_COUNT] = {A0, A1, A2, A3, A4, A5};
/* The pins where the LEDs corresponding to each key are connected: 7 for C, 6 for D, etc. */
int led_pins[NOTE_COUNT] = {7, 6, 5, 4, 3, 2};
/* The pin where the loudspeaker/buzzer is connected */
int buzzer_pin = 8;

/* When the song is complete, the Arduipiano plays a C major chord */
int winning[] = {262, 330, 392, 523};


/* Keeps track of which keys are in a pressed status */
bool keys_pressed[NOTE_COUNT];
/* Keeps track of the current luminosity value for each key */
int current_values[NOTE_COUNT];
/* Keeps track of luminosity values over time to produce a rolling average */
int sensor_values[SENSOR_VALUE_LENGTH];
/* Keeps track of the latest updated value in the rolling average */
int pointing_positions = 0;
/* Keeps track of whether the array is full enough to produce a rolling average */
int values_initialized = false;
/* Keeps track of when a key started playing - this ensures that a note isn't played for more than a second (because it's annoying - can probably be removed */
unsigned long last_press = 0;

/* Keeps track of what note is currently playing */
int is_playing = -1;

/* A function to start the song */
void restart() {
  /* Lighting up all LEDs */
  for (int k=0; k<NOTE_COUNT; k++) {
    digitalWrite(led_pins[k], HIGH);
  }
  /* Playing the "chord of victory" */
  for (int i = 0; i<4; i++) {
    tone(buzzer_pin, winning[i], 200);
    delay(200);
  }
  /* Resetting to the start of the song */
  current_note = -1;
  showNextNote();
}

/* A function that lights up the LED for the next note in the song */
void showNextNote() {
  /* Turns off all LEDs */
  for (int i=0; i<NOTE_COUNT;i++) {
    digitalWrite(led_pins[i], LOW);
  }
  /* Moves up one note in the array */
  current_note += 1;
  if (current_note < MUSIC_LENGTH) {
    /* Light up the LED corresponding to the note to play */
    digitalWrite(led_pins[music[current_note]], HIGH);
  } else {
    /* If the song is over, restart */
    restart();
  }
}

/* The classic Arduino "Setup" function, running once at the start */
void setup() {
  for (int i=0; i<NOTE_COUNT;i++) {
    /* Initialize the status of keys */
    keys_pressed[i] = false;
    /* Make sure that LEDs can be lit */
    pinMode(led_pins[i], OUTPUT);
  }
  /* Using the Builtin LED to keep track of where a key is pressed or not */
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  restart();
}

void loop() {
  /* Writing down time - to avoid playing a note too long */
  unsigned long now = millis();
  
  /* Writing down the luminosity values (photocell input) for each key */
  for (int i=0; i<NOTE_COUNT;i++) {
    int value = analogRead(pins[i]);
    current_values[i] = value;
    /* Adding the value to the array that calculates the rolling average */
    sensor_values[i*AVG_LENGTH + pointing_positions] = value;
  }

  
  /* Key detection is only done when an average of 64 values is available */
  if (values_initialized) {
    for (int i=0; i<NOTE_COUNT; i++) {
      /* We calculate the rolling average */
      unsigned long sum = 0;
      for (int j=0; j < AVG_LENGTH; j++) {
         sum += sensor_values[i*AVG_LENGTH + j];
      }
      double average_value = sum/AVG_LENGTH;
      /* We compare the rolling average with the current value */
      double ratio = current_values[i]/average_value;
      
      /* If there is 5% more luminosity, we assume that fingers have been taken off the key */
      if (ratio > UP_THRESHOLD) {
        /* The status of the key is updated */
        keys_pressed[i] = false;
        /* Bultin LED is turned off */
        digitalWrite(LED_BUILTIN, LOW);
      }

      /* If there is 20% less luminosity, we assume that fingers have been put on the key */
      if (ratio < DOWN_THRESHOLD) {
        /* All other keys are considered released */
        for (int k=0; k<NOTE_COUNT; k++) {
          keys_pressed[k] = false;
        }
        /* The status of the key is updated */
        keys_pressed[i] = true;
        /* Bultin LED is turned on */
        digitalWrite(LED_BUILTIN, HIGH);
        last_press = now;
      }
      
      
    }

    /* Checking which note is playing */
    int was_playing = is_playing;
    is_playing = -1;
    
    for (int i=0; i<NOTE_COUNT; i++) {
      if (keys_pressed[i]) {
        if (now-last_press > 1000) {
          /* Releasing key if a note has been playing too long */
          keys_pressed[i] = false;
          digitalWrite(LED_BUILTIN, LOW);
        } else {
          is_playing = i;
          /* Playing sound */
          tone(buzzer_pin, frequencies[i], 40);
        }
      }
    }
    
    if (was_playing != is_playing) {
      /* What happens when a key is released (or we move on directly to the next key */
      if (is_playing < 0) {
        if (was_playing == music[current_note]) {
          /* If the key was right, let's light up the next note (which could be the same) */
          showNextNote();
        } else {
          /* A "bad note" buzzer sound */
          tone(buzzer_pin, 50, 200);
        }
      }
    
    }
    
  }
  
  /* Housekeeping the position of the pointer for rolling average */
  pointing_positions++;
  if (pointing_positions >= AVG_LENGTH) {
    values_initialized = true;
    pointing_positions = 0;
  }
}
