#include <LiquidCrystal.h>
#include <math.h>

#define MOTOR_BUTONU_PIN 22
#define KEMER_BUTONU_PIN 24
#define KAPI_ANAHTARI_PIN 26
#define SICAKLIK_SENSORU_PIN A0
#define ISIK_SENSORU_PIN A1
#define YAKIT_SENSORU_PIN A2

#define LCD_RS_PIN 12
#define LCD_E_PIN 11
#define LCD_D4_PIN 5
#define LCD_D5_PIN 4
#define LCD_D6_PIN 3
#define LCD_D7_PIN 2

#define KEMER_LED_PIN 30
#define FAR_LED_PIN 32
#define YAKIT_LED_PIN 34
#define KAPI_LED_R_PIN 36
#define KAPI_LED_G_PIN 38
#define KAPI_LED_B_PIN 40
#define BUZZER_PIN 42
#define MOTOR_PIN 44
#define FAN_PIN 46

LiquidCrystal lcd(LCD_RS_PIN, LCD_E_PIN, LCD_D4_PIN, LCD_D5_PIN, LCD_D6_PIN, LCD_D7_PIN);

const float SICAKLIK_ACMA_ESIK = 25.0;
const float SICAKLIK_KAPAMA_ESIK = 24.0;
const int ISIK_KARANLIK_ESIK = 250;
const int ISIK_AYDINLIK_ESIK = 300;
const int YAKIT_AZ_ESIK = 10;
const int YAKIT_KRITIK_ESIK = 5;
const long YAKIT_LED_YANIP_SONME_HIZI = 400;
const long DEBOUNCE_SURESI = 50;
const long MESAJ_GOSTERIM_SURESI_DEFAULT = 2000;
const long LCD_GUNCELLEME_ARALIGI = 1000;

bool motorCalisiyor = false;
bool kemerTakili = false;
bool kapiKapali = true;
bool farlarAcik = false;
bool fanAcik = false;
bool motorButonunaBasildiGecici = false;

enum YakitDurumu { YAKIT_NORMAL, YAKIT_AZ, YAKIT_KRITIK, YAKIT_BITTI };
YakitDurumu mevcutYakitDurumu = YAKIT_NORMAL;
int anlikYakitYuzdesi = 100;

String lcdSatir1 = "", lcdSatir2 = "";
unsigned long sonLcdGuncellemeZamani = 0;
unsigned long mesajBaslangicZamani = 0;
bool mesajGosteriliyor = false;
unsigned long mesajSuresi = MESAJ_GOSTERIM_SURESI_DEFAULT;

unsigned long suankiZaman = 0;
unsigned long motorButonSonOkumaZamani = 0;
int motorButonSonDurum = HIGH;
int motorButonHamSonDurum = HIGH;
unsigned long yakitLedSonYanipSonmeZamani = 0;
bool yakitLedDurumu = LOW;

float anlikSicaklik = 20.0;

void GirdileriOku(bool ilkOkuma = false);
void DurumlariGuncelle();
void CiktilariYonet();
void LcdYonetimi();
float sicaklikOkuRaw();
int yakitYuzdeOku();
void RGB_RenkAyarlari(byte red, byte green, byte blue);
void LcdGuncelle(String s1, String s2, bool zorla = false);
void LcdMesajGoster(String s1, String s2, unsigned long sure);

void setup() {
    Serial.begin(9600);
    lcd.begin(16, 2);
    lcd.print("Sistem Basliyor");
    delay(1000);

    pinMode(MOTOR_BUTONU_PIN, INPUT_PULLUP);
    pinMode(KEMER_BUTONU_PIN, INPUT_PULLUP);
    pinMode(KAPI_ANAHTARI_PIN, INPUT_PULLUP);
    pinMode(SICAKLIK_SENSORU_PIN, INPUT);
    pinMode(ISIK_SENSORU_PIN, INPUT);
    pinMode(YAKIT_SENSORU_PIN, INPUT);
    pinMode(KEMER_LED_PIN, OUTPUT);
    pinMode(FAR_LED_PIN, OUTPUT);
    pinMode(YAKIT_LED_PIN, OUTPUT);
    pinMode(KAPI_LED_R_PIN, OUTPUT);
    pinMode(KAPI_LED_G_PIN, OUTPUT);
    pinMode(KAPI_LED_B_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(MOTOR_PIN, OUTPUT);
    pinMode(FAN_PIN, OUTPUT);

    digitalWrite(KEMER_LED_PIN, LOW);
    digitalWrite(FAR_LED_PIN, LOW);
    digitalWrite(YAKIT_LED_PIN, LOW);
    RGB_RenkAyarlari(0, 0, 0);
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(MOTOR_PIN, LOW);
    digitalWrite(FAN_PIN, LOW);

    GirdileriOku(true);
    anlikSicaklik = sicaklikOkuRaw();

    if (anlikYakitYuzdesi <= 0) mevcutYakitDurumu = YAKIT_BITTI;
    else if (anlikYakitYuzdesi <= YAKIT_KRITIK_ESIK) mevcutYakitDurumu = YAKIT_KRITIK;
    else if (anlikYakitYuzdesi <= YAKIT_AZ_ESIK) mevcutYakitDurumu = YAKIT_AZ;
    else mevcutYakitDurumu = YAKIT_NORMAL;

    lcd.clear();
    lcd.print("Sistem Hazir");
    delay(1500);
    lcd.clear();
    sonLcdGuncellemeZamani = 0;
}

void loop() {
    suankiZaman = millis();
    GirdileriOku();
    DurumlariGuncelle();
    CiktilariYonet();
    LcdYonetimi();
    motorButonunaBasildiGecici = false;
}

void GirdileriOku(bool ilkOkuma) {
    int motorButonOkunan = digitalRead(MOTOR_BUTONU_PIN);

    if (motorButonOkunan != motorButonHamSonDurum) {
        motorButonSonOkumaZamani = suankiZaman;
    }

    if ((suankiZaman - motorButonSonOkumaZamani) > DEBOUNCE_SURESI) {
        if (motorButonOkunan != motorButonSonDurum) {
            motorButonSonDurum = motorButonOkunan;
            if (motorButonSonDurum == LOW) {
                motorButonunaBasildiGecici = true;
            }
        }
    }
    motorButonHamSonDurum = motorButonOkunan;

    kemerTakili = (digitalRead(KEMER_BUTONU_PIN) == LOW);
    kapiKapali = (digitalRead(KAPI_ANAHTARI_PIN) == HIGH);
    anlikYakitYuzdesi = yakitYuzdeOku();
    anlikSicaklik = sicaklikOkuRaw();
}

void DurumlariGuncelle() {
    if (anlikYakitYuzdesi <= 0) {
        mevcutYakitDurumu = YAKIT_BITTI;
        if (motorCalisiyor) {
            motorCalisiyor = false;
            fanAcik = false;
            LcdMesajGoster("Yakit Bitti!", "Motor Durdu", MESAJ_GOSTERIM_SURESI_DEFAULT * 2);
        }
    } else if (anlikYakitYuzdesi <= YAKIT_KRITIK_ESIK) {
        mevcutYakitDurumu = YAKIT_KRITIK;
    } else if (anlikYakitYuzdesi <= YAKIT_AZ_ESIK) {
        mevcutYakitDurumu = YAKIT_AZ;
    } else {
        mevcutYakitDurumu = YAKIT_NORMAL;
    }

    if (!kapiKapali && motorCalisiyor) {
        motorCalisiyor = false;
        fanAcik = false;
        LcdMesajGoster("Kapi Acildi!", "Motor Durduruldu!", MESAJ_GOSTERIM_SURESI_DEFAULT * 2);
    }

    if (motorButonunaBasildiGecici) {
        if (motorCalisiyor) {
            motorCalisiyor = false;
            fanAcik = false;
            LcdMesajGoster("Motor Durduruldu", "(Butonla)", MESAJ_GOSTERIM_SURESI_DEFAULT);
        } else {
            if (mevcutYakitDurumu == YAKIT_BITTI) {
                LcdMesajGoster("Yakit Yok!", "Motor Calismaz", MESAJ_GOSTERIM_SURESI_DEFAULT);
            } else if (!kapiKapali) {
                LcdMesajGoster("Kapi Acik!", "Motor Calismaz", MESAJ_GOSTERIM_SURESI_DEFAULT);
            } else {
                motorCalisiyor = true;
                LcdMesajGoster("Motor Calistirildi", "(Butonla)", MESAJ_GOSTERIM_SURESI_DEFAULT);
            }
        }
    }

    if (motorCalisiyor) {
        if (!isnan(anlikSicaklik)) {
            bool fanDurumuEski = fanAcik;
            if (anlikSicaklik >= SICAKLIK_ACMA_ESIK && !fanAcik) {
                fanAcik = true;
            } else if (anlikSicaklik < SICAKLIK_KAPAMA_ESIK && fanAcik) {
                fanAcik = false;
            }
            if (fanAcik != fanDurumuEski) {
                if (fanAcik) {
                    LcdMesajGoster("Klima Acildi", "Sogutuyor...", MESAJ_GOSTERIM_SURESI_DEFAULT);
                } else {
                    LcdMesajGoster("Klima Kapandi", "", MESAJ_GOSTERIM_SURESI_DEFAULT);
                }
            }
        } else {
            if (fanAcik) {
                fanAcik = false;
                LcdMesajGoster("Sicaklik Hatasi!", "Klima Kapatildi", MESAJ_GOSTERIM_SURESI_DEFAULT);
            }
        }
    } else {
        if (fanAcik) {
            fanAcik = false;
            LcdMesajGoster("Klima Kapandi", "(Motor Durdu)", MESAJ_GOSTERIM_SURESI_DEFAULT);
        }
    }

    int isikSeviye = analogRead(ISIK_SENSORU_PIN);
    bool farDurumuEski = farlarAcik;
    if (isikSeviye <= ISIK_KARANLIK_ESIK && !farlarAcik) {
        farlarAcik = true;
    } else if (isikSeviye > ISIK_AYDINLIK_ESIK && farlarAcik) {
        farlarAcik = false;
    }
    if (farlarAcik != farDurumuEski) {
        LcdMesajGoster(farlarAcik ? "Farlar Acildi" : "Farlar Kapandi", "", MESAJ_GOSTERIM_SURESI_DEFAULT);
    }
}

void CiktilariYonet() {
    digitalWrite(MOTOR_PIN, motorCalisiyor ? HIGH : LOW);
    digitalWrite(FAN_PIN, fanAcik ? HIGH : LOW);
    digitalWrite(KEMER_LED_PIN, !kemerTakili ? HIGH : LOW);
    digitalWrite(FAR_LED_PIN, farlarAcik ? HIGH : LOW);

    if (mevcutYakitDurumu == YAKIT_KRITIK) {
        if (suankiZaman - yakitLedSonYanipSonmeZamani >= YAKIT_LED_YANIP_SONME_HIZI) {
            yakitLedSonYanipSonmeZamani = suankiZaman;
            yakitLedDurumu = !yakitLedDurumu;
            digitalWrite(YAKIT_LED_PIN, yakitLedDurumu);
        }
    } else if (mevcutYakitDurumu == YAKIT_AZ) {
        digitalWrite(YAKIT_LED_PIN, HIGH);
        yakitLedDurumu = HIGH;
    } else {
        digitalWrite(YAKIT_LED_PIN, LOW);
        yakitLedDurumu = LOW;
    }

    if (!kapiKapali) {
        RGB_RenkAyarlari(255, 0, 255);
    } else {
        RGB_RenkAyarlari(0, 0, 0);
    }

   bool buzzerCalmali = (motorCalisiyor && !kemerTakili);
    digitalWrite(BUZZER_PIN, buzzerCalmali ? HIGH : LOW);
}

void LcdYonetimi() {
    if (mesajGosteriliyor) {
        if (suankiZaman - mesajBaslangicZamani >= mesajSuresi) {
            mesajGosteriliyor = false;
            sonLcdGuncellemeZamani = 0;
            LcdGuncelle("", "", true);
        } else {
            return;
        }
    }

    String s1 = "", s2 = "";
    bool oncelikliUyariVar = false;

    if (mevcutYakitDurumu == YAKIT_BITTI) {
        s1 = "!! YAKIT YOK !!";
        s2 = "Motor Calismaz!";
        oncelikliUyariVar = true;
    } else if (!kapiKapali) {
        s1 = "!! KAPI ACIK !!";
        s2 = motorCalisiyor ? "Motor Durdu!" : "Motor Calismaz!";
        oncelikliUyariVar = true;
    } else if (!kemerTakili && motorCalisiyor) {
        s1 = "* Emniyet Kemeri";
        s2 = "* Takili Degil!";
        oncelikliUyariVar = true;
    } else if (mevcutYakitDurumu == YAKIT_KRITIK) {
        s1 = "! Kritik Yakit !";
        s2 = "%" + String(anlikYakitYuzdesi) + " Hizli Dolum";
        oncelikliUyariVar = true;
    } else if (mevcutYakitDurumu == YAKIT_AZ) {
        s1 = "Uyari: Yakit Az";
        s2 = "%" + String(anlikYakitYuzdesi) + " Dolum Gerekli";
        oncelikliUyariVar = true;
    }

    if (oncelikliUyariVar) {
        LcdGuncelle(s1, s2);
    } else if (suankiZaman - sonLcdGuncellemeZamani >= LCD_GUNCELLEME_ARALIGI) {
        s1 = "Y:" + String(anlikYakitYuzdesi) + "%";
        if (isnan(anlikSicaklik)) {
            s1 += " S:HATA";
        } else {
            s1 += " S:" + String(anlikSicaklik, 1) + (char)223 + "C";
        }
        s2 = "M:" + String(motorCalisiyor ? "ACIK" : "KAPALI") + " F:" + String(farlarAcik ? "A" : "K") + " K:" + String(fanAcik ? "A" : "K");
        LcdGuncelle(s1, s2);
        sonLcdGuncellemeZamani = suankiZaman;
    }
}

void LcdGuncelle(String s1, String s2, bool zorla) {
    while (s1.length() < 16) s1 += " ";
    s1 = s1.substring(0, 16);
    while (s2.length() < 16) s2 += " ";
    s2 = s2.substring(0, 16);

    if (zorla || s1 != lcdSatir1 || s2 != lcdSatir2) {
        lcd.setCursor(0, 0);
        lcd.print(s1);
        lcd.setCursor(0, 1);
        lcd.print(s2);
        lcdSatir1 = s1;
        lcdSatir2 = s2;
    }
}

void LcdMesajGoster(String s1, String s2, unsigned long sure) {
    LcdGuncelle(s1, s2, true);
    mesajBaslangicZamani = suankiZaman;
    mesajSuresi = sure;
    mesajGosteriliyor = true;
}

float sicaklikOkuRaw() {
    int sensorDeger = analogRead(SICAKLIK_SENSORU_PIN);
    float gerilim = sensorDeger * (5.0 / 1023.0);
    float sicaklikC = gerilim * 100.0;

    if (isnan(sicaklikC) || sicaklikC < -20.0 || sicaklikC > 150.0) {
        return NAN;
    }
    return sicaklikC;
}

int yakitYuzdeOku() {
    int potDeger = analogRead(YAKIT_SENSORU_PIN);
    int yuzde = map(potDeger, 0, 1023, 0, 100);
    return constrain(yuzde, 0, 100);
}

void RGB_RenkAyarlari(byte red, byte green, byte blue) {
    analogWrite(KAPI_LED_R_PIN, red);
    analogWrite(KAPI_LED_G_PIN, green);
    analogWrite(KAPI_LED_B_PIN, blue);
}