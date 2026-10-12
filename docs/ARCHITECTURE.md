# Architecture and protocol limitations

```text
Browser UI (ALG / RPN / text / conversions / graph)
          |
      HTTP POST /api/print
          |
ESP8266 WebServer + LittleFS
          |
ASCII conversion (UTF-8 limited)
          |
Experimental Redeye packet encoder
          |
GPIO4 -> NPN transistor -> 940 nm IR LED
          |
      HP 82240A/B
```

Provisioning: open recovery AP -> Wi-Fi scan -> SSID/password storage on LittleFS -> restart -> STA connection. Persistent AP is available after setup.

## Code layout
- `src/main.cpp`: Wi-Fi, HTTP, IR and state (not yet split into separate C++ modules).
- `data/index.html`: standalone browser front end, ALG/RPN, text preview and utilities.
- `platformio.ini`: Wemos D1 Mini and LittleFS.

## Signal validation
Nominal 32.768kHz IR carrier, nominal 427µs half-bit, seven-cycle burst modeled using GPIO calls and microsecond delays. Interrupts and GPIO overhead affect real timings. Verify waveform and HP parity/check bits against the manufacturer's documentation before concluding protocol interoperability. No real-printer test completed.

## Operational and security limits
One-way IR, no printer receipt/ACK. Synchronous HTTP print request; no persistent job queue. Unauthenticated HTTP API, open AP and plaintext Wi-Fi secret storage. Do not expose to the Internet. Future versions should use protected setup AP, API authorization and tested timed IR transmission.

## Roadmap
1. Bench test waveforms and printer output.
2. Improve security and isolate provisioning.
3. Extract modules, add queue, tests and diagnostics.
4. Exact HP text encoding and graphics capabilities.
