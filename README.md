# Razvoj ugradbenih sustava

_Članovi tima: Mislav Dobrinić, Matej Ledinski_

## Pametni dozator lijekova - Automatski sustav za izdavanje lijekova u određeno vrijeme

Ime projekta "Pametni dozator lijekova" naglašava glavnu namjenu projekta - pouzdano i automatsko izdavanje lijekova u zadano vrijeme. Ovaj projekt ima za cilj pomoći osobama koje moraju redovito uzimati lijekove, posebno starijim osobama ili osobama s kroničnim bolestima, osiguravajući da dobiju pravu dozu u pravo vrijeme.

Pokretanje projekta preko Wokwi simulatora: [Link na Wokwi projekt](https://wokwi.com/projects/428213172579332097)

[Doxygen dokumentacija](https://mdobrini.github.io/RUS--PametniDozatorLjekova/)

## Opis projekta

Ovaj projekt je rezultat timskog rada u sklopu projektnog zadatka kolegija Razvoj ugradbenih sustava na Tehničkom veleučilištu u Zagrebu.

Cilj projekta je izraditi pouzdani elektronički sustav koji će pomoći u pravilnom i redovitom uzimanju lijekova. Motivacija za ovaj projekt proizlazi iz sve veće potrebe za pomoćnim tehnologijama u zdravstvenoj skrbi, posebno za starije osobe koje često zaboravljaju uzeti svoje lijekove u određeno vrijeme. Sustav koristi Arduino platformu s različitim komponentama za praćenje vremena, izdavanje lijekova i interakciju s korisnikom, sve s ciljem povećanja sigurnosti i poboljšanja kvalitete života korisnika.

# Funkcionalni zahtjevi - Pametni dozator lijekova

## Shema povezivanja komponenti

### Tablica povezivanja

| Arduino Pin | Komponenta         | Priključak komponente | Opis                                           |
| ----------- | ------------------ | --------------------- | ---------------------------------------------- |
| A4          | LCD 16x2 (I2C)     | SDA                   | I2C podatkovni signal                          |
| A5          | LCD 16x2 (I2C)     | SCL                   | I2C clock signal                               |
| A4          | RTC modul DS1307   | SDA                   | I2C podatkovni signal (dijeljen s LCD)         |
| A5          | RTC modul DS1307   | SCL                   | I2C clock signal (dijeljen s LCD)              |
| 9           | Gornji servo motor | Signal                | PWM signal za kontrolu gornjeg serva           |
| 10          | Donji servo motor  | Signal                | PWM signal za kontrolu donjeg serva            |
| 11          | Buzzer             | Signal                | PWM signal za zvučne signale                   |
| 12          | Indikator LED      | Anoda                 | Indikacija buđenja iz sleep moda               |
| 2           | Tipka za buđenje   | Switch                | Buđenje uređaja iz sleep moda (spojen na INT0) |
| 5           | 4x4 tipkovnica     | Red 1                 | Prvi red tipkovnice                            |
| 4           | 4x4 tipkovnica     | Red 2                 | Drugi red tipkovnice                           |
| 3           | 4x4 tipkovnica     | Red 3                 | Treći red tipkovnice                           |
| 7           | 4x4 tipkovnica     | Red 4                 | Četvrti red tipkovnice                         |
| A0          | 4x4 tipkovnica     | Stupac 1              | Prvi stupac tipkovnice                         |
| A1          | 4x4 tipkovnica     | Stupac 2              | Drugi stupac tipkovnice                        |
| A2          | 4x4 tipkovnica     | Stupac 3              | Treći stupac tipkovnice                        |
| A3          | 4x4 tipkovnica     | Stupac 4              | Četvrti stupac tipkovnice                      |
| 5V          | Sve komponente     | VCC                   | Napajanje za sve komponente                    |
| GND         | Sve komponente     | GND                   | Zajedničko uzemljenje                          |

## Opća funkcionalnost

| Zahtjev | Opis                                                                         | Prioritet |
| ------- | ---------------------------------------------------------------------------- | --------- |
| F-1.1   | Uređaj mora imati dva servo motora za kontrolu ispuštanja spremnika lijekova | Visok     |
| F-1.2   | Uređaj mora prikazivati trenutno vrijeme na LCD zaslonu                      | Visok     |
| F-1.3   | Uređaj mora imati tipkovnicu (keypad) za unos podataka                       | Visok     |
| F-1.4   | Uređaj mora imati režim uštede energije (sleep mode)                         | Srednji   |
| F-1.5   | Uređaj mora čuvati postavke nakon gubitka napajanja                          | Visok     |
| F-1.6   | Uređaj mora podržavati do 5 različitih vremena za lijekove                   | Visok     |
| F-1.7   | Uređaj mora imati zvučni signal (buzzer) za obavijesti                       | Srednji   |
| F-1.8   | Uređaj mora imati namjensku tipku za buđenje iz režima uštede energije       | Visok     |
| F-1.9   | Uređaj mora imati LED indikator koji treperi prilikom buđenja                | Srednji   |

## Funkcionalnosti korisničkog sučelja

| Zahtjev | Opis                                                             | Prioritet |
| ------- | ---------------------------------------------------------------- | --------- |
| F-2.1   | Korisnik može postaviti vrijeme za svaki lijek pomoću tipkovnice | Visok     |
| F-2.2   | Korisnik može pregledati sva postavljena vremena za lijekove     | Srednji   |
| F-2.3   | Korisnik može ručno testirati dispenser mehanizam                | Nizak     |
| F-2.4   | Uređaj prikazuje trenutno vrijeme i status na glavnom zaslonu    | Visok     |
| F-2.5   | Uređaj prikazuje poruku kad je vrijeme za izdavanje lijeka       | Visok     |
| F-2.6   | Korisnik može deaktivirati određeni raspored lijeka              | Srednji   |
| F-2.7   | Korisnik može probuditi uređaj iz sleep moda pritiskom na tipku  | Visok     |

## Funkcionalnosti mehanizma za izdavanje lijekova

| Zahtjev | Opis                                                                       | Prioritet |
| ------- | -------------------------------------------------------------------------- | --------- |
| F-3.1   | Gornji servo motor otvara se da propusti spremnik s lijekom                | Visok     |
| F-3.2   | Donji servo motor otvara se da ispusti spremnik s lijekom                  | Visok     |
| F-3.3   | Mehanizam osigurava da samo jedan spremnik lijeka bude ispušten            | Visok     |
| F-3.4   | Uređaj prikazuje status procesa ispuštanja lijeka na zaslonu               | Srednji   |
| F-3.5   | Proces ispuštanja lijekova mora biti sekvencijalan i pouzdano kontroliran  | Visok     |
| F-3.6   | Uređaj mora emitirati zvučni signal nakon uspješnog izdavanja lijeka       | Srednji   |
| F-3.7   | Uređaj se mora automatski probuditi iz sleep moda kada je vrijeme za lijek | Visok     |

## Funkcionalnosti praćenja vremena

| Zahtjev | Opis                                                                | Prioritet |
| ------- | ------------------------------------------------------------------- | --------- |
| F-4.1   | Uređaj koristi RTC DS1307 modul za točno praćenje vremena           | Visok     |
| F-4.2   | Uređaj odbrojava svaki dan nove cikluse za lijekove                 | Visok     |
| F-4.3   | Uređaj resetira status izdanih lijekova u ponoć                     | Visok     |
| F-4.4   | Uređaj provjerava raspored svakih nekoliko sekundi                  | Visok     |
| F-4.5   | Uređaj periodički provjerava vrijeme čak i u režimu uštede energije | Visok     |

## Tehnički zahtjevi

| Zahtjev | Opis                                                                            | Prioritet |
| ------- | ------------------------------------------------------------------------------- | --------- |
| F-6.1   | Uređaj koristi Arduino Uno platformu                                            | Visok     |
| F-6.2   | LCD zaslon povezan je putem I2C sučelja                                         | Visok     |
| F-6.3   | Tipkovnica je 4x4 matrica                                                       | Visok     |
| F-6.4   | Servo motori montiraju se vertikalno jedan iznad drugog                         | Visok     |
| F-6.5   | Uređaj koristi EEPROM za pohranu rasporeda lijekova                             | Visok     |
| F-6.6   | Uređaj koristi Watchdog timer za periodičko buđenje iz sleep moda               | Srednji   |
| F-6.7   | RTC DS1307 modul s baterijom osigurava točno vrijeme i nakon nestanka napajanja | Visok     |
| F-6.8   | Buzzer emitira zvučne signale frekvencije oko 2kHz za jasno čujne obavijesti    | Srednji   |
| F-6.9   | Tipka za buđenje povezana je na INT0 (pin 2) za efikasno buđenje iz sleep moda  | Visok     |

## Posebne značajke

| Zahtjev | Opis                                                               | Prioritet |
| ------- | ------------------------------------------------------------------ | --------- |
| F-7.1   | Korisničko sučelje nudi jasne upute za podešavanje i korištenje    | Srednji   |
| F-7.2   | Struktura koda mora biti dobro dokumentirana s Doxygen komentarima | Srednji   |
| F-7.3   | Kod mora biti modularan i održiv                                   | Srednji   |
| F-7.4   | Uređaj mora biti siguran za upotrebu                               | Visok     |
| F-7.5   | Zvučni signali moraju jasno ukazivati na izdavanje lijeka          | Srednji   |

# Nefunkcionalni zahtjevi

Nefunkcionalni zahtjevi i zahtjevi domene primjene dopunjuju funkcionalne zahtjeve. Oni opisuju kako se sustav treba ponašati i koja ograničenja treba poštivati (performanse, korisničko iskustvo, pouzdanost, standardi kvalitete, sigurnost...).

## Opći nefunkcionalni zahtjevi

| Zahtjev | Opis                                                                                | Prioritet |
| ------- | ----------------------------------------------------------------------------------- | --------- |
| NF-1.1  | Sustav mora raditi neprekidno bez grešaka najmanje 1000 sati                        | Visok     |
| NF-1.2  | Vrijeme pokretanja uređaja ne smije biti duže od 2 sekunde                          | Srednji   |
| NF-1.3  | Uređaj mora biti kompatibilan s industrijskim standardima (npr. I2C, UART)          | Visok     |
| NF-1.4  | Uređaj mora biti energetski učinkovit, s potrošnjom ispod 100 mW u stanju mirovanja | Visok     |
| NF-1.5  | Uređaj mora raditi u temperaturnom rasponu od -10°C do +60°C                        | Srednji   |
| NF-1.6  | Softver mora biti dokumentiran i testiran prema definiranim testnim scenarijima     | Visok     |
| NF-1.7  | Korisnički interfejs mora biti intuitivan i lako razumljiv                          | Srednji   |
| NF-1.8  | Vrijeme odziva na pritisak tipke mora biti kraće od 200 ms                          | Srednji   |
| NF-1.9  | Uređaj mora biti otporan na kratkotrajne prekide napajanja                          | Visok     |
| NF-1.10 | Uređaj mora biti siguran za upotrebu od strane starijih osoba                       | Visok     |
| NF-1.11 | Zvučni signal mora biti dovoljno glasan da se čuje u prostoriji                     | Srednji   |

## Zahtjevi za održavanje

| ID zahtjeva | Opis                                                                   | Prioritet |
| ----------- | ---------------------------------------------------------------------- | --------- |
| NF-3.1      | Sustav treba biti oblikovan tako da omogućuje jednostavno održavanje   | Visok     |
| NF-3.2      | Sustav treba omogućiti jednostavno punjenje spremnika s lijekovima     | Visok     |
| NF-3.3      | Servo motori moraju biti lako dostupni za zamjenu ili popravak         | Srednji   |
| NF-3.4      | Sustav treba imati dovoljnu dokumentaciju                              | Visok     |
| NF-3.5      | Kod sustava treba biti dobro dokumentiran Doxygen komentarima          | Visok     |
| NF-3.6      | Sustav treba sadržavati dijagnostičke funkcije za provjeru ispravnosti | Srednji   |
| NF-3.7      | Sustav treba imati mogućnost vraćanja na tvornički zadane postavke     | Srednji   |

## Zahtjevi pouzdanosti i sigurnosti

| ID zahtjeva | Opis                                                               | Prioritet |
| ----------- | ------------------------------------------------------------------ | --------- |
| NF-4.1      | Sustav mora osigurati da se lijekovi izdaju samo u zadano vrijeme  | Visok     |
| NF-4.2      | Sustav mora biti pouzdan i ne smije propustiti izdavanje lijeka    | Visok     |
| NF-4.3      | Sustav treba imati mehanizam samotestiranja pri pokretanju         | Srednji   |
| NF-4.4      | Mehanički dijelovi moraju izdržati najmanje 10.000 ciklusa rada    | Visok     |
| NF-4.5      | Spremnici lijekova ne smiju se zaglaviti u mehanizmu za izdavanje  | Visok     |
| NF-4.6      | U slučaju nestanka struje, RTC DS1307 mora održavati točno vrijeme | Visok     |

## Zahtjevi korisničkog iskustva

| ID zahtjeva | Opis                                                                          | Prioritet |
| ----------- | ----------------------------------------------------------------------------- | --------- |
| NF-5.1      | Sustav treba biti jednostavan za uporabu bez potrebe za tehničkim predznanjem | Visok     |
| NF-5.2      | Proces postavljanja vremena za lijekove ne smije imati više od 5 koraka       | Srednji   |
| NF-5.3      | Sustav mora pružati jasne povratne informacije o svim postupcima              | Visok     |
| NF-5.4      | Zvučni i vizualni signali moraju biti prilagođeni starijim osobama            | Srednji   |
| NF-5.5      | Svi natpisi na zaslonu moraju biti čitljivi s udaljenosti od 1 metra          | Srednji   |
| NF-5.6      | Zvučni signal mora biti jasno raspoznatljiv kao podsjetnik za uzimanje lijeka | Visok     |
