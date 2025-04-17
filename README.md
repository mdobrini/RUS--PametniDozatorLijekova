# Pametni dozator lijekova

![GitHub](https://img.shields.io/github/license/mdobrini/RUS--PametniDozatorLjekova?style=flat-square)
![Arduino](https://img.shields.io/badge/Arduino-UNO-blue?style=flat-square&logo=arduino)

> Automatski sustav za precizno doziranje lijekova temeljen na Arduino platformi

## Dokumentacija

- [📚 API dokumentacija (Doxygen)](https://mdobrini.github.io/RUS--PametniDozatorLijekova/)
- [📖 GitHub Wiki](https://github.com/mdobrini/RUS--PametniDozatorLjekova/wiki)
- [🖥️ Wokwi simulacija](https://wokwi.com/projects/428213172579332097)

## Shema spajanja

### Pin konfiguracija

| Komponenta    | Arduino Pin                                   | Opis                                        |
| ------------- | --------------------------------------------- | ------------------------------------------- |
| LCD I2C       | SDA (A4), SCL (A5)                            | 16x2 LCD zaslon preko I2C sučelja           |
| RTC DS1307    | SDA (A4), SCL (A5)                            | Modul za precizno praćenje vremena          |
| Keypad        | 3, 4, 5, 7 (redovi) + A0, A1, A2, A3 (stupci) | 4x4 matrična tipkovnica                     |
| Gornji servo  | 9                                             | Kontrolira ulaz lijeka u područje čekanja   |
| Donji servo   | 10                                            | Kontrolira izlaz lijeka iz područja čekanja |
| Zujalica      | 11                                            | Zvučna signalizacija doziranja              |
| LED indikator | 12                                            | Vizualna signalizacija buđenja              |
| Tipka buđenja | 2 (INT0)                                      | Gumb za ručno buđenje iz sleep moda         |

## Prikaz uređaja

![image](https://github.com/user-attachments/assets/68a581ea-cd3e-4c64-9ddd-8ea13e9f110c)


## O projektu

Pametni dozator lijekova je uređaj koji automatizira proces izdavanja lijekova prema programiranom rasporedu. Dizajniran je prvenstveno za starije osobe i pacijente s kroničnim bolestima koji trebaju redovito uzimati lijekove.

### Ključne značajke

- Podržava do 5 različitih vremena doziranja
- Precizno praćenje vremena preko RTC modula
- Energetski učinkovit rad s periodima spavanja
- Jednostavno korisničko sučelje s LCD zaslonom i tipkovnicom
- Trajna pohrana postavki u EEPROM
- Zvučna i vizualna signalizacija
- Mehanizam za sigurno doziranje lijekova pomoću dva servo motora

## Brze upute

1. Pritisnite tipku **A** za postavljanje rasporeda doziranja
2. Pritisnite tipku **B** za pregled svih postavljenih vremena
3. Pritisnite tipku **C** za testiranje mehanizma doziranja
4. Pritisnite tipku **D** za povratak na prikaz vremena

---

Razvio: [Mislav Dobrinić](https://github.com/mdobrini1), [Matej Ledinski](https://github.com)
