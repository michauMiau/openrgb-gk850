# OpenRGB GK850W Plugin

Plugin implementujący wsparcie dla klawiatury BY Tech / Mad Dog GK850(W) z kontrolerem Sinowealth.

## Budowanie

### Wymagania
- Qt 6.4.2 (AppImage compatible)
- hidapi
- OpenRGB headers (`~/OpenRGB/headers/OpenRGB/` i `~/OpenRGB/RGBController/`)

```bash
cd plugin
~/Qt/6.4.2/gcc_64/bin/qmake OpenGK850WPlugin.pro
make -j1
```

### Uruchomienie
Skopiuj `libOpenGK850WPlugin.so` do `~/.config/OpenRGB/plugins/`.

## Uprawnienia HID (Linux)

Jeśli plugin nie wykrywa klawiatury z błędem "Brak dostępu", upewnij się że:

1. **Masz zainstalowane domyślne reguły OpenRGB:**
   ```bash
   sudo apt install openrgb-client openrgb-server  # lub inny pakiet OpenRGB
   # Lub ręcznie dodaj:
   echo 'SUBSYSTEM=="hidraw", KERNEL=="hidraw*", ATTRS{idVendor}=="258a", MODE="0660", GROUP="plugdev"' | sudo tee /etc/udev/rules.d/99-gk850w-udev.rules
   ```

2. **Dodaj użytkownika do grupy plugdev:**
   ```bash
   sudo usermod -aG plugdev $USER
   ```

3. **Przeładuj reguły udev i zaloguj się ponownie:**
   ```bash
   sudo udevadm control --reload-rules
   sudo udevadm trigger
   # Wyloguj się i zaloguj ponownie (lub: sg plugdev bash)
   ```

4. **Sprawdź urządzenie:**
   ```bash
   ls -la /dev/hidraw* | grep -E "258a|gk"
   ```

### Alternatywnie — uruchom OpenRGB jako root (niezalecane):
```bash
sudo openrgb
```

## Urządzenie
- VID:PID: `258A:0049`
- Product string: "GK850"
- Report size: 1032 bajty (Report ID 6)

## Troubleshooting

Jeśli plugin wyświetla "Failed to open via path: ... Brak dostępu":
- Sprawdź czy użytkownik należy do grupy plugdev: `groups $USER`
- Sprawdź czy istnieją pliki `/dev/hidraw*`: `ls -la /dev/hidraw*`
- Upewnij się że nie ma konfliktu z innym oprogramowaniem (Razer Synapse, etc.)

## Repozytorium OpenRGB
https://gitlab.com/CalcProgrammer1/OpenRGB
