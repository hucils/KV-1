# Tancul KV-1

## Ce construiești

Un tanc KV-1 printat 3D, la scara 1:10, pe care îl conduci de pe telefon. Merge pe șenile, turela se învârte, tunul urcă și coboară. Când apeși **FOC**, țeava sare înapoi și la gura ei se aprinde un LED. **Nu iese nimic din țeavă și nu trebuie să pui nimic în ea.**

Modelul e făcut de **Salin10**. Îl găsești pe [Printables](https://www.printables.com/model/219806-tank-kv-1-rc-model-110), [Thingiverse](https://www.thingiverse.com/thing:5069466) și [Cults3D](https://cults3d.com/en/3d-model/game/heavy-tank-kv-1-rc-model-1-10).

## Citește asta întâi

Acest README a fost scris **fără acces la pagina modelului și la ghidul lui de asamblare**: site-urile au refuzat cererile automate. Nu au fost găsite nici videoclipuri cu construcția acestui model, deci nu a fost văzut niciunul.

| Sigur, din descrierea modelului | De verificat de tine în ghid |
|---|---|
| scara 1:10, versiunea din iunie 1941 | toți pașii de asamblare |
| tracțiune cu motor, turelă rotativă, tun care urcă și coboară | ce motoare și ce reductor se potrivesc |
| 8 șuruburi M4x50 și 2 șuruburi M4x85 (cele M4x85 trec prin piesa 35) | restul de șuruburi și rulmenți |
| conține kit pentru KV-1E și turelă de KV-2 | cum se fixează țeava și dacă poate culisa |

**Ghidul din arhiva modelului are ultimul cuvânt.** Când spune altceva decât acest README despre piese printate, șuruburi sau ordinea asamblării, ascultă de ghid. Electronica și programul de aici sunt separate de model. Le-am ales noi, nu Salin10.

## Ce ai nevoie

### Modelul

1. Fă-ți cont pe Printables și descarcă toate fișierele modelului, inclusiv ghidul de asamblare. Printre fișiere ar trebui să găsești `MODEL.rar` (modelul, cu ajustările din 13.11.2021) și `UP01.rar` (piesele modificate).
2. Citește tot ghidul **înainte** să printezi ceva. Fă lista ta de piese, șuruburi și motoare.
3. Citește și notele autorului despre modificări. Sunt variante pentru piesele 19+20, 20+21, 23 și 24, scutul tunului (T47) a fost refăcut, iar la fulii s-a schimbat diametrul exterior. Printează ultimele versiuni.

### Electronica

- placă **ESP32** (ESP32 Dev Module) și un cablu USB de date
- două motoare cu reductor, câte unul pe fiecare șenilă. Motoarele galbene TT de la mașinuțe nu sunt potrivite: tancul iese de vreo 67 cm lungime și 33 cm lățime (calcul făcut din dimensiunile tancului real). Folosește ce recomandă ghidul
- un modul **L298N**. Scoate jumperii de pe ENA și ENB. El duce cel mult 2A pe canal și pierde vreo 2V din tensiunea bateriei. Măsoară curentul motoarelor tale cu șenila blocată un moment. Dacă trece mult de 2A, ia un driver mai puternic, cu aceleași trei intrări (PWM, IN1, IN2) și care acceptă semnale de 3,3V
- un servo cu **rotație continuă** (360°, de tip FS90R) pentru turelă
- două servo-uri obișnuite, de tip MG90S, cu pinioane de metal: unul pentru ridicarea tunului, altul pentru recul
- un LED galben sau alb și un rezistor de 220Ω
- o baterie potrivită cu tensiunea motoarelor. Pentru motoare de 6V merge un LiPo 2S de 7,4V
- **un modul de 5V (step-down / buck) de cel puțin 3A**, pentru placă și cele trei servo-uri. Reglează-l și măsoară cu un multimetru că scoate 5V **înainte** să legi ceva la el
- un condensator de vreo 1000µF între 5V și GND, lângă servo-uri. Atenție la polaritate: dunga de pe el merge la GND
- un întrerupător și o siguranță de vreo 5A pe plusul bateriei

**Dacă ghidul folosește un motor obișnuit la turelă, nu un servo:** programul de acum nu știe să-l comande. Ori găsești un loc pentru servo-ul de 360°, ori programul trebuie schimbat.

Bateria LiPo se încarcă doar cu încărcător de LiPo, cu cineva de față. Dacă s-a umflat sau s-a lovit, nu o mai folosi.

## Cum printezi

Setările de aici sunt un punct de pornire general, nu ale autorului. Dacă ghidul dă altele, folosește-le pe ale lui.

- duză de 0,4 mm, strat de 0,2 mm, 3 sau 4 pereți, umplere 20-30%
- PLA merge pentru carcasă. Piesele care duc forță, cum sunt roțile dințate și suporturile de motor, sunt mai rezistente din PETG
- verifică dacă piesele cele mai mari încap pe patul imprimantei tale
- printează o singură șenilă și un suport de motor, apoi potrivește-le. Mai bine afli acum că trebuie reglată toleranța decât după ce ai printat tot tancul

## Cum legi firele

Masa bateriei, a modulului L298N, a modulului de 5V, a plăcii și a servo-urilor trebuie să fie **legată împreună**.

| L298N | ESP32 |
|---|---|
| ENA | 25 |
| IN1 | 26 |
| IN2 | 27 |
| ENB | 18 |
| IN3 | 19 |
| IN4 | 21 |
| GND | GND |

- motorul din stânga: OUT1 și OUT2
- motorul din dreapta: OUT3 și OUT4

| Piesă | Semnal la ESP32 |
|---|---|
| servo turelă | 22 |
| servo tun | 23 |
| servo recul | 13 |
| LED la gura țevii | 4, prin rezistorul de 220Ω. Celălalt picior la GND |

Toate cele trei servo-uri iau plusul (V+) de la **ieșirea modulului de 5V, nu de la placă**. Placa ia 5V de la același modul, pe pinul VIN. Când placa e în USB, scoate firul de la VIN.

Turela nu are contacte rotative, deci firele din ea se răsucesc. **Nu o învârti mai mult de o tură în același sens.** Programul nu are cum să te oprească. Lasă firele destul de lungi și trece-le prin centrul turelei.

## Cum pui programul pe placă

Fă asta întâi pe masă, cu placa legată doar prin USB. Motoarele pot aștepta.

1. Instalează Arduino IDE, de pe [arduino.cc/en/software](https://www.arduino.cc/en/software).
2. **Tools > Board > Boards Manager**. Caută `esp32`, cel de la Espressif, și apasă Install. Prima dată durează.
3. **Tools > Manage Libraries**. Caută `WebSockets`, cel de la Markus Sattler, și apasă Install.
4. **File > Open** și deschide `KV1_Tank/KV1_Tank.ino`.
5. Sus în fișier e linia `// #define WOKWI_SIM`. Las-o așa, cu `//` în față. Linia aia e doar pentru simulator.
6. Tot în fișier găsești `WIFI_PASSWORD`. **Pune parola ta**, de cel puțin 8 caractere. Cine are parola poate conduce tancul.
7. Leagă placa la calculator cu un cablu USB de date. Un cablu care doar încarcă telefonul nu trimite programul.
8. **Tools > Board > esp32 > ESP32 Dev Module**.
9. **Tools > Port**. Alege portul apărut când ai băgat cablul.
10. Apasă săgeata de Upload. Dacă se blochează pe „Connecting”, ține apăsat BOOT, apasă o dată EN sau RST, dă drumul la BOOT și încarcă din nou.
11. Deschide Serial Monitor, lupa din dreapta sus, cu viteza **115200**. Trebuie să vezi:

    `Waiting for a phone...`

## Cum legi telefonul

1. Pornește tancul.
2. Pe telefon, la Wi-Fi, alege rețeaua **KV-1-Tank** și scrie parola ta.
3. Dacă telefonul spune că rețeaua nu are internet, alege să rămâi conectat.
4. Deschide browserul și scrie exact **`http://192.168.4.1`**
5. Întoarce telefonul pe orizontală. Când pe ecran scrie **Conectat**, poți conduce.

Nu instalezi nicio aplicație și nu-ți trebuie internet. Merge și pe iPhone, și pe Android. Un singur telefon conduce odată.

## Comenzile

| Pe ecran | Ce face |
|---|---|
| stick-ul din stânga, sus și jos | înainte și înapoi |
| stick-ul al doilea, stânga și dreapta | viraj. Tancul se poate învârti pe loc |
| butonul din mijloc | comută între încet și repede. Pornește pe încet |
| discul din dreapta, stânga și dreapta | învârte turela |
| discul din dreapta, sus și jos | ridică și coboară tunul |
| **FOC** | trage. Butonul stă gri vreo două secunde, până se „încarcă” tunul |

Dacă ridici degetul, stick-ul revine la mijloc. Tunul rămâne unde l-ai lăsat. Dacă telefonul iese din rază, se blochează, primește un apel sau ieși din pagină, șenilele și turela se opresc singure în cel mult o jumătate de secundă.

## Cum faci reculul

Nu avem confirmat că modelul are o țeavă care culisează, așa că mecanismul de recul poate fi nevoie să-l faci tu.

1. Uită-te în ghid cum se fixează țeava. Ea trebuie să poată aluneca vreo 1 cm înainte și înapoi, ușor și fără joc lateral.
2. Dacă e fixă, fă-i o bucșă printată prin care să culiseze.
3. Pune servo-ul de recul în turelă și leagă brațul lui de țeavă cu o sârmă rigidă.
4. Trece două fire subțiri prin țeavă până la LED, cu destulă rezervă ca să nu se întindă când tunul urcă.

Modelul are licența **CC BY-NC-ND**. Îl poți printa și modifica pentru tine, dar nu îl poți vinde și **nu poți publica** piese modificate. De aceea fișierele modelului nu sunt în acest repo.

## Prima pornire

Fă pașii în ordine. Un servo legat greșit poate rupe piese printate.

1. **Servo-urile fără brațe.** Pornește programul și apasă FOC, mișcă discul. Verifică să se miște toate și că placa nu repornește.
2. **Montează servo-urile cu programul pornit**, ca să fie deja la mijloc. Abia apoi prinde brațele și legăturile.
3. **Turela.** Dacă se învârte singură cu degetul ridicat, schimbă `TURRET_STOP_US` puțin câte puțin, până stă pe loc. Dacă merge invers, pune `TURRET_REVERSED` pe `true`.
4. **Tunul.** Limitele din program sunt mici la început: `GUN_LOW_US = 1400` și `GUN_HIGH_US = 1650`. Mărește-le cu câte 25 și oprește-te înainte ca tunul să atingă ceva. Dacă merge invers, pune `GUN_REVERSED` pe `true`.
5. **Reculul.** La fiecare FOC, țeava sare repede înapoi și revine încet. Cât de mult se trage reglezi din `RECOIL_BACK_US`. Valoarea 1000 înseamnă vreo 45° la servo, adică aproape 1 cm cu un braț de 12 mm. **Înainte să legi sârma, pune 1300**, apoi crește treptat spre 1000, ca să nu rupi ceva dacă țeava se blochează. Dacă trage în sensul greșit, folosește 1700 și urcă spre 2000. Oprește-te când țeava ajunge la capăt fără ca servo-ul să bâzâie.
6. **Șenilele, cu tancul pe un suport**, să nu atingă masa. Stick-ul din stânga în sus trebuie să le învârtă pe amândouă înainte. Dacă una merge invers, pune `true` la `LEFT_REVERSED` sau `RIGHT_REVERSED`.
7. **Pe podea, pe viteză mică.** Valorile din program sunt doar un punct de pornire. Le potrivești după ce vezi cum merge tancul tău.

## Dacă ceva nu merge

- **Pagina nu se deschide.** Oprește datele mobile pe telefon și încearcă din nou. Verifică să fie `http://` în față și să fie telefonul pe KV-1-Tank.
- **Rețeaua KV-1-Tank nu apare.** Uită-te în Serial Monitor. Dacă scrie `Could not start Wi-Fi`, parola are sub 8 caractere.
- **Placa repornește când apeși FOC sau când pornesc motoarele.** Bateria scade prea tare sau modulul de 5V e prea slab. Încarcă bateria, verifică firele groase și condensatorul.
- **Tancul merge înainte, dar nu virează pe loc.** Șenilele cer multă forță ca să alunece. Crește `MIN_PWM` și `SLOW_LIMIT`, sau încearcă pe o podea netedă. Dacă tot nu merge, motoarele sunt prea slabe.
- **Motoarele doar bâzâie la viteză mică.** Crește `MIN_PWM`.
- **O șenilă sare de pe roți.** Verifică întinderea ei și roțile dințate, după ghid.
- **L298N se încinge tare.** Dacă nu poți ține degetul pe el, motoarele cer prea mult. Nu le ține blocate, lasă-l să se răcească și gândește-te la un driver mai puternic.
- **Un servo bâzâie și se încălzește când stă.** Împinge în ceva. Micșorează limita lui din program.
- **Țeava nu revine la loc.** Se freacă în bucșă. Șlefuiește piesa și verifică să nu se îndoaie sârma.

## Simulatorul

Poți încerca programul fără nicio piesă, în Wokwi. Trei joystick-uri, două butoane și un comutator țin locul telefonului. LED-urile arată motoarele și focul, iar cele trei servo-uri arată turela, tunul și reculul.

1. În VS Code instalează extensia **Wokwi**.
2. Scoate `//` din fața liniei `#define WOKWI_SIM`.
3. În Arduino IDE: **Sketch > Export Compiled Binary**.
4. Deschide `KV1_Tank/diagram.json` în VS Code și pornește simularea. Mesajul `Waiting for a phone...` e normal. Comutatorul ține locul telefonului: la stânga, cum pornește, telefonul e conectat și trebuie să apară `Phone connected`. Dacă îl muți la dreapta, e ca și cum telefonul ar pierde legătura. Abia apoi răspund joystick-urile și butoanele.
5. **Pune `//` la loc înainte să încarci pe tanc.** Cu linia activă, programul nu pornește Wi-Fi-ul.

Fișierul `kv1.test.yaml` verifică singur șenilele, turela, limitele tunului și focul. Se rulează cu [Wokwi CLI](https://docs.wokwi.com/wokwi-ci/getting-started), care cere un token de la Wokwi:

`wokwi-cli . --scenario kv1.test.yaml --timeout 30000`
