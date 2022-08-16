/*
Decodeur morse

Le volet (géré par les relais) descend, se stoppe ou monte soit: -grâce à l'infrarouge
                                                                 -grâce au temps
*/
#include <LiquidCrystal.h>

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

const char recepteurIr = A0;
const char relaisD = 9; //relais LOW quand descente
const char relaisS = 8; //relais LOW pour descendre et HIGH pour monter
const char relaisM = 7; //relais LOW pour monter
const char retroE =  13; //pour retroéclairage (à modif sur pin pwm)
long pendule = 82800; //heure en sec
int position = 0; //stocke la position du volet

struct dictionnaireEntree   //structure du dictionnaire
{
	char caractere;
	String signal;
};
dictionnaireEntree dictionnaire[36] =
{
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



class Decode
{
	String code, mot; //seront complétés au fur et à mesure du décryptage
	unsigned long debutAppui, debutRelache; //pour mesurer longueur des fronts montant et descendant
	bool rajouterDash; //necessité de rajout de dash ou pas
	long uniteDeTps;


public:
	Decode()
	{
		code = "", mot = "";
		debutAppui = 0, debutRelache = 0;
		uniteDeTps = 5;
	}

	void update()
	{
		if (digitalRead(recepteurIr) == LOW)   //GERE LES SIGNAUX
		{
			debutAppui = millis();
			completeCode();
			debutRelache = millis();
		}
		else
		{
			if (codeTermine())
			{
				completeMot(); //dès que code obtenu, lettre correspondante s'ajoute au mot
				if (!motTermine()) code = ""; //réinitialise code pour obetnir lettre suivante
			}
			if (motTermine())
			{
				mot = "";
				code = "";
			}
		}
	}

	bool correspond(String sequence)
	{
		if (mot.substring(mot.length() - sequence.length(), mot.length()) == sequence) {return true; } 
		else return false;
	}
	

private:
	void dot()   //ajoute le caractere dot a code
	{
		code += ".";
	}

	void dash()   //ajoute le caractere dash a code
	{
		code += "-";
	}

	char getLettre(String code)   //permet a partir du code d'obtenir la lettre
	{
		for (int i = 0; i < 36; i++)
			if (dictionnaire[i].signal == code) return dictionnaire[i].caractere;
		return ' ';
	}

	void completeMot()
	{
		mot += getLettre(code);
	}

	void completeCode()
	{
		while (digitalRead(recepteurIr) == LOW)   //tant qu'il y a signal
		{
			if (millis() - debutAppui <= uniteDeTps + 1)	rajouterDash = false; //comparaison longueur signal pour savoir si dot ou dash
			else rajouterDash = true;
		}
		if (rajouterDash) dash();
		else dot();
	}

	bool motTermine()   //vrai si temps suffisant pour determiner fin d'un mot
	{
		if (millis() - debutRelache >= uniteDeTps * 7 - 1 && mot != "")	return true; //marge d'erreur
		return false;
	}

	bool codeTermine()   //vrai si temps suffisant pour fin du code/lettre
	{
		if (millis() - debutRelache >= uniteDeTps * 3 - 1 && code != "") return true;
		return false;
	}

};



class Clock
{
	unsigned long heureApres, heureAvant, timer1, timer2, total, temp;
	short HH, MM, sec;
	bool changement;


public:
	Clock()
	{
		timer1 = millis();
	}

	void update()
	{
		changement = false;
		timer2 = millis();
		if (timer2 - timer1 >= 1000)
		{
			pendule++;
			timer1 = timer1 + 1000;
			changement = true;
		}
		if (pendule >= 86400) pendule = pendule - 86400;
		else if (pendule < 0) pendule = abs(pendule + 86400);
		if (changement == true)
		{
			miseEnForme();
			affichage();
			//lcd.clear();
			//lcd.print(position/15000);
			changement = false;
		}
	}
	

private:
	void affichage()
	{
		lcd.clear();
		lcd.print(HH);
		lcd.setCursor(3, 0);
		lcd.print(":");
		lcd.setCursor(5, 0);
		lcd.print(MM);
		lcd.setCursor(8, 0);
		lcd.print(":");
		lcd.setCursor(10, 0);
		lcd.print(sec);
	}
	void miseEnForme()
	{
		HH = pendule / 3600;
		temp = pendule % 3600;
		MM = temp / 60;
		sec = temp % 60;
	}

};



Decode morse;

class Volet
{
	int state; //pour savoir l'action en cours du volet: 1 descendre, 2 monter, 0 stop
	unsigned long timerUp, timerDown; //POUR LIMITER LE TPS DES ACTIONS
	bool automat;
	int positionVoulue;
	

public:
	Volet()
	{
		state = 0;
		timerUp = 0;
		timerDown = 0;
		automat = false;
		positionVoulue = 0;
	}
	void update()
	{
		action();
		if (timerUp != 0)   //si il est activé
		{
			if (position < 15000)   //si position n'est pas au max
			{
				if (position >= 15000)
				{
					position = 15000;
					timerUp = 0;
				}
				else
				{
					position += (millis() - timerUp);
					timerUp = millis();
				}
			}
		}
		else if (timerDown != 0)
		{
			if (position > 0)   //si position n'est pas au min
			{
				if (position <= 0)
				{
					position = 0;
					timerDown = 0;
				}
				else
				{
					position -= (millis() - timerDown);
					timerDown = millis();
				}
			}
		}
		modeAuto();
		if (automat == false) { //arrete le timer si up ou down est activé
		                        //car sinon il ne s'arrete pas en mode manuel
			if (position >= 15000 || position <= 0) {
				stop_();
			}
		}
	}
	
	
private:
	void action()   //execute les actions en fonction des interractions
	{
		if (morse.correspond("up"))
		{
			up();
			automat = false;
			delay(2);
		}
		else if (morse.correspond("down"))
		{
			down();
			automat = false;
			delay(2);
		}
		else if (morse.correspond("stop"))
		{
			stop_();
			automat = false;
			delay(2);
		}
		else if (morse.correspond("min"))
		{
			automat = false;
			pendule += 60;
			delay(100);
		}
		else if (morse.correspond("hour"))
		{
			automat = false;
			pendule += 3600;
			delay(100);
		}
	}
	void down()   //RAJOUT
	{
		state = 1;
		digitalWrite(relaisD, LOW);
		digitalWrite(relaisM, HIGH);
		digitalWrite(relaisS, LOW);
		timerDown = millis();
		if (timerUp != 0)   //si le timer est activé
		{
			timerUp = 0;
		}
	}

	void up()   //RAJOUT
	{
		state = 2;
		digitalWrite(relaisD, HIGH);
		digitalWrite(relaisM, LOW);
		digitalWrite(relaisS, HIGH);
		timerUp = millis();
		if (timerDown != 0)
		{
			timerDown = 0;
		}
	}

	void stop_()   //RAJOUT
	{
		state = 0;
		digitalWrite(relaisD, HIGH);
		digitalWrite(relaisM, HIGH);
		digitalWrite(relaisS, HIGH);
		if (timerUp != 0)   //si le timer est activé
		{
			timerUp = 0;
		}
		else if (timerDown != 0)
		{
			timerDown = 0;
		}
		positionVoulue = position;
	}
	void modeAuto()   //gere le mode auto
	{
		switch(pendule)
		{
			case 28800: //8h00 decolle les lames
			  automat = true;
				positionVoulue = 1100;
				break;
			case 30000: //8h20 moitie
			  automat = true;
				positionVoulue = 7500;
				break;
			case 30900: //8h35 completement
			  automat = true;
				positionVoulue = 15000;
				break;
			case 75600: //21h00
			  automat = true;
				positionVoulue = 7500;
				break;
			case 77400: //21h30
				positionVoulue = 4000;
				automat = true;
				break;
			case 79200: //22h00
			  automat = true;
				positionVoulue = 0;
				break;
		}
		if (automat == true)
		{
			if ((position < positionVoulue && state == 0) || (position < positionVoulue && state == 1))
			{
				up();
			}
			else if ((position > positionVoulue && state == 0) || (position > positionVoulue && state == 2))
			{
				down();
			}
			else
			{
				if (position == positionVoulue)
				{
					stop_();
					automat = false;
				}
			}
		}
	}
};


Clock clock;
Volet volet;

void setup() {
  
	lcd.begin(16, 2);
	lcd.clear();
	lcd.println("demarrage...");
	pinMode(recepteurIr, INPUT);
	pinMode(relaisD, OUTPUT);
	pinMode(relaisS, OUTPUT);
	pinMode(relaisM, OUTPUT);
	pinMode(retroE, OUTPUT);
	digitalWrite(relaisM, HIGH);
	digitalWrite(relaisD, HIGH);
	digitalWrite(relaisS, HIGH);
	digitalWrite(retroE, HIGH);
	delay(50); //histoire de temporiser le tout
	lcd.clear();
}

void loop() {
  
	morse.update();
	volet.update();
	clock.update();
}