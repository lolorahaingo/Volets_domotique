/*
Telecommande morse
*/

#define frequence 38000

long uniteDeTps = 5;
const char btnM = 2;
const char btnD = 4;
const char btnE = 5;
const char btnR = 6;
bool btnState = true; //APPUI == LO
bool lastBtnState = true;
const char led = 3;
unsigned long debut;
bool reglage = false;

struct dictionnaireEntree { //deux types de var ds le tableau
  char caractere;
  String signal;
};
dictionnaireEntree dictionnaire[36] = {
  {'a', ".-"},
  {'b', "-..."},
  {'c', "-.-."},
  {'d', "-.."},
  {'e', "."},
  {'f', "..-."},
  {'g', "--."},
  {'h', "...."},
  {'i', ".."},
  {'j', ".---"},
  {'k', "-.-"},
  {'l', ".-.."},
  {'m', "--"},
  {'n', "-."},
  {'o', "---"},
  {'p', ".--."},
  {'q', "--.-"},
  {'r', ".-."},
  {'s', "..."},
  {'t', "-"},
  {'u', "..-"},
  {'v', "...-"},
  {'w', ".--"},
  {'x', "-..-"},
  {'y', "-.--"},
  {'z', "--.."},
  
  {'0', "-----"},
  {'1', ".----"},
  {'2', "..---"},
  {'3', "...--"},
  {'4', "....-"},
  {'5', "....."},
  {'6', "-...."},
  {'7', "--..."},
  {'8', "---.."},
  {'9', "----."},
};

void dot() {
  delay(uniteDeTps);
  tone(led, frequence);
  digitalWrite(13, HIGH);
  delay(uniteDeTps);
  noTone(led);
  digitalWrite(13, LOW);
}

void dash() {
  delay(uniteDeTps);
  tone(led, frequence);
  digitalWrite(13, HIGH);
  delay(3 * uniteDeTps);
  noTone(led);
  digitalWrite(13, LOW);
}

void finLettre() {
  delay(3 * uniteDeTps);
  noTone(led);
  digitalWrite(13, LOW);
}

void finMot() {
  delay(7 * uniteDeTps);
  noTone(led);
  digitalWrite(13, LOW);
}

String signalDe(char lettre) {
  for (int i = 0; i < 36; i++) {
    if (dictionnaire[i].caractere == lettre) //si ça correspond à quelque chose
      return dictionnaire[i].signal; //est convertit en string
  }
}

void envoie(String mot){
  for (int i = 0; i < mot.length(); i++) { //pour chaque lettre
    String signal = signalDe(mot[i]); //signal de la lettre   
    for (int t = 0; t < signal.length(); t++) { //pour chaque caractere du signal
      switch (signal[t]) { //allume ou eteint la led suivant la nature du caractere
        case '.':
          dot();
          break;
        case '-':
          dash();
          break;
        case ' ':
          finMot();
          break;
      }
    }
    finLettre();
  }
}

void setup() {
    pinMode(btnM, INPUT);
    pinMode(btnD, INPUT);
    pinMode(btnE, INPUT);
    pinMode(btnR, INPUT);
    pinMode(led, OUTPUT);
}

void loop() {
	btnState = digitalRead(btnR);
	
	if (btnState != lastBtnState) { //attention: il faut detecter un appui
		if (btnState == LOW) {
		  if (reglage == true) {	 
		  	reglage = false; 
		  }
		  else {
		  	reglage = true; 
		  }
		  //envoie("appui");
		}
		lastBtnState = btnState;
	}
	
	if (reglage == false) {
		if(digitalRead(btnM) == LOW) { 
		    envoie("up");
		}
	    if(digitalRead(btnD) == LOW) { 
		    envoie("down");
	    }
		if(digitalRead(btnE) == LOW) { 
		    envoie("stop");
		}
	}
	
  	else if (reglage == true) {
  		if(digitalRead(btnM) == LOW) { 
		    envoie("min");
		    delay(70);
		}
	    if(digitalRead(btnD) == LOW) { 
		    envoie("hour");
		    delay(70);
	    }
  	}
}