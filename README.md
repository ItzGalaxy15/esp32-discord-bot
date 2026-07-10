# ESP32 Discord Bot

Een ESP32 Discord bot gebaseerd op [esp-discord](https://github.com/abobija/esp-discord) en ESP-IDF. De bot verbindt via WiFi met Discord en echo't berichten terug in een serverkanaal.

## Wat doet de bot?

Dit is een **echo bot** (gebaseerd op het officiële esp-discord echo voorbeeld):

1. Verbindt met je WiFi netwerk
2. Logt in bij Discord via je bot token
3. Luistert naar berichten in kanalen waar de bot toegang toe heeft
4. Antwoordt met: `Hey <gebruikersnaam> you wrote `<jouw bericht>``

Berichten van andere bots worden genegeerd.

## Vereisten

- ESP32 development board (getest met ELEGOO ESP32 DevKit, CP2102, USB-C)
- ESP-IDF v6.0.2 (via Espressif Installation Manager)
- Cursor of VS Code met de ESP-IDF extensie
- WiFi netwerk (2.4 GHz)
- Discord bot token ([Discord Developer Portal](https://discord.com/developers/applications))

## Discord bot instellen

### 1. Applicatie en bot aanmaken

1. Ga naar [Discord Developer Portal](https://discord.com/developers/applications)
2. Maak een nieuwe Application aan
3. Ga naar **Bot** en klik **Reset Token** (of kopieer het token)
4. Zet deze **Privileged Gateway Intents** aan:
   - **Message Content Intent** (verplicht)
   - **Server Members Intent** (optioneel)

### 2. Bot uitnodigen naar je server

1. Ga naar **OAuth2** > **URL Generator**
2. Scopes: `bot`
3. Bot Permissions: minimaal **Send Messages** en **Read Message History**
4. Open de gegenereerde URL en voeg de bot toe aan je server

## Project configureren

### 1. Environment variabelen

Kopieer het voorbeeldbestand en vul je gegevens in:

```powershell
copy .env.example .env
```

Vul `.env` in:

```env
DISCORD_TOKEN=jouw_discord_bot_token
WIFI_SSID=jouw_wifi_naam
WIFI_PASSWORD=jouw_wifi_wachtwoord
```

### 2. Secrets synchroniseren

Dit script schrijft je token naar `main/secrets.h` en WiFi gegevens naar `sdkconfig`:

```powershell
python scripts/sync_env.py
```

Voer dit uit na elke wijziging in `.env`.

### 3. TLS certificaten (eenmalig)

De esp-discord library heeft Discord TLS certificaten nodig. Genereer ze eenmalig:

```powershell
.\scripts\generate_discord_certs.ps1
```

## Bouwen en flashen

### Via Cursor (aanbevolen)

1. Open dit project in Cursor
2. `Ctrl+Shift+P` > **ESP-IDF: Build your Project**
3. Sluit alle andere terminals/monitors die COM6 gebruiken
4. `Ctrl+Shift+P` > **ESP-IDF: Build, Flash and Monitor**

Of apart flashen en monitoren:

- **ESP-IDF: Flash (UART) Your Project**
- **ESP-IDF: Monitor your Device**

### Via ESP-IDF terminal

Open **ESP-IDF: Open ESP-IDF Terminal**, dan:

```powershell
python scripts/sync_env.py
idf.py -p COM6 flash monitor
```

Pas `COM6` aan als je board op een andere poort staat. Controleer via **ESP-IDF: Select Port to Use**.

## De bot testen

### 1. Serial monitor controleren

Na opstarten zou je dit moeten zien:

```
I (xxxx) wifi: connected
I (xxxx) discord_bot: Bot JouwBotNaam#1234 connected
```

Als je `Bot logged out` of authenticatiefouten ziet, controleer je token en intents.

### 2. Bericht sturen in Discord

1. Ga naar een tekstkanaal op je server waar de bot in zit
2. Typ bijvoorbeeld: `hallo`
3. De bot antwoordt met: `Hey jouw_naam you wrote `hallo``

### 3. Serial output bij een bericht

```
I (xxxx) discord_bot: New message (dm=false, autor=...
I (xxxx) discord_bot: Echo message successfully sent
```

## Veelvoorkomende problemen

| Probleem | Oplossing |
|----------|-----------|
| `command not found` (ESP-IDF) | Herlaad Cursor, wacht tot extensie activeert |
| `Could not open COM6, port is busy` | Sluit andere monitors/terminals, trek USB kort los |
| `idf.py` niet herkend in cmd | Gebruik **ESP-IDF: Open ESP-IDF Terminal** |
| Bot verbindt niet met Discord | Token opnieuw genereren, intents controleren |
| WiFi verbindt niet | Controleer `.env` en run `python scripts/sync_env.py` opnieuw |
| Geen berichten zichtbaar | Message Content Intent aan, bot moet in het kanaal staan |

## Projectstructuur

```
esp32-discord-bot/
├── main/
│   └── discord_bot.c      # Bot logica
├── components/
│   └── esp-discord/       # Discord library (lokaal, IDF 6 patch)
├── scripts/
│   ├── sync_env.py        # .env naar build bestanden
│   └── generate_discord_certs.ps1
├── .env                   # Secrets (niet in git)
├── sdkconfig.defaults     # Project defaults
└── docs/
    └── INTERNAL.md        # Interne code documentatie
```

## Documentatie

- [esp-discord library](https://github.com/abobija/esp-discord)
- [ESP-IDF documentatie](https://docs.espressif.com/projects/esp-idf/)
- Interne architectuur: zie [docs/INTERNAL.md](docs/INTERNAL.md)
