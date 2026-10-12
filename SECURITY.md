# Security policy

## Supported versions
Only the latest development version receives security fixes. This is experimental firmware and **not production hardened**.

## Reporting vulnerabilities
Please **do not open public issues** containing credentials, exploits or undisclosed vulnerabilities. Use GitHub's **Report a vulnerability** private advisory feature if enabled. Otherwise contact the repository owner privately through their GitHub profile and request a secure channel. Do not post secrets in screenshots or logs.

## Operational warnings
- Do not expose the ESP8266 HTTP server to the public Internet or port-forward it.
- The HTTP API uses an API key, but **HTTP is unencrypted**. On an untrusted LAN, keys and Wi-Fi passwords can be intercepted. Isolate the device on a trusted VLAN or use an authenticated TLS reverse proxy.
- The API key is printed on the serial console. Protect serial access and logs.
- Wi-Fi credentials and the API key are stored on LittleFS without encryption; physical access can expose them.
- The setup AP password is printed on the serial console; change the device's credentials if compromised.
- Printing is experimental and unidirectional, so HTTP acceptance does not prove a physical print.
- Review upstream dependencies and firmware changes before flashing.

## Repository maintenance
Enable GitHub secret scanning, push protection, Dependabot alerts and branch protection/rulesets in Settings. Repository file changes alone **cannot enforce** those account-level settings.
