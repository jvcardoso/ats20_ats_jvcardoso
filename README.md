# ATS_EX Firmware for ATS-20 / ATS-20+ DSP Receiver (`v1.18-r3`)

<p align="center">
  <a href="#english"><strong>🇺🇸 English Documentation</strong></a> &nbsp;|&nbsp;
  <a href="#portugues-br"><strong>🇧🇷 Documentação em Português (PT-BR)</strong></a> &nbsp;|&nbsp;
  <a href="/rus/README.md"><strong>🇷🇺 Руководство на русском языке (Russian)</strong></a>
</p>

<p align="center">
    <img src="img/ats20.png" alt="ATS-20 Receiver running ATS_EX" />
</p>

| Project / Projeto | Details / Detalhes |
| :--- | :--- |
| **Custom Revision / Revisão Customizada** | **`ATS_EX v1.18-r3`** (`JVCARDOSO 2026` — [`jvcardoso/ats20_ats_jvcardoso`](https://github.com/jvcardoso/ats20_ats_jvcardoso)) |
| **Original Author / Autor Original (`v1.18`)** | **[Goshante (`goshante/ats20_ats_ex`)](https://github.com/goshante/ats20_ats_ex)** |
| **DSP Library / Biblioteca Base** | **[PU2CLR SI4735 (`pu2clr/SI4735`)](https://github.com/pu2clr/SI4735)** (inspired by `swling.ru`) |
| **Download `v1.18-r3` (Recommended / Com RDS)** | **[`ATS_EX_v1.18-r3_RDS.hex`](https://github.com/jvcardoso/ats20_ats_jvcardoso/releases/download/v1.18-r3/ATS_EX_v1.18-r3_RDS.hex)** (`30,570` bytes Flash / `1,074` bytes SRAM) |
| **Download `v1.18-r3` (No RDS / Sem RDS)** | **[`ATS_EX_v1.18-r3_NO_RDS.hex`](https://github.com/jvcardoso/ats20_ats_jvcardoso/releases/download/v1.18-r3/ATS_EX_v1.18-r3_NO_RDS.hex)** (`29,232` bytes Flash / `1,053` bytes SRAM) |
| **Download Original `v1.18` (Goshante)** | **[`ATS_EX_v1.18.hex`](https://github.com/goshante/ats20_ats_ex/releases/download/v1.18/ATS_EX_v1.18.hex)** (03.04.2024) |

> **Source hardening note:** the current source tree contains post-r3 fixes and its measured sizes are shown above. The previously published r3 assets must be regenerated from this tree before they match these values; the `v1.18-r3` banner is intentionally unchanged.

---

<a id="english"></a>

# 🇺🇸 English Documentation

## Basic Description
**ATS_EX** is an advanced open-source firmware for **ATS-20 / ATS-20+** receivers running on an **ATmega328P (Arduino Nano or Uno)** microcontroller and the **Si473x (Si4732 / Si4735)** DSP receiver chip.

Originally created by **[Goshante](https://github.com/goshante/ats20_ats_ex)** on top of the **[PU2CLR SI4735](https://github.com/pu2clr/SI4735)** library, this repository (**[`jvcardoso/ats20_ats_jvcardoso`](https://github.com/jvcardoso/ats20_ats_jvcardoso)**) maintains the **`v1.18-r3` (`JVCARDOSO 2026`)** custom revision with SSB tuning/BFO precision fixes, validated EEPROM storage, bounded RDS buffers, lower SRAM consumption, and reproducible CLI builds.

### Boot Splash Screen (`v1.18-r3`)
```text
+--------------------+
|  ATS-20 RECEIVER   |
|  ATS_EX v1.18-r3   |
|   JVCARDOSO 2026   |
|  Custom firmware   |
+--------------------+
```

## What's Improved in Custom Revision `v1.18-r3`
Key technical improvements over upstream `v1.18`:
- **Exact SSB Carrier & BFO Rollover (`±15 kHz`)**: Stores the 1 kHz carrier anchor (`g_currentFrequency`) separately from the fine BFO offset (`g_currentBFO`), preventing cumulative tuning drift when crossing `±15 kHz` boundaries or saving/restoring bands.
- **Cancel Seek by Encoder or Button**: AM/FM station scan (`Sca`) can be aborted immediately either by rotating the tuning encoder or pressing the encoder button.
- **Validated EEPROM State (`v4`/`v5`)**: Validates all stored parameters on boot and automatically resets to safe defaults if out-of-range or incompatible data is detected. Upgrading from upstream `v1.18` or `r1` restores defaults once; `r3` preserves valid `r2`/`r3` settings for the same RDS variant.
- **Bounded RDS Buffers (`USE_RDS=1` / `USE_RDS=0`)**: Safely sanitizes PS/RT/PI fields to printable 16-character ASCII strings without buffer overruns.
- **Nominal CPU Clock**: Removed the runtime CPU clock divider (`CPU` setting) so `millis()`, button debouncing, and EEPROM save timers always run at nominal speed.
- **`PROGMEM` Tables**: Moved step, sub-band, battery and validation-limit tables to Flash (`FlashAccess.h`). The current build uses `1,074` B of SRAM (`974` B free) and `30,570` B of Flash (`150` B free) on the Nano with RDS.

### Additional Hardening in This Source Tree

- **Seek grid independent of manual step**: AM/FM scans start on the documented `10 kHz`/`100 kHz` grid instead of jumping by the selected manual step.
- **Invalid tuning reads are rejected**: A failed or out-of-band frequency read can no longer replace valid state or be written to EEPROM.
- **Factory reset needs a sustained hold**: Releasing the encoder button during the confirmation animation aborts the reset.
- **Menu browsing no longer wears EEPROM**: Navigating or closing an unchanged menu writes nothing.
- **RDS printable filter**: PS/RT/PI bytes outside printable ASCII are replaced by spaces.
- **Display-off polling suspended**: RDS, S-meter and battery updates stop while the OLED is off.
- **Full-screen RDS**: up to 64 RadioText characters in four 16-column lines; PS, 2B and 2A are cycled with the encoder button.
- **Fatal sanitizers and CI**: Host tests halt on UBSan errors and GitHub Actions builds both boards with and without RDS.

## Features
- **Fully reworked minimalistic interface**: Clean, readable layout with a custom 7-segment frequency font (no stretched characters).
- **Fully reworked controls**: Intuitive single-hand operation (see the [User Manual](#user-manual-en) below).
- **Seamless SSB tuning**: BFO is integrated into the main frequency display and regulated directly by the selected tuning step. The carrier rebases every `15 kHz` (`-15 kHz` to `+15 kHz`, covering a continuous `30 kHz` fine-tuning window).
- **Wide frequency coverage**:
  - **LW**: `153` – `520` kHz
  - **MW**: `520` – `1710` kHz
  - **SW**: `1710` – `30000` kHz (continuous tuning across all SW sub-bands, including CB and all HF Ham Radio bands from 160m to 10m)
  - **FM**: `64` – `108` MHz
- **Modulations**: `AM`, `LSB`, `USB`, and `CW` on LW/MW/SW; `WFM` on FM.
- **Full-Screen FM RDS**: `MODE` opens a dedicated 4×16 page showing up to 64 characters of Station Name (8), Station Info/2B (32), or Program Info/2A (64). The encoder button cycles the three fields; `MODE`, tuning, or any adjustment returns to the main screen.
- **AM/FM Station Scan (`Sca`)**: Scans in the last tuning direction (`100 kHz` spacing on FM; `1/5/9/10 kHz` on AM).
- **Rich tuning steps**:
  - **AM**: `1k`, `5k`, `9k`, `10k`, `50k`, `100k`, `1M`
  - **SSB / CW**: `10 Hz`, `25 Hz`, `50 Hz`, `100 Hz`, `500 Hz`, `1k`, `5k`, `9k`, `10k`
  - **FM**: `50k`, `100k`, `1M`
- **Optional S-Meter scale**: Toggleable signal strength bar at the bottom of the screen (long press `STEP`).
- **Comprehensive Settings Menu**: Configure `ATT`/AGC, Soft Mute (`SM`), Automatic Volume Control (`AVC`), SSB AVC (`SVC`), FM De-Emphasis (`DeE`), SSB Sync (`Syn`), Screen Brightness (`Scr`), SW units (`SW`), SSB Soft Mute mode (`SSM`), SSB Cutoff Filter (`COF`), RDS error threshold (`RDS`), permanent BFO calibration (`BFO`), units display (`Uni`), Scan toggle (`Sca`), and CW sideband (`CW`).
- **Quick Mute (`VOL-`)** and **Display On/Off (`AGC`)** buttons.
- **Battery Charge Indicator**: Automatically displayed when a two-resistor (`10 kΩ` + `10 kΩ`) voltage divider is connected to pin `A2`.

---

<a id="how-to-flash-it-on-my-receiver"></a>

## How to Flash It on Your Receiver

> **IMPORTANT TIP BEFORE FLASHING:** Keep the receiver's **physical power switch turned ON** (running on battery) while connected to a **USB 3.0 port** on your computer. The Si4735 DSP and OLED screen draw extra current on reset; having the battery switched ON prevents USB voltage drops that can interrupt flashing around ~70%.

### Step 1: Identify Your Receiver Board & Bootloader
ATS-20 / ATS-20+ receivers come with an **ATmega328P** MCU and a **CH340/CH341 USB-UART** chip (`VID:PID 1a86:7523`), in two hardware revisions:
1. **Arduino Nano revision**: An Arduino Nano module is soldered as a daughterboard onto the main PCB (typically uses **Old Bootloader at `57600` baud**, or **New Optiboot at `115200` baud**).
2. **Arduino Uno / Integrated revision**: The ATmega328P chip is soldered directly onto the main PCB (uses **Optiboot at `115200` baud**).

Note: Both `nano` and `uno` profiles use the exact same `.hex` firmware file (`ATS_EX_v1.18-r3_RDS.hex`). The only difference during upload is the bootloader baud rate (`115200` vs `57600`).

---

### Option A: Flashing on Windows (`AVRDUDESS` GUI)
1. Install the **CH341 USB-UART driver** if Windows does not detect the receiver's COM port automatically.
2. Download and open **[AVRDUDESS](https://blog.zakkemble.net/avrdudess-a-gui-for-avrdude/)**.
3. Turn the receiver **ON** and plug in the Micro-USB / USB-C cable.
4. Under **Presets**, select:
   - **"Arduino Uno (ATmega328P)"** (`115200` baud), OR
   - **"Arduino Nano (ATmega328P)"** / **"Arduino Nano (ATmega328P, Old Bootloader)"** (`57600` baud) if `115200` does not respond.
5. Select your receiver's **COM port**.
6. *(Recommended — Backup current firmware)*: In the **Flash** section, choose a destination filename (e.g. `backup_flash.hex`), select **Read**, and click **Go**.
7. *(Write new firmware)*: In the **Flash** section, browse to **`ATS_EX_v1.18-r3_RDS.hex`**, select **Write**, and click **Go**.
8. If the first attempt stops around ~70%, simply click **Go** a second time immediately — the second pass will complete `100%` and reboot into `ATS_EX v1.18-r3`.

---

### Option B: USB Backup, Flashing & Restore on macOS / Linux (`avrdude` CLI)

#### 1. Detect the USB serial port and test bootloader baud rate
Connect the receiver with its power switch **ON** and list the serial port:

```sh
# macOS:
ls -l /dev/cu.usbserial*
# Linux:
ls -l /dev/ttyUSB*
```

Test communication in read-only mode (`-n`) at `115200` baud first (if it times out, replace `-b 115200` with `-b 57600`):

```sh
AVRDUDE="$HOME/Library/Arduino15/packages/arduino/tools/avrdude/8.0.0-arduino1/bin/avrdude"
AVRDUDE_CONF="$HOME/Library/Arduino15/packages/arduino/tools/avrdude/8.0.0-arduino1/etc/avrdude.conf"
PORT="/dev/cu.usbserial-11440"   # Replace with your port
BAUD="115200"                    # Use 115200 (Optiboot) or 57600 (Old Bootloader)

"$AVRDUDE" -C "$AVRDUDE_CONF" -N -p m328p -c arduino -P "$PORT" -b "$BAUD" -D -n -U "signature:r:-:h"
# Expected output for ATmega328P: 0x1e,0x95,0xf
```

#### 2. Back up the current Flash memory (`flash.hex`)
Read the full 32 KB Flash and 3-byte MCU signature using `-n` (no-write) and `-D` (no auto-erase):

```sh
backup_dir="$HOME/Desktop/ats-backup-$(date +%Y%m%d-%H%M%S)"
mkdir -p "$backup_dir"

"$AVRDUDE" -C "$AVRDUDE_CONF" \
  -N -p m328p -c arduino -P "$PORT" -b "$BAUD" -D -n \
  -U "flash:r:$backup_dir/flash.hex:i" \
  -U "signature:r:$backup_dir/signature.bin:r"

shasum -a 256 "$backup_dir"/*
```

> **Note on EEPROM backups over USB:** In Arduino AVR core 1.8.8, Optiboot ignores the memory-type parameter in STK500v1 page-read requests and returns Flash bytes instead of EEPROM bytes. Therefore, USB backups reliably preserve the program Flash (`flash.hex`), whereas reading raw EEPROM or fuse/lock bits requires an external ISP programmer.

#### 3. Flash and verify `ATS_EX v1.18-r3`
Run the chained command below. If the receiver is running an active firmware with the OLED and Si4735 powered on, the first pass may time out around ~70% right after clearing page 0; the chained second pass (`||`) immediately completes `100%` write and byte-for-byte verification:

```sh
HEX_FILE="build/releases/ATS_EX_v1.18-r3_RDS.hex"

"$AVRDUDE" -C "$AVRDUDE_CONF" \
  -N -p m328p -c arduino -P "$PORT" -b "$BAUD" -D \
  -U "flash:w:$HEX_FILE:i" || \
"$AVRDUDE" -C "$AVRDUDE_CONF" \
  -N -p m328p -c arduino -P "$PORT" -b "$BAUD" -D \
  -U "flash:w:$HEX_FILE:i"
```

Wait for `avrdude` to report `30570 bytes of flash verified`. The receiver will reboot and show the `ATS_EX v1.18-r3` / `JVCARDOSO 2026` splash screen.

#### 4. Restore a previous Flash backup (Rollback)
```sh
"$AVRDUDE" -C "$AVRDUDE_CONF" \
  -N -p m328p -c arduino -P "$PORT" -b "$BAUD" -D \
  -U "flash:w:$backup_dir/flash.hex:i"
```

---

### Option C: Building and Testing from Source
This repository includes a reproducible build script (`scripts/build.py`) and a host regression test suite (`tests/run.py`). Pinned dependencies are defined in `ATS_EX/sketch.yaml` and downloaded automatically by Arduino CLI:

```sh
# Compile Nano and Uno profiles (both RDS=1 and RDS=0):
python3 scripts/build.py

# Compile only the Nano profile with RDS enabled:
python3 scripts/build.py --profile nano --rds 1

# Run host regression tests with AddressSanitizer and UndefinedBehaviorSanitizer:
python3 tests/run.py
```

---

<a id="user-manual-en"></a>

## User Manual

> **EEPROM Reset:** Upgrading from `v1.18` or `r1` resets EEPROM to safe defaults automatically. To manually reset all settings to factory defaults at any time, **press and hold the Encoder Button while turning the receiver ON** and keep holding until the **`EEPROM RESET`** progress bar completes (about 1 second). Releasing the button during the bar cancels the reset.

### Button Functionality
- **`BAND+`**:
  1. **Short press**: Enter band selection mode (rotate encoder to choose, press encoder button or `BAND+` to confirm). In SW mode, jumps across SW sub-bands.
  2. **Long press**: Quickly scroll through bands forward.
  3. **In Settings menu**: Switch between Settings pages.
- **`BAND-`**:
  1. **Short press**: Open / close the **Settings menu** (settings are saved to EEPROM when closing).
  2. **Long press**: Quickly scroll through bands backward.
- **`VOL+`**:
  1. **Short press**: Enter volume adjustment mode.
  2. **Long press**: Quickly increase volume.
- **`VOL-`**:
  1. **Short press**: **Mute / Unmute** audio (`M` is shown in place of volume).
  2. **Long press**: Quickly decrease volume.
- **`STEP`**:
  1. **Short press**: Enter tuning step selection mode.
  2. **Long press**: Toggle the **S-Meter** signal strength bar at the bottom of the screen.
- **`AGC`**:
  1. **Short press**: Turn the OLED **Display ON / OFF** (saves battery and reduces RF noise).
  2. **Long press**: Toggle **SSB Sync (`S`)** mode when in LSB/USB.
- **`BW`**:
  1. **Short press**: Select the audio/IF **Bandwidth filter** for the active modulation.
- **`MODE`**:
  1. **In LW/MW/SW**: Cycle between **`AM`**, **`LSB`**, **`USB`**, and **`CW`** modulations.
  2. **In FM**: Open/close the **full-screen RDS page**.
- **Encoder Rotation**:
  1. **Normal mode**: Tune frequency by the active step.
  2. **Settings / Menus**: Navigate and adjust values.
- **Encoder Button**:
  1. **In AM/FM (when `Sca` is `On`)**: Start **station scan** in the last tuning direction. Rotate the encoder or press the encoder button to **cancel scanning**.
  2. **In SSB/CW (or when `Sca` is `Off`)**: Quick shortcut to adjust the **tuning step**.
  3. **In FM with the RDS page active**: Cycle between **Station Name**, **Station Info**, and **Program Info**; each field fills up to four 16-column lines.
  4. **Held during power-on**: Perform an **EEPROM Reset**; keep holding until the progress bar completes.

### Settings Menu

<p align="center">
    <img src="img/ats20_settings.png" alt="ATS_EX Settings Pages" />
</p>

- **`ATT`**: RF Attenuation (`AUT` for Automatic Gain Control, or manual `1` to `37`).
- **`SM`**: Soft Mute attenuation (`0` to `32`).
- **`AVC`**: Automatic Volume Control maximum gain (`12` to `90`).
- **`SVC`**: Enable/disable Automatic Volume Control in SSB mode (`On` / `Off`).
- **`DeE`**: FM De-Emphasis time constant (`50` µs for Europe/Brazil/most of the world, `75` µs for Americas/South Korea).
- **`Syn`**: Enable/disable SSB Sync mode (`On` / `Off`).
- **`Scr`**: OLED Screen brightness (`5` to `125`).
- **`SW`**: Shortwave AM frequency unit (`KHz` or `MHz`).
- **`SSM`**: SSB Soft Mute trigger mode (`RSS` based on RSSI, or `SNR` based on Signal-to-Noise Ratio).
- **`COF`**: SSB Cutoff Filter (`On` = bandpass filter, `Off` = low-pass filter, `AUT` = automatically enabled when bandwidth is `≤ 2.0 kHz`).
- **`RDS`**: FM RDS error tolerance threshold (`0` = strictest/cleanest text, `3` = most tolerant on weak signals).
- **`BFO`**: Permanent SSB BFO calibration offset (`-60` to `+60`, where each unit is `10 Hz` / `0.01 kHz`, allowing `-0.60 kHz` to `+0.60 kHz` calibration).
- **`Uni`**: Show or hide frequency unit labels (`On` / `Off`).
- **`Sca`**: Enable or disable AM/FM station scan on the Encoder Button (`On` / `Off`).
- **`CW`**: Select base sideband used for CW reception (`LSB` or `USB`).

### Display Elements Description

<p align="center">
    <img src="img/ats20_display.png" alt="ATS_EX Display Elements" />
</p>

1. **Current Modulation**: `AM`, `LSB`, `USB`, `CW` (from `153` to `30000` kHz), or `FM` (`64` to `108` MHz). Shows `S` next to `LSB`/`USB` when Sync is active.
2. **Bandwidth (`BW`)**: Active filter width (or `AUTO` in FM; hidden in `CW`).
3. **Current Frequency**: Main 7-segment readout (includes `.xx` kHz fractional digits in `SSB`/`CW` and `.xx` MHz in `FM`).
4. **Band Name**: Active band indicator (`LW`, `MW`, `SW`, `CB`, `FM`).
5. **Step Value**: Current tuning increment (values without a suffix are in **Hz**, e.g. `10`, `50`, `100`, `500`).
6. **Frequency Units**: `KHz` or `MHz` (automatically hidden when 5-digit SSB frequency + fractional display needs the space).
7. **Volume / Mute**: Audio level (`0`–`63`), or `M` when muted.
8. **Battery Charge Level**: Battery percentage (requires the simple two-resistor voltage divider on pin `A2`; hidden if not installed).

---

## Quick Guide: Listening to Amateur Radio (HF / SSB / CW) on the ATS-20

By international convention, HF voice transmissions use **SSB (Single Sideband)**:
- **Below 10 MHz (`160m`, `80m`, `40m`)** → select **`LSB`** with the **`MODE`** button.
- **Above 10 MHz (`20m`, `17m`, `15m`, `12m`, `10m` and CB `11m`)** → select **`USB`** with the **`MODE`** button.
- **Morse Code (Telegraphy)** → select **`CW`** with the **`MODE`** button (typically in the first `50 kHz` of each band).

| Band | Frequency (kHz) | Voice Modulation | Best Time / Propagation Tip |
| :--- | :--- | :---: | :--- |
| **80 meters** | `3500` – `3800` kHz | **`LSB`** | Night and early morning (regional/national nets) |
| **40 meters** | `7000` – `7300` kHz | **`LSB`** | **Most active band!** Late afternoon, night, and morning (`7050`–`7200` kHz) |
| **20 meters** | `14000` – `14350` kHz | **`USB`** | Daytime and late afternoon (international DX, `14150`–`14300` kHz) |
| **15 meters** | `21000` – `21450` kHz | **`USB`** | Daytime |
| **11 meters (CB)** | `26965` – `27405` kHz | **`AM` / `LSB` / `USB`** | All day (e.g., Channel 19 at `27185` kHz) |
| **10 meters** | `28000` – `29700` kHz | **`USB`** | Daytime during good solar propagation (`28300`–`28600` kHz) |

**Practical tips for clean SSB reception:**
1. **Filter (`BW`)**: In **`LSB`/`USB`**, press **`BW`** and select **`2.2k`** or **`1.8k`** for voice, or **`0.5k` / `1.0k`** for **`CW`**.
2. **Fine tuning (`STEP`)**: In SSB, press the **Encoder Button** (or **`STEP`**) to quickly toggle between **`1k`** (for scanning across the band) and **`100` Hz / `50` Hz / `10` Hz** (to clarify voice pitch).
3. **Antenna**: On `40m` and `80m`, clipping a simple **5 to 10 meter wire** onto the telescopic whip (stretched toward a window or balcony) dramatically improves reception.

---

## How to Add the Battery Charge Level Indicator (Pin `A2` Mod)
Solder two **`10 kΩ`** resistors in series between the **battery positive (`+`)** and **ground (`-`)**, and connect their midpoint to pin **`A2`** on the Arduino/ATmega328P. This applies half the battery voltage to `A2`, and `ATS_EX` will automatically detect and display the battery percentage on screen.

---

<a id="portugues-br"></a>

# 🇧🇷 Documentação em Português (PT-BR)

## Descrição Básica
O **ATS_EX** é um firmware avançado de código aberto para os receptores **ATS-20 / ATS-20+** equipados com microcontrolador **ATmega328P (Arduino Nano ou Uno)** e chip DSP **Si473x (Si4732 / Si4735)**.

Criado originalmente por **[Goshante (`goshante/ats20_ats_ex`)](https://github.com/goshante/ats20_ats_ex)** sobre a biblioteca **[PU2CLR SI4735](https://github.com/pu2clr/SI4735)** (do brasileiro Ricardo Caratti, PU2CLR), este repositório (**[`jvcardoso/ats20_ats_jvcardoso`](https://github.com/jvcardoso/ats20_ats_jvcardoso)**) mantém a revisão customizada **`v1.18-r3` (`JVCARDOSO 2026`)**, trazendo correções de precisão de sintonia SSB/BFO, validação segura de EEPROM, proteção de buffers RDS, redução no consumo de memória RAM e guia completo de atualização via USB.

### Tela de Inicialização (`v1.18-r3`)
```text
+--------------------+
|  ATS-20 RECEIVER   |
|  ATS_EX v1.18-r3   |
|   JVCARDOSO 2026   |
|  Custom firmware   |
+--------------------+
```

## O que há de novo na revisão `v1.18-r3` (`JVCARDOSO 2026`)
Principais melhorias técnicas sobre a versão `v1.18` original:
- **Sintonia SSB e Rollover de BFO exatos (`±15 kHz`)**: A portadora de 1 kHz (`g_currentFrequency`) agora é salva separadamente do deslocamento fino de BFO (`g_currentBFO`), eliminando desvios cumulativos de frequência ao cruzar `±15 kHz` ou ao trocar de banda.
- **Cancelamento imediato de Busca (`Scan`)**: A busca automática de estações em AM/FM pode ser interrompida na hora girando o encoder ou apertando o botão do encoder.
- **EEPROM Validada (`v4`/`v5`)**: Todos os dados lidos da EEPROM são validados na inicialização; caso haja valores corrompidos ou de uma versão antiga incompatível (`v1.18` original ou `r1`), o rádio restaura os padrões de fábrica automaticamente com segurança.
- **Buffers RDS Protegidos (`USE_RDS=1` / `USE_RDS=0`)**: Sanitização completa das mensagens RDS (até 16 caracteres ASCII imprimíveis) sem risco de estouro de memória.
- **Clock Nominal Estável**: Removida a divisão de clock da CPU para garantir que `millis()`, leitura de botões e temporizadores da EEPROM funcionem sempre com tempo exato.
- **Tabelas em `PROGMEM`**: Tabelas de passos, subfaixas, bateria e limites de validação movidas para a memória Flash (`FlashAccess.h`). O build atual usa `1.074` B de SRAM (`974` B livres) e `30.570` B de Flash (`150` B livres) no Nano com RDS.

### Endurecimento adicional nesta árvore de código

- **Grade de busca independente do passo manual**: buscas AM/FM começam na grade documentada de `10 kHz`/`100 kHz`, sem pular pelo passo manual selecionado.
- **Leituras de sintonia inválidas são rejeitadas**: uma leitura falha ou fora da banda não substitui mais estado válido nem é gravada na EEPROM.
- **Reset de fábrica exige pressionar e segurar**: soltar o botão do encoder durante a animação cancela o reset.
- **Navegar no menu não desgasta mais a EEPROM**: navegar ou fechar um menu sem mudanças não grava nada.
- **Filtro RDS imprimível**: bytes PS/RT/PI fora do ASCII imprimível são substituídos por espaço.
- **Polling suspenso com display desligado**: RDS, S-meter e bateria deixam de ser atualizados enquanto o OLED está desligado.
- **RDS em tela cheia**: até 64 caracteres de RadioText em quatro linhas de 16 colunas; PS, 2B e 2A são alternados pelo botão do encoder.
- **Sanitizadores fatais e CI**: os testes host param em erros do UBSan e o GitHub Actions compila as duas placas com e sem RDS.

> **Nota:** esta árvore contém correções posteriores ao asset r3 publicado. Os tamanhos acima correspondem ao build atual; os arquivos r3 publicados precisam ser regravados a partir desta árvore. O banner `v1.18-r3` foi mantido de propósito.

## Principais Funcionalidades
- **Interface totalmente redesenhada**: Visual limpo, sem fontes esticadas, com fonte numérica de 7 segmentos própria.
- **Sintonia SSB contínua e suave**: O BFO faz parte direta da frequência principal exibida na tela. A portadora é reancorada a cada `15 kHz` (`-15 kHz` a `+15 kHz`, cobrindo `30 kHz` contínuos de sintonia fina).
- **Cobertura completa de frequências**:
  - **LW (Ondas Longas)**: `153` a `520` kHz
  - **MW (Ondas Médias / AM)**: `520` a `1710` kHz
  - **SW (Ondas Curtas)**: `1710` a `30000` kHz (sintonia contínua entre todas as sub-bandas, incluindo Faixa do Cidadão/PX 11m e todas as faixas de Radioamador HF de 160m a 10m)
  - **FM**: `64` a `108` MHz
- **Modulações**: `AM`, `LSB`, `USB` e `CW` (telegrafia) em LW/MW/SW; `WFM` em FM.
- **RDS em tela cheia no FM**: `MODE` abre uma página dedicada de 4×16 com até 64 caracteres do Nome da Estação (8), Informação da Estação/2B (32) ou Programação/2A (64). O botão do encoder alterna os três campos; `MODE`, sintonia ou qualquer ajuste volta à tela principal.
- **Busca Automática de Estações (`Scan`)** em AM e FM.
- **Passos de sintonia (`STEP`) variados**:
  - **AM**: `1k`, `5k`, `9k`, `10k`, `50k`, `100k`, `1M`
  - **SSB / CW**: `10 Hz`, `25 Hz`, `50 Hz`, `100 Hz`, `500 Hz`, `1k`, `5k`, `9k`, `10k`
  - **FM**: `50k`, `100k`, `1M`
- **S-Meter integrado**: Barra de intensidade de sinal na base da tela (segure `STEP`).
- **Botão Mute (`VOL-`)** e **Botão para desligar a tela OLED (`AGC`)** (economiza bateria e reduz ruído de RF).
- **Indicador de nível de bateria (`%`)**: Ativado automaticamente ao instalar o divisor de tensão com 2 resistores de `10 kΩ` no pino `A2`.

---

## Como Fazer Backup e Atualizar o Firmware no seu Rádio

> **DICA ESSENCIAL ANTES DE GRAVAR:** Conecte o rádio ao computador com a **chave física traseira ligada (`ON`)** e prefira uma **porta USB 3.0**. Quando o rádio reinicia durante a gravação, o chip Si4735 e a tela OLED consomem mais corrente; manter a bateria ligada evita quedas de tensão na USB que interrompem a gravação perto de ~70%.

### Passo 1: Identifique a placa e o Bootloader do seu ATS-20
Os receptores ATS-20 / ATS-20+ usam o microcontrolador **ATmega328P** com conversor USB-Serial **CH340/CH341** (`VID:PID 1a86:7523`) e existem em duas revisões internas:
1. **Revisão Arduino Nano**: Possui uma plaquinha Arduino Nano soldada sobre a placa principal (pode usar **Old Bootloader em `57600` baud** ou **Optiboot novo em `115200` baud**).
2. **Revisão Arduino Uno (Integrada)**: O chip ATmega328P é soldado diretamente na placa principal do rádio (usa **Optiboot em `115200` baud**).

Observação: O arquivo `.hex` (`ATS_EX_v1.18-r3_RDS.hex`) é **exatamente o mesmo** para ambas as versões. A única diferença na hora de gravar é a velocidade da porta (`115200` ou `57600` baud).

---

### Opção A: Gravando pelo Windows (`AVRDUDESS`)
1. Instale o driver **CH341 USB-UART** caso o Windows não reconheça a porta COM automaticamente.
2. Baixe o arquivo **[`ATS_EX_v1.18-r3_RDS.hex`](https://github.com/jvcardoso/ats20_ats_jvcardoso/releases/latest)** na página de Releases.
3. Baixe e abra o programa **[AVRDUDESS](https://blog.zakkemble.net/avrdudess-a-gui-for-avrdude/)**.
4. Ligue o rádio na chave traseira (**ON**) e conecte o cabo USB ao PC.
5. Em **Presets**, selecione:
   - **"Arduino Uno (ATmega328P)"** (`115200` baud), OU
   - **"Arduino Nano (ATmega328P, Old Bootloader)"** (`57600` baud) caso `115200` não responda.
6. Selecione a **porta COM** do rádio.
7. *(Recomendado — Fazer Backup antes)*: Na seção **Flash**, escolha um caminho para salvar (ex.: `backup_original.hex`), marque a opção **Read** e clique em **Go**.
8. *(Gravar o novo firmware)*: Na seção **Flash**, selecione o arquivo **`ATS_EX_v1.18-r3_RDS.hex`**, marque **Write** e clique em **Go**.
9. Caso a primeira tentativa pare em torno de ~70%, basta clicar em **Go** novamente em seguida — a segunda passada completará `100%` e o rádio reiniciará já com o `ATS_EX v1.18-r3`.

---

### Opção B: Backup, Gravação e Restauração no macOS / Linux (`avrdude`)

#### 1. Descobrir a porta USB e confirmar o baud rate (`115200` vs `57600`)
Com o rádio ligado (**ON**) e conectado via USB, liste a porta serial:

```sh
# No macOS:
ls -l /dev/cu.usbserial*
# No Linux:
ls -l /dev/ttyUSB*
```

Teste a leitura da assinatura do chip em modo somente-leitura (`-n`) usando `115200` baud (se não responder, troque `115200` por `57600`):

```sh
AVRDUDE="$HOME/Library/Arduino15/packages/arduino/tools/avrdude/8.0.0-arduino1/bin/avrdude"
AVRDUDE_CONF="$HOME/Library/Arduino15/packages/arduino/tools/avrdude/8.0.0-arduino1/etc/avrdude.conf"
PORT="/dev/cu.usbserial-11440"   # Substitua pela porta do seu rádio
BAUD="115200"                    # Use 115200 (Optiboot) ou 57600 (Old Bootloader)

"$AVRDUDE" -C "$AVRDUDE_CONF" -N -p m328p -c arduino -P "$PORT" -b "$BAUD" -D -n -U "signature:r:-:h"
# Saída esperada para o ATmega328P: 0x1e,0x95,0xf
```

#### 2. Fazer Backup completo da Flash atual (`flash.hex`)
Salve uma cópia dos 32 KB da Flash atual no seu Desktop antes de atualizar:

```sh
backup_dir="$HOME/Desktop/ats-backup-$(date +%Y%m%d-%H%M%S)"
mkdir -p "$backup_dir"

"$AVRDUDE" -C "$AVRDUDE_CONF" \
  -N -p m328p -c arduino -P "$PORT" -b "$BAUD" -D -n \
  -U "flash:r:$backup_dir/flash.hex:i" \
  -U "signature:r:$backup_dir/signature.bin:r"

shasum -a 256 "$backup_dir"/*
```

> **Nota técnica sobre EEPROM via USB:** No Optiboot (Arduino AVR core 1.8.8), o bootloader ignora o parâmetro de tipo de memória nas leituras STK500v1 e devolve bytes da Flash no lugar da EEPROM. Por isso, o backup via cabo USB preserva fielmente o programa (`flash.hex`), enquanto a leitura física da EEPROM exige gravador ISP externo.

#### 3. Gravar e verificar o firmware `ATS_EX v1.18-r3`
Execute o comando encadeado abaixo. Se o rádio estiver com o display OLED e o Si4735 ativos na primeira tentativa, pode haver um timeout por volta de ~70% logo após limpar a página 0; a segunda chamada encadeada (`||`) conclui `100%` da escrita e verificação byte a byte imediatamente:

```sh
HEX_FILE="build/releases/ATS_EX_v1.18-r3_RDS.hex"

"$AVRDUDE" -C "$AVRDUDE_CONF" \
  -N -p m328p -c arduino -P "$PORT" -b "$BAUD" -D \
  -U "flash:w:$HEX_FILE:i" || \
"$AVRDUDE" -C "$AVRDUDE_CONF" \
  -N -p m328p -c arduino -P "$PORT" -b "$BAUD" -D \
  -U "flash:w:$HEX_FILE:i"
```

Ao terminar, o `avrdude` exibirá `30570 bytes of flash verified` e o rádio reiniciará exibindo a tela `ATS_EX v1.18-r3` / `JVCARDOSO 2026`.

#### 4. Restaurar o backup anterior (Rollback)
Caso queira voltar para o firmware salvo no passo 2:

```sh
"$AVRDUDE" -C "$AVRDUDE_CONF" \
  -N -p m328p -c arduino -P "$PORT" -b "$BAUD" -D \
  -U "flash:w:$backup_dir/flash.hex:i"
```

---

### Opção C: Compilando do Código-Fonte e Rodando os Testes
Este repositório inclui um script de build reproduzível (`scripts/build.py`) e uma suíte de testes de regressão no host (`tests/run.py`). As dependências fixadas em `ATS_EX/sketch.yaml` são baixadas automaticamente pelo Arduino CLI:

```sh
# Compilar os perfis Nano e Uno (com RDS=1 e RDS=0):
python3 scripts/build.py

# Compilar apenas o perfil Nano com RDS ativado:
python3 scripts/build.py --profile nano --rds 1

# Executar os testes de regressão com AddressSanitizer e UndefinedBehaviorSanitizer:
python3 tests/run.py
```

---

## Manual do Usuário (PT-BR)

> **Reset de EEPROM:** Ao atualizar a partir do `v1.18` original, o rádio redefine a EEPROM automaticamente para os padrões seguros. Para fazer um **Reset de Fábrica manual** a qualquer momento, **mantenha o Botão do Encoder pressionado enquanto liga o rádio na chave traseira** e continue segurando até a barra de progresso de **`EEPROM RESET`** terminar (cerca de 1 segundo). Soltar o botão durante a barra cancela o reset.

### Funções dos Botões
- **`BAND+`**:
  1. **Toque curto**: Entra na seleção de bandas (gire o encoder para escolher e confirme no botão do encoder ou em `BAND+`). Em Ondas Curtas (`SW`), avança para a sub-banda seguinte.
  2. **Pressionar e segurar**: Avança rapidamente pelas bandas.
  3. **Dentro do menu Settings**: Alterna entre as páginas de configurações.
- **`BAND-`**:
  1. **Toque curto**: Abre e fecha o **Menu de Configurações** (ao fechar, todas as alterações são salvas na EEPROM).
  2. **Pressionar e segurar**: Retrocede rapidamente pelas bandas.
- **`VOL+`**:
  1. **Toque curto**: Abre o ajuste de volume.
  2. **Pressionar e segurar**: Aumenta o volume rapidamente.
- **`VOL-`**:
  1. **Toque curto**: **Mute / Desmute** imediato (mostra a letra `M` no lugar do volume).
  2. **Pressionar e segurar**: Diminui o volume rapidamente.
- **`STEP`**:
  1. **Toque curto**: Seleciona o passo de sintonia (`Step`).
  2. **Pressionar e segurar**: Liga/desliga a barra de sinal **S-Meter** na parte inferior da tela.
- **`AGC`**:
  1. **Toque curto**: **Liga / Desliga a tela OLED** (excelente para economizar bateria e eliminar ruídos na recepção de sinais fracos).
  2. **Pressionar e segurar**: Liga/desliga o modo **Sync (`S`)** quando estiver em SSB (`LSB`/`USB`).
- **`BW`**:
  1. **Toque curto**: Ajusta a **largura de filtro (`Bandwidth`)** da modulação atual.
- **`MODE`**:
  1. **Em LW/MW/SW**: Alterna entre as modulações **`AM`**, **`LSB`**, **`USB`** e **`CW`**.
  2. **No FM**: Abre/fecha a página **RDS em tela cheia**.
- **Giro do Encoder**:
  1. **Modo normal**: Sintoniza a frequência no passo escolhido.
  2. **Modo de configuração**: Navega entre os itens e altera seus valores.
- **Botão do Encoder**:
  1. **Em AM/FM (com `Sca` = `On`)**: Inicia a **busca automática de estações (`Scan`)** na última direção utilizada. Gire o encoder ou aperte o botão novamente para **parar a busca**.
  2. **Em SSB/CW (ou com `Sca` = `Off`)**: Atalho rápido para trocar o **passo de sintonia (`Step`)**.
  3. **No FM com a página RDS ativa**: Alterna entre os 3 modos de texto RDS (**Nome da Estação**, **Informação da Estação** e **Programação**); cada campo ocupa até quatro linhas de 16 colunas.
  4. **Segurado ao ligar o rádio**: Executa o **Reset da EEPROM**; continue segurando até a barra de progresso terminar.

### Menu de Configurações (`Settings`)

<p align="center">
    <img src="img/ats20_settings.png" alt="Páginas de Configuração do ATS_EX" />
</p>

- **`ATT`**: Atenuação de RF (`AUT` = Controle Automático de Ganho AGC ativo, ou manual de `1` a `37`).
- **`SM`**: Soft Mute (`0` a `32`) — atenua o áudio quando não há sinal.
- **`AVC`**: Controle Automático de Volume (`12` a `90`).
- **`SVC`**: Ativa/desativa o AVC no modo SSB (`On` / `Off`).
- **`DeE`**: De-Emphasis do FM (`50` µs usado no Brasil e Europa; `75` µs usado nos EUA).
- **`Syn`**: Ativa/desativa o modo Sync em SSB (`On` / `Off`).
- **`Scr`**: Brilho da tela OLED (`5` a `125`).
- **`SW`**: Unidade de exibição em Ondas Curtas AM (`KHz` ou `MHz`).
- **`SSM`**: Critério do Soft Mute em SSB (`RSS` baseado em força de sinal RSSI, ou `SNR` baseado na relação sinal/ruído).
- **`COF`**: Filtro de corte de agudos em SSB (`On` = passa-faixa, `Off` = passa-baixa, `AUT` = ativa automaticamente quando `BW ≤ 2.0 kHz`).
- **`RDS`**: Limite de tolerância a erros de RDS no FM (`0` = mais rigoroso/texto limpo; `3` = mais tolerante em sinais fracos).
- **`BFO`**: Calibração permanente de offset do BFO em SSB (`-60` a `+60`, onde cada unidade equivale a `10 Hz` / `0,01 kHz`, permitindo calibrar de `-0,60 kHz` a `+0,60 kHz`).
- **`Uni`**: Exibe ou oculta o sufixo `KHz`/`MHz` na tela (`On` / `Off`).
- **`Sca`**: Ativa ou desativa a busca de estações no botão do encoder em AM/FM (`On` / `Off`).
- **`CW`**: Escolhe a banda lateral base para recepção de telegrafia CW (`LSB` ou `USB`).

### Descrição dos Elementos da Tela

<p align="center">
    <img src="img/ats20_display.png" alt="Elementos da tela do ATS_EX" />
</p>

1. **Modulação atual**: `AM`, `LSB`, `USB`, `CW` (de `153` a `30000` kHz) ou `FM` (`64` a `108` MHz). Exibe a letra `S` ao lado de `LSB`/`USB` quando o modo Sync está ativo.
2. **Largura de Banda (`BW`)**: Filtro ativo no momento (`AUTO` disponível no FM; oculto em `CW`).
3. **Frequência Atual**: Leitura principal em 7 segmentos (inclui casas decimais `.xx` kHz em `SSB`/`CW` e `.xx` MHz no `FM`).
4. **Nome da Banda**: Indica a faixa atual (`LW`, `MW`, `SW`, `CB` ou `FM`).
5. **Passo de Sintonia (`Step`)**: Incremento atual do encoder (quando aparece sem sufixo `k`/`M`, o valor está em **Hz**, ex.: `10`, `50`, `100`, `500`).
6. **Unidade de Frequência**: `KHz` ou `MHz`.
7. **Volume / Mute**: Nível de áudio (`0` a `63`) ou `M` quando silenciado.
8. **Nível de Bateria**: Porcentagem da bateria (requer o mod de 2 resistores de `10 kΩ` no pino `A2`; fica oculto se não estiver instalado).

---

## Guia Rápido: Como Ouvir Radioamador (HF / SSB / CW) no ATS-20

Por convenção internacional, transmissões de voz em HF utilizam **SSB (Banda Lateral Única)**:
- **Abaixo de 10 MHz (`160m`, `80m`, `40m`)** → selecione **`LSB`** no botão **`MODE`**.
- **Acima de 10 MHz (`20m`, `17m`, `15m`, `12m`, `10m` e PX `11m`)** → selecione **`USB`** no botão **`MODE`**.
- **Código Morse (Telegrafia)** → selecione **`CW`** no botão **`MODE`** (geralmente nos primeiros `50 kHz` de cada faixa).

| Faixa | Frequência (kHz) | Modulação de Voz | Dica de Horário / Propagação |
| :--- | :--- | :---: | :--- |
| **80 metros** | `3500` – `3800` kHz | **`LSB`** | Noite e madrugada (conversas regionais/nacionais) |
| **40 metros** | `7000` – `7300` kHz | **`LSB`** | **A mais ativa no Brasil!** Fim de tarde, noite e manhã (`7050`–`7150` kHz) |
| **20 metros** | `14000` – `14350` kHz | **`USB`** | Dia e fim de tarde (DX internacional, `14150`–`14300` kHz) |
| **15 metros** | `21000` – `21450` kHz | **`USB`** | Período diurno |
| **11 metros (PX/CB)** | `26965` – `27405` kHz | **`AM` / `LSB` / `USB`** | Dia todo (ex.: Canal 19 em `27185` kHz) |
| **10 metros** | `28000` – `29700` kHz | **`USB`** | Dia com boa abertura solar (`28300`–`28600` kHz) |

**Dicas práticas para recepção SSB limpa:**
1. **Filtro (`BW`)**: Em **`LSB`/`USB`**, aperte **`BW`** e escolha **`2.2k`** ou **`1.8k`** para voz, ou **`0.5k` / `1.0k`** para **`CW`**.
2. **Sintonia fina (`STEP`)**: Em SSB, aperte o **botão do encoder** (ou **`STEP`**) para alternar rapidamente entre **`1k`** (para procurar estações) e **`100` Hz / `50` Hz / `10` Hz** (para deixar a voz natural, sem timbre fino ou grave demais).
3. **Antena**: Em `40m` e `80m`, prender um fio simples de **5 a 10 metros** na ponta da antena telescópica (esticado até uma janela ou varanda) melhora drasticamente a recepção.

---

## Como Fazer o Indicador de Bateria Funcionar (Mod no Pino `A2`)
Você precisará de um ferro de solda e **dois resistores de `10 kΩ`**. Solde um resistor no polo negativo (GND) e o outro no polo positivo (`+`) da bateria de lítio. Solde a junção central entre os dois resistores diretamente no pino **`A2`** do Arduino/ATmega328P. Dessa forma, metade da tensão da bateria será aplicada ao pino `A2`, e o firmware **ATS_EX** passará a exibir automaticamente a porcentagem de carga no canto superior direito da tela.
