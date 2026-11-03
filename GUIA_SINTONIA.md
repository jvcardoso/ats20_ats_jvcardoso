# Guia Completo de Sintonia e Testes do ATS-20 (`ATS_EX v1.18-r3`)
**Localização de referência:** São José do Rio Preto – SP, Brasil

Este guia mostra **como parametrizar cada função do rádio** (`BAND`, `MODE`, `BW`, `STEP` e `Settings`) para explorar todas as faixas que o **ATS-20** é capaz de receber, com exemplos reais que você pode sintonizar a partir de **São José do Rio Preto – SP**.

---

## 1. Tabela Resumo: Como Parametrizar o Rádio para Cada Faixa

| O que você quer ouvir | Banda (`BAND+`) | Modulação (`MODE`) | Filtro (`BW`) | Passo (`STEP`) | Configuração ideal no Menu (`BAND-`) |
| :--- | :---: | :---: | :---: | :---: | :--- |
| **1. FM Local + Texto RDS** | `FM` | `FM` (depois `MODE` p/ RDS) | `AUTO` ou `110k` | `100k` | `DeE = 50`, `RDS = 1` ou `2`, `Sca = On` |
| **2. Rádios AM Regionais (`MW`)** | `MW` | `AM` | `3.0k` ou `4.0k` | `10k` | `ATT = AUT`, `SM = 8`, `Sca = On` |
| **3. Ondas Curtas Brasil (`SW`)** | `SW` | `AM` | `3.0k` ou `4.0k` | `5k` | `ATT = AUT`, `SM = 8`, `AVC = 48` |
| **4. Ondas Curtas Internacionais (China/Europa/EUA)** | `SW` | `AM` | `2.5k` ou `3.0k` | `5k` | `ATT = AUT` (ou `1`–`5`), `SM = 4` |
| **5. Radioamador Voz (`40m` / `80m`)** | `SW` | **`LSB`** | `2.2k` ou `1.8k` | `1k` (busca) / `50` Hz (fino) | `ATT = AUT` (ou `1`), `SVC = On`, `COF = AUT` |
| **6. Radioamador Voz (`20m` / `15m` / `10m`)** | `SW` | **`USB`** | `2.2k` ou `1.8k` | `1k` (busca) / `50` Hz (fino) | `ATT = AUT` (ou `1`), `SVC = On`, `COF = AUT` |
| **7. Caminhoneiros / PX (`11m`)** | `CB` (ou `SW`) | `AM` (ou `USB`/`LSB`) | `3.0k` (`AM`) / `2.2k` (`SSB`) | `10k` (canais) / `1k` | `ATT = AUT`, `SM = 6` |
| **8. Código Morse / Telegrafia (`CW`)** | `SW` | **`CW`** | *(Fixo)* | `100` Hz ou `50` Hz | `CW = LSB` (abaixo de 10 MHz) ou `USB` |
| **9. Aviação HF / Meteorologia (`VOLMET`)** | `SW` | **`USB`** | `2.2k` | `1k` / `100` Hz | `ATT = AUT`, `SVC = On` |

---

## 2. Roteiro Prático: 1 Exemplo de Cada Modalidade para Testar o Rádio

Siga os **8 testes abaixo** para validar todas as bandas e modulações do seu ATS-20 em São José do Rio Preto:

---

### Teste 1 — FM Comercial de São José do Rio Preto + RDS + S-Meter
Ideal para testar o decodificador **RDS**, a busca automática (**Scan**) e a barra de sinal (**S-Meter**).

- **Como parametrizar no rádio:**
  1. Aperte **`BAND+`** → escolha **`FM`** (`64` a `108` MHz).
  2. Aperte **`BW`** → escolha **`AUTO`**.
  3. Aperte **`STEP`** → escolha **`100k`**.
  4. Segure **`STEP`** por 1 segundo para ativar a barra **S-Meter** no rodapé da tela.
  5. Sintonize uma emissora forte de São José do Rio Preto:
     - **`90.30` MHz** — *Rádio Nativa FM Rio Preto*
     - **`93.10` MHz** — *Gazeta FM Rio Preto*
     - **`99.50` MHz** — *Band FM Rio Preto*
     - **`102.10` MHz** — *Jovem Pan FM Rio Preto*
     - **`106.70` MHz** — *Educativa FM Rio Preto*
  6. Aperte **`MODE`** (toque curto) para ligar o **RDS**: abaixo da frequência aparecerão `...` e, após alguns segundos sincronizando, o **nome da rádio / música**.
  7. Aperte o **Botão do Encoder** para alternar entre os 3 textos do RDS (*Nome da Estação*, *Texto* e *Informação*).

---

### Teste 2 — Ondas Médias (`MW` / AM Comercial à Noite)
Durante o dia você pega emissoras da região; **após as 18h30**, sinais de São Paulo (capital), Paraná, Minas, Goiás e até Argentina/Uruguai chegam em Rio Preto!

- **Como parametrizar no rádio:**
  1. Aperte **`BAND+`** → escolha **`MW`** (`520` a `1710` kHz).
  2. Aperte **`MODE`** → escolha **`AM`**.
  3. Aperte **`BW`** → escolha **`4.0k`** (para emissoras fortes) ou **`2.5k`** (para emissoras distantes).
  4. Aperte **`STEP`** → escolha **`10k`** (no Brasil todas as rádios AM usam múltiplos de `10 kHz`).
- **Exemplos para testar à noite (`19h` às `23h`):**
  - **`700` kHz** — *Nossa Rádio (São Paulo - SP)*
  - **`840` kHz** — *Rádio Bandeirantes (São Paulo - SP)*
  - **`1000` kHz** — *Rádio Record (São Paulo - SP)*
  - **`1100` kHz** — *Rádio Globo / Rádio Capital (`1040` kHz)*
  - *Dica:* Gire o corpo do rádio horizontalmente! No modo `MW` e `LW`, a recepção fica mais forte ou mais fraca conforme você aponta a antena para a cidade transmissora.

---

### Teste 3 — Ondas Curtas Nacionais (`SW` em `AM` — Emissoras do Brasil)
Excelente para testar a recepção de longa distância dentro do Brasil a qualquer hora do dia.

- **Como parametrizar no rádio:**
  1. Aperte **`BAND+`** → escolha **`SW`**.
  2. Aperte **`MODE`** → escolha **`AM`**.
  3. Aperte **`BW`** → escolha **`3.0k`** ou **`4.0k`**.
  4. Aperte **`STEP`** → escolha **`5k`**.
- **Exemplos certeiros em São José do Rio Preto:**
  - **`11780` kHz** *(25 metros)* — **Rádio Nacional da Amazônia (Brasília - DF, 100 kW)**: É a emissora de Ondas Curtas mais forte do Brasil. Pega muito bem durante o dia e início da noite.
  - **`6180` kHz** *(49 metros)* — **Rádio Nacional da Amazônia (Brasília - DF)**: Pega fortíssimo do fim da tarde (**17h30**) até o amanhecer.
  - **`6090` kHz** e **`11925` kHz** — **Rádio Bandeirantes (São Paulo - SP)**.
  - **`9820` kHz** *(31 metros)* — **Rádio Nove de Julho (São Paulo - SP)**: Forte no fim de tarde.
  - **`11750` kHz** e **`9665` kHz** — **Rádio Voz Missionária (Camboriú - SC)**: Chega muito forte no interior de SP.
  - **`4885` kHz** *(60 metros, após as 19h30)* — **Rádio Clube do Pará (Belém - PA)** / **Rádio Difusora Acreana (Rio Branco - AC)**.

---

### Teste 4 — Ondas Curtas Internacionais: Rádio Internacional da China, Europa, Cuba e Relógio Atômico
O melhor horário em São José do Rio Preto é entre **18h00 e 23h00** (horário de Brasília). Fique perto de uma janela com a antena telescópica totalmente esticada.

- **Como parametrizar no rádio:**
  1. **`BAND+`** → **`SW`**
  2. **`MODE`** → **`AM`**
  3. **`BW`** → **`3.0k`** (ou **`2.5k`** se houver chiado)
  4. **`STEP`** → **`5k`**

#### 🇨🇳 Rádio Internacional da China (CRI — Pequim / Retransmissores)
| Frequência | Idioma | Horário em Rio Preto (Brasília) | Observação |
| :--- | :--- | :--- | :--- |
| **`9665` kHz** | **Português** | **19h00 às 20h00** | Faixa de 31m — costuma chegar muito forte em SP |
| **`13650` kHz** / **`11640` kHz** | **Português** | **19h00 às 20h00** | Faixa de 22m / 25m |
| **`7415` kHz** / **`7350` kHz** | **Português** | **20h00 às 21h00** | Faixa de 41m |
| **`5990` kHz** | **Espanhol / Chinês** | **19h00 às 23h59** | Retransmitida via Cuba — sinal fortíssimo no Brasil! |
| **`9580` kHz** / **`9685` kHz** / **`9800` kHz** | **Chinês / Espanhol / Inglês** | **18h30 às 23h00** | Transmissões diretas da Ásia e via Havana |
| **`11860` kHz** / **`15120` kHz** | **Chinês / Espanhol** | **17h30 às 19h30** | Final de tarde |

#### 🌍 Outras Emissoras Internacionais Fortes em Rio Preto
| Frequência | Emissora / País | Idioma | Horário Ideal (Brasília) |
| :--- | :--- | :--- | :--- |
| **`10000` kHz** e **`15000` kHz** | **WWV / WWVH (EUA e Havaí — Relógio Atômico)** | Inglês + Pulsos | **Dia e noite** (ouça o *"tic-tic-tic"* de cada segundo e o anúncio da hora UTC a cada minuto!) |
| **`11940` kHz** e **`12030` kHz** | **Rádio Exterior da Espanha (Madri, Espanha)** | Espanhol / Português | **18h00 às 22h00** (chega muito limpa da Europa) |
| **`6000` kHz**, **`9535` kHz**, **`11760` kHz`**, **`15140` kHz** | **Rádio Havana Cuba (Havana, Cuba)** | Espanhol / Português | **18h00 às 01h00** (sinal muito forte) |
| **`9410` kHz**, **`9915` kHz**, **`12095` kHz** | **BBC World Service (Reino Unido / Ilha de Ascensão)** | Inglês | **18h00 às 21h00** |
| **`9700` kHz**, **`11800` kHz** | **Rádio Romênia Internacional (Bucareste)** | Espanhol / Português | **19h00 às 22h00** |
| **`15770` kHz**, **`9395` kHz**, **`5950` kHz** | **WRMI (Miami, Flórida - EUA)** | Inglês / Espanhol | **18h00 às 23h59** |

---

### Teste 5 — Radioamador em Voz (`40 metros` em `LSB` — A Mais Movimentada do Brasil!)
Na faixa de **40 metros (`7000` a `7300` kHz)** você ouvirá radioamadores de todo o estado de São Paulo, Minas, Paraná, Rio, Sul e Nordeste conversando ao vivo no fim da tarde, à noite e pela manhã.

- **Como parametrizar no rádio:**
  1. Aperte **`BAND+`** → escolha **`SW`**.
  2. Aperte **`MODE`** → escolha **`LSB`** *(muito importante: abaixo de 10 MHz usa-se sempre `LSB`!)*.
  3. Aperte **`BW`** → escolha **`2.2k`** ou **`1.8k`** *(filtro estreito específico para voz em SSB)*.
  4. Aperte o **Botão do Encoder** (ou **`STEP`**) → escolha **`1k`** para procurar conversas entre **`7050.00` kHz e `7160.00` kHz**.
  5. **Como deixar a voz natural (Sintonia Fina):** Quando encontrar alguém falando (a voz pode parecer "Pato Donald" ou grave demais), aperte o **Botão do Encoder**, mude o passo para **`100` Hz** ou **`50` Hz** e gire 1 ou 2 cliques até a voz ficar perfeitamente natural!
- **Frequências típicas em 40m (`LSB`):**
  - **`7050` a `7150` kHz (`LSB`)**: Rodadas e conversas entre radioamadores brasileiros (muito ativa das **17h00 às 23h00** e das **06h30 às 09h30**).
- **Para testar também em 80 metros à noite (`20h00` às `06h00`):**
  - Mantenha em **`LSB`**, filtro **`2.2k`**, e procure entre **`3700.00` e `3780.00` kHz** (rodadas paulistas e do interior).

---

### Teste 6 — Radioamador Internacional DX (`20m` e `10m` em `USB` — Durante o Dia e Fim de Tarde)
Acima de 10 MHz, a convenção mundial muda para **`USB` (Upper Sideband)**.

- **Como parametrizar no rádio:**
  1. Aperte **`BAND+`** → escolha **`SW`**.
  2. Aperte **`MODE`** → escolha **`USB`**.
  3. Aperte **`BW`** → escolha **`2.2k`**.
  4. Aperte o **Botão do Encoder** → passo **`1k`** para buscar, depois **`50` Hz** para clarificar a voz.
- **Onde procurar:**
  - **Faixa de 20 metros (`14150.00` a `14300.00` kHz em `USB`)**: Entre **14h00 e 20h00**, você ouve estações da América do Sul, EUA e Europa fazendo contatos internacionais (*"CQ DX"*).
  - **Faixa de 10 metros (`28300.00` a `28600.00` kHz em `USB`)**: Em dias de sol entre **10h00 e 17h00**, abre propagação para os EUA, Caribe e Europa.

---

### Teste 7 — Faixa do Cidadão / PX (`11 metros` / Caminhoneiros e Estações PX)
Como São José do Rio Preto é cortada pela **BR-153** e pela **Rodovia Washington Luís (SP-310)**, há bastante movimento de caminhoneiros e estações base de PX.

- **Como parametrizar no rádio:**
  1. Aperte **`BAND+`** → gire até a sub-banda **`CB`** (ou sintonize em `SW` a partir de `26965` kHz).
  2. Aperte **`MODE`** → escolha **`AM`** para canais de rodovia, ou **`USB`/`LSB`** para canais de longa distância (DX em PX).
  3. Aperte **`BW`** → **`3.0k`** (em `AM`) ou **`2.2k`** (em `USB`/`LSB`).
  4. Aperte **`STEP`** → **`10k`** ou **`5k`**.
- **Canais principais do PX para testar:**
  - **`27015` kHz (`AM`)** — **Canal 5** (caminhoneiros em comboio nas rodovias)
  - **`27185` kHz (`AM`)** — **Canal 19** (canal oficial dos caminhoneiros na BR-153 e SP-310)
  - **`27455` kHz (`USB` ou `LSB`)** — **Canal 45** (canal de chamada nacional/internacional em SSB — durante o dia pega o Brasil inteiro!)
  - **`27555` kHz (`USB`)** — Frequência internacional de chamada DX em 11 metros.

---

### Teste 8 — Código Morse (`CW`) e Aviação Oceânica (`USB` — VOLMET)
- **Para ouvir Código Morse (`CW`):**
  1. Vá para **`7005.00` a `7035.00` kHz** (à noite) ou **`14005.00` a `14040.00` kHz** (de dia/tarde).
  2. Aperte **`MODE`** até aparecer **`CW`**.
  3. Use passo de **`100` Hz** ou **`50` Hz** para centrar o apito ("bip-bip-bip") do código Morse no tom mais agradável.
- **Para ouvir Meteorologia e Controle de Aviação em HF (`USB`):**
  1. Coloque em **`MODE` = `USB`** e **`BW` = `2.2k`**.
  2. Sintonize **`8825.00` kHz**, **`8906.00` kHz** ou **`13267.00` kHz** (`USB`) à noite para ouvir comunicações de aeronaves cruzando o Atlântico e boletins meteorológicos aeronáuticos (*VOLMET*).

---

## 3. Quatro Dicas de Ouro para Multiplicar a Recepção do seu ATS-20

1. **Desligue a tela OLED ao ouvir sinais fracos (Botão `AGC`):**
   - Depois de sintonizar uma estação distante (como a Rádio da China ou um radioamador fraco), dê **um toque curto no botão `AGC`**. Isso apaga a tela OLED, economiza bateria e elimina a interferência elétrica interna da tela (o sinal fica visivelmente mais limpo!). Basta apertar `AGC` de novo para reacender.
2. **Fuja de fontes chaveadas dentro de casa:**
   - Carregadores de celular, fontes de notebook, lâmpadas LED baratas e roteadores Wi-Fi jogam muito chiado nas Ondas Curtas (`SW`) e Ondas Médias (`MW`). Ouvir na **varanda, quintal ou perto da janela**, usando apenas a bateria interna do rádio, reduz 80% do chiado.
3. **Antena de Fio Simples (5 a 10 metros) para `40m` / `80m` / `SW`:**
   - A antena telescópica original tem ~70 cm (ótima para FM e para frequências acima de 11 MHz, mas curta para `40m` e `60m`).
   - Se você prender um pedaço de **fio elétrico comum de 5 a 10 metros** com uma garra jacaré na ponta da antena telescópica e esticar esse fio até a janela ou varanda, o ATS-20 passa a captar dezenas de estações que antes não apareciam!
4. **Se uma antena externa deixar o sinal "saturado":**
   - Abra o menu em **`BAND-`**, selecione **`ATT`** (Atenuador) e mude de `AUT` para um valor manual entre **`5` e `20`**. Isso evita sobrecarga no chip Si4735 quando há sinais muito fortes.
