# HP 82240 Scientific Web Terminal — English

**v0.1.0 — experimental prototype; not hardware validated.**

## Features
Free-text editor and thermal paper preview; ALG and RPN scientific calculator demonstration, unit conversions and ASCII graphs; Wi-Fi SSID discovery, selection and saved credentials; recovery access point; HTTP print and IR test endpoints.

## Wiring
Wemos D1 Mini GPIO4/D2 -> 1 kΩ -> NPN 2N2222 base (check exact pinout). 3.3V -> correctly sized resistor (47 Ω is only a starting example) -> 940nm IR LED anode; cathode -> NPN collector; emitter -> ground. HP printer remains unmodified.

## Flash and provision
1. Open project in PlatformIO.
2. `pio run -t upload`
3. `pio run -t uploadfs`
4. Connect to open AP `HP82240-Setup`, browse to `http://192.168.4.1`.
5. Choose Wi-Fi scan, select SSID, enter password; after restart find assigned IP in serial output (115200) or router DHCP table.

## REST API
- `GET /api/status`: current connection and TX counter
- `GET /api/wifi/scan`: discovered SSIDs and RSSI
- `POST /api/wifi`: form `ssid`, `password`
- `POST /api/print`: form `plain`, up to 2048 bytes
- `POST /api/ir/test`: fixed test phrase

`curl -X POST http://192.168.4.1/api/print --data-urlencode 'plain=HELLO HP 82240'`

## Important limitations
The software IR encoder uses approximate timing, is unverified on HP hardware and must be measured with a logic analyzer. Printing is unidirectional: HTTP success only confirms an attempted transmission. No API authentication/TLS; open recovery AP; plaintext credentials in LittleFS. **Laboratory/trusted isolated networks only.** ASCII subset only; bitmap and exact HP character mappings pending. Calculator is not high-precision validated. C++ modules remain in a single translation unit in this prototype.

## References
- [HP 82240B Technical Interfacing Guide](https://literature.hpcalc.org/official/hp82240b-tech.pdf)
