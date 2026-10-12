# HP 82240 Scientific Web Terminal — Guida italiana

**v0.1.0 — prototipo non validato sulla stampante.**

## Funzioni
- Editor testo e anteprima carta termica.
- Calcolatrice ALG e RPN, conversioni e grafici/ASCII (motore dimostrativo).
- Ricerca reti Wi-Fi, scelta SSID, password, persistenza credenziali e AP di recupero.
- API HTTP e test IR per HP 82240A/B.

## Hardware
Wemos D1 Mini / ESP8266; D2 (GPIO4) -> 1 kΩ -> base transistor NPN 2N2222 (verificare pinout). 3V3 -> resistenza limitatrice (47 Ω solo valore indicativo, da dimensionare) -> anodo LED IR 940 nm; catodo -> collettore; emettitore -> GND. Stampante originale non modificata.

## Installazione
1. Aprire il progetto in PlatformIO.
2. `pio run -t upload` per il firmware.
3. `pio run -t uploadfs` per i file Web LittleFS.
4. Collegarsi a `HP82240-Setup` e aprire `http://192.168.4.1`.
5. Nella pagina Wi-Fi scegliere *Cerca reti Wi-Fi*, selezionare SSID e inserire password.
6. Dopo il riavvio verificare l'IP DHCP dal monitor seriale a 115200 baud oppure dal router.

## API
| Metodo | Endpoint | Funzione |
| --- | --- | --- |
| GET | `/api/status` | Stato connessione e contatore |
| GET | `/api/wifi/scan` | SSID e RSSI |
| POST | `/api/wifi` | Parametri `ssid`, `password` |
| POST | `/api/print` | Parametro form `plain`, massimo 2048 byte |
| POST | `/api/ir/test` | Test IR |

Esempio: `curl -X POST http://192.168.4.1/api/print --data-urlencode 'plain=HELLO HP 82240'`

## Avvertenze
- Il protocollo Redeye è implementato in forma sperimentale. Oscilloscopio/analizzatore logico richiesti per validare portante, pacchetti e temporizzazione. Nessuna prova di stampa fisica ancora effettuata.
- La stampante non invia ACK; la risposta HTTP non certifica il risultato su carta.
- AP aperto, nessuna autenticazione API, HTTP senza TLS; password Wi-Fi in chiaro su LittleFS. **Usare solo in rete di laboratorio isolata.**
- Nessuna coda persistente; invio sincrono. Caratteri UTF-8 non ASCII sostituiti, grafica bitmap HP non implementata.
- La calcolatrice è dimostrativa e non adatta a calcoli critici.
- La separazione C++ dei moduli è pianificata; attualmente il core ESP è in `src/main.cpp`.

## Roadmap
Validare Redeye su hardware; proteggere AP/API; sostituire timing GPIO software con driver timer affidabile; implementare charset HP, grafica e coda; test matematici e refactor moduli.

## Riferimenti
- [HP 82240B Technical Interfacing Guide](https://literature.hpcalc.org/official/hp82240b-tech.pdf)
- [HP Journal 82240A](https://transnull.com/files/hp48/HP_Journal_HP82240A_1.pdf)
