#include <LiquidCrystal.h>

const int BuzzerPin = 10;               // Broche du buzzer
const int LedRouge = 12;                // Broche de la LED rouge
const int LedVerte = 11;                // Broche de la LED verte
const int CapteurHumiditeSol = A0;      // Broche du capteur d'humidité du sol
const int PompeEau = 13;                // Broche de la pompe à eau
const int BrocheLDR = A1;               // Broche du capteur de lumière (LDR)

LiquidCrystal lcd(2, 3, 4, 5, 6, 7);    // Initialisation de l'écran LCD

unsigned long debutPompage = 0;         // Heure de début du pompage
unsigned long dureePompage = 0;         // Durée du pompage
bool pompeActive = false;               // État de la pompe
bool arrosageEffectue = false;          // Indicateur pour savoir si l'arrosage a été effectué
bool alerteNiveauEau = false;           // Alerte de niveau d'eau bas
int indexMessageActuel = 0;             // Index du message actuel affiché sur l'écran
unsigned long derniereMAJ = 0;          // Dernière mise à jour des messages

const int BrocheTrig = 8;               // Broche du Trig pour le capteur ultrason
const int BrocheEcho = 9;               // Broche de l'Echo pour le capteur ultrason

String messages[3];                     // Tableau des messages à afficher sur l'écran
long distanceEau; // Variable pour stocker la distance de niveau d'eau

void setup() {
  pinMode(PompeEau, OUTPUT);
  pinMode(LedRouge, OUTPUT);
  pinMode(LedVerte, OUTPUT);
  pinMode(BuzzerPin, OUTPUT);
  
  pinMode(BrocheTrig, OUTPUT);
  pinMode(BrocheEcho, INPUT);
  
  Serial.begin(9600);
  
  lcd.begin(16, 2);
  lcd.clear();
  
  // Affichage du message de bienvenue
  lcd.setCursor(0, 0);
  String messageBienvenue = "Bienvenue!";
  for (int i = 0; i < messageBienvenue.length(); i++) {
    lcd.print(messageBienvenue.charAt(i));
    delay(100);
  }
  delay(1000); // Pause de 1 seconde avant d'afficher les autres messages
  lcd.clear();

  // Affichage des messages principaux
  lcd.setCursor(0, 0);
  String message1 = "Arrosage Auto";
  String message2 = "Des Plantes";
  for (int i = 0; i < message1.length(); i++) {
    lcd.print(message1.charAt(i));
    delay(100);
  }
  lcd.setCursor(0, 1);
  for (int i = 0; i < message2.length(); i++) {
    lcd.print(message2.charAt(i));
    delay(100);
  }
  delay(2500);
  lcd.clear();

  // Initialisation des messages
  messages[0] = "Humidite:      ";
  messages[1] = "Pompe:          ";
  messages[2] = "Niveau d'eau OK";
}

void loop() {
  int valeurHumidite = analogRead(CapteurHumiditeSol);         
  int pourcentageHumidite = map(valeurHumidite, 0, 876, 0, 99);  
  int valeurLDR = analogRead(BrocheLDR);                           
  int niveauLuminosite = map(valeurLDR, 6, 679, 0, 100);          

  // Mise à jour du message d'humidité
  messages[0] = "Humidite: " + String(pourcentageHumidite) + "%  ";

  // Mesure du niveau d'eau
  long duree;
  digitalWrite(BrocheTrig, LOW); 
  delayMicroseconds(2);
  digitalWrite(BrocheTrig, HIGH); 
  delayMicroseconds(10);
  digitalWrite(BrocheTrig, LOW);
  
  duree = pulseIn(BrocheEcho, HIGH); 
  distanceEau = (duree * 0.034) / 2; // Mettre à jour la variable de distance du niveau d'eau
  
  // Affichage de l'humidité, luminosité et distance sur le moniteur série
  Serial.print("Humidite : ");
  Serial.print(pourcentageHumidite);
  Serial.print("% | Luminosite : ");
  Serial.print(niveauLuminosite);
  Serial.print("% | Distance : ");
  Serial.print(distanceEau);
  Serial.println(" cm");

  // Vérification du niveau d'eau
  if (distanceEau < 10) {
    if (!alerteNiveauEau) {
      lcd.setCursor(0, 1);
      lcd.print("Eau faible!     ");
      jouerSon(); 
      alerteNiveauEau = true; 
    }
    messages[2] = "Niveau d'eau LOW ";
  } else {
    alerteNiveauEau = false; 
    messages[2] = "Niveau d'eau OK   ";
  }

  // Mise à jour du message d'état de la pompe
  mettreAJourMessagePompe();

  // Affichage du message actuel en fonction du temps
  if (millis() - derniereMAJ >= 2000) { 
    indexMessageActuel = (indexMessageActuel + 1) % 3; 
    afficherMessage(messages[indexMessageActuel]);
    derniereMAJ = millis();
  }

  // Logique de contrôle de la pompe
  controlerPompe(pourcentageHumidite, niveauLuminosite);
  
  delay(1000);  // Attente
}

void afficherMessage(String message) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(message);
}

void mettreAJourMessagePompe() {
  messages[1] = "Pompe: " + String(pompeActive ? "ON" : "OFF");
}

void controlerPompe(int pourcentageHumidite, int niveauLuminosite) {
  if (distanceEau < 10) {
    pompeActive = false;
    digitalWrite(PompeEau, LOW); 
    digitalWrite(LedVerte, LOW);
    digitalWrite(LedRouge, HIGH); 
    Serial.println("Pompe arrêtée pour cause de niveau d'eau bas.");
    return;
  }
  
  if (!arrosageEffectue && pourcentageHumidite < 50) {
    dureePompage = (niveauLuminosite > 70) ? 20000 : 10000;
    debutPompage = millis();                          
    pompeActive = true;                                
    arrosageEffectue = true; 
    digitalWrite(PompeEau, HIGH);                    
    digitalWrite(LedVerte, HIGH);
    digitalWrite(LedRouge, LOW);
    jouerSon();                                      
    Serial.println("Pompe activée.");
  }

  if (pompeActive && millis() - debutPompage >= dureePompage) {
    digitalWrite(PompeEau, LOW);     
    pompeActive = false;                
    digitalWrite(LedVerte, LOW);
    digitalWrite(LedRouge, HIGH);
    Serial.println("Pompe désactivée.");
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Arrosage termine");
    delay(2000);
  }
}

void jouerSon() {
  tone(BuzzerPin, 87, 100);  
  delay(100);                
  noTone(BuzzerPin);        
}
