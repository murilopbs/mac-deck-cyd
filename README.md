# 🎛️ MacDeck CYD — Stream Deck Bluetooth BLE para macOS

> Um **Stream Deck físico e sem fio** construído com a placa **ESP32-2432S028 (CYD - Cheap Yellow Display 2.8" Touch)** para controlar mídias, microfone de reuniões e atalhos no **macOS (Apple Silicon M1/M2/M3/M4 e Intel)**.

[![ESP32](https://img.shields.io/badge/Hardware-ESP32--2432S028-blue.svg)](https://github.com/murilopbs/mac-deck-cyd)
[![Bluetooth](https://img.shields.io/badge/Protocol-Bluetooth_BLE_HID-blueviolet.svg)](https://github.com/murilopbs/mac-deck-cyd)
[![macOS](https://img.shields.io/badge/Platform-macOS_Apple_Silicon-000000.svg?logo=apple)](https://github.com/murilopbs/mac-deck-cyd)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

---

## ⚡ Por que este projeto é especial?

1. **Zero Instalação no Mac**: O ESP32 utiliza o protocolo padrão da indústria **Bluetooth Low Energy HID (Human Interface Device)**. O macOS reconhece o dispositivo nativamente como um teclado/controlador multimídia. Não é necessário instalar nenhum aplicativo, driver ou extensão de kernel no Mac.
2. **Compatibilidade Nativa Apple Silicon**: Funciona diretamente com a arquitetura ARM dos chips **M1, M2, M3 e M4** no macOS Sequoia, Sonoma, Ventura e Monterey.
3. **Tela Colorida Touch 320x240**: Grade de 6 botões grandes (2 linhas x 3 colunas) com ícones vetoriais modernos, cores vivas e feedback tátil/visual imediato ao tocar.
4. **Eficiência Extrema de Memória**: Utiliza a stack moderna `NimBLE 2.5.x`, consumindo apenas **11% de RAM** e metade da memória Flash do ESP32, garantindo conexão instantânea e sem atrasos (latência < 5ms).
5. **Feedback Físico por LED**: O LED RGB traseiro onboard da placa CYD emite um flash luminoso verde a cada comando disparado com sucesso para o Mac (ou vermelho se desconectado).

---

## 🎮 Mapa dos 6 Botões Padrão

```
┌─────────────────────────────────────────────────────────────┐
│  [MACDECK BLE]                      [● CONECTADO]           │
├───────────────────┬─────────────────────┬───────────────────┤
│    [ ▶ ❚❚ ]       │       [ ▶▶| ]       │       [ 🔇 ]      │
│   PLAY/PAUSE      │       PROXIMO       │        MUDO       │
│  Spotify/Mídia    │      Next Track     │     Audio Mute    │
├───────────────────┼─────────────────────┼───────────────────┤
│    [ 🎙️ / ]       │       [ 📸 ]        │       [ 🔒 ]      │
│    MIC MUTE       │       CAPTURA       │     TRAVAR MAC    │
│   Meet / Zoom     │     Cmd+Shift+4     │    Lock Screen    │
└───────────────────┴─────────────────────┴───────────────────┘
```

| # | Botão | Ação / Atalho | Finalidade |
| :---: | :--- | :--- | :--- |
| **1** | **PLAY / PAUSE** | `Consumer::MEDIA_PLAY_PAUSE` | Toca ou pausa Spotify, Apple Music ou YouTube |
| **2** | **PRÓXIMO** | `Consumer::MEDIA_NEXT_TRACK` | Avança para a próxima música |
| **3** | **MUDO** | `Consumer::MEDIA_MUTE` | Silencia / ativa o áudio do sistema do Mac |
| **4** | **MIC MUTE** | `Cmd + Shift + M` | Muta/desmuta o microfone no Google Meet / Zoom |
| **5** | **CAPTURA** | `Cmd + Shift + 4` | Dispara a ferramenta de captura de área da tela |
| **6** | **TRAVAR MAC** | `Cmd + Ctrl + Q` | Bloqueia a tela do Mac instantaneamente ao levantar |

---

## 🌐 Portal Web Local Embarcado (`http://macdeck.local`)

O MacDeck CYD possui um **servidor web HTTP e mDNS nativo** rodando diretamente no ESP32:
- **Acesse pelo Navegador**: Conectado à mesma rede Wi-Fi, basta abrir [http://macdeck.local](http://macdeck.local) no Safari, Chrome ou pelo celular.
- **Controle Remoto Touch**: Espelha os 6 botões físicos na tela do celular — você pode acionar os atalhos do Mac deitado na cama ou em outra parte da mesa!
- **Configuração de Wi-Fi sem Sofrimento**: Escaneie as redes locais e digite a senha direto pelo smartphone.
- **Modo Contingência (SoftAP)**: Se nenhuma rede estiver configurada ou fora de alcance, a placa cria a rede Wi-Fi **`MacDeck-Setup`** (IP `192.168.4.1`) para você configurar em segundos.
- **Pronto para Spotify Web API**: Aba dedicada para salvar `Client ID`, `Client Secret` e `Refresh Token`.

---

## 🛠️ Pinagem do Hardware (ESP32-2432S028 CYD)

| Periférico | Pinos ESP32 | Descrição |
| :--- | :--- | :--- |
| **Display ST7789** | `MOSI: 13`, `SCLK: 14`, `CS: 15`, `DC: 2`, `BL: 21` | SPI HSPI (320x240, Rotação 3) |
| **Touch XPT2046** | `MOSI: 32`, `MISO: 39`, `CLK: 25`, `CS: 33`, `IRQ: 36` | SPI VSPI Calibrado |
| **LED RGB Onboard** | `Red: 4`, `Green: 16`, `Blue: 17` | Ativo em nível BAIXO (LOW) |

---

## 📲 Como Usar e Emparelhar

1. Conecte o **ESP32 CYD** na porta USB.
2. Na primeira vez, conecte o celular à rede Wi-Fi **`MacDeck-Setup`** e acesse `http://192.168.4.1` para configurar o Wi-Fi da sua casa.
3. No seu Mac, abra **Ajustes do Sistema > Bluetooth**, localize **`MacDeck CYD`** e clique em **Conectar**.
4. A tela exibirá o badge verde **`[● CONECTADO]`** e o link **`http://macdeck.local`** no rodapé.
5. Use os botões na tela física ou no seu navegador web!

---

## 💻 Como Compilar e Gravar

### Pré-requisitos
* [Arduino IDE](https://www.arduino.cc/en/software) ou `arduino-cli`.
* Pacote ESP32 instalado nas Placas (`esp32` v3.x ou superior).

### Bibliotecas Necessárias
Instale pelo Gerenciador de Bibliotecas da Arduino IDE:
1. `NimBLE-Arduino` (v2.5.x ou superior)
2. `HijelHID_BLEKeyboard`
3. `ArduinoJson` (v7.x)
4. `Adafruit GFX Library`
5. `Adafruit ST7789 and ST7735 Library`
6. `XPT2046_Touchscreen`

### Esquema de Partição Recomendado
No menu da Arduino IDE:
* **Tools > Partition Scheme > "Huge APP (3MB No OTA/1MB SPIFFS)"**

### Gravação via CLI
```bash
arduino-cli compile --fqbn esp32:esp32:esp32:PartitionScheme=huge_app macDeckCYD/
arduino-cli upload -p /dev/cu.usbserial-* --fqbn esp32:esp32:esp32:PartitionScheme=huge_app macDeckCYD/
```

---

## ⚙️ Como Personalizar os Botões

No arquivo [`macDeckCYD.ino`](macDeckCYD.ino), você pode alterar livremente os textos, ícones e atalhos na estrutura `buttons`:

```cpp
// Exemplo: Botão para abrir o Spotlight / Raycast (Cmd + Espaço)
{
  10, 36, 94, 86,
  "RAYCAST", "Spotlight",
  ICON_SCREENSHOT, COLOR_CYAN,
  ACT_MACRO, 0, KEY_MOD_LGUI, KEY_SPACE,
  false
}
```

---

## 📄 Licença
Distribuído sob a licença MIT. Sinta-se livre para usar, modificar e compartilhar!
Criado com ❤️ por [Murilo P. B. S.](https://github.com/murilopbs).
