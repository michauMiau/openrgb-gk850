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

**Uwaga:** Istniejące reguły OpenRGB (`60-openrgb.rules`) pokrywają tylko PID `258a:010c` dla klawiatur Sinowealth, ale nasz GK850W ma PID `258a:0049` — wymaga dodatkowej reguły.

### Opcja 1: Reguła udev (zalecane)

Dodaj plik `/etc/udev/rules.d/99-gk850w-plugin.rules`:

```bash
echo 'SUBSYSTEMS=="usb|hidraw", ATTRS{idVendor}=="258a", ATTRS{idProduct}=="0049", TAG+="uaccess"' | sudo tee /etc/udev/rules.d/99-gk850w-plugin.rules

sudo udevadm control --reload-rules
sudo udevadm trigger
```

Dodaj użytkownika do grupy plugdev:
```bash
sudo usermod -aG plugdev $USER
# Wyloguj się i zaloguj ponownie (lub: sg plugdev bash)
```

### Opcja 2: Tymczasowe uprawnienia

Jeśli nie chcesz modyfikować reguł systemowych, możesz tymczasowo zmienić uprawnienia:
```bash
sudo chmod 666 /dev/hidraw*
```

**Uwaga:** To wymaga powtórzenia po każdym ponownym podłączeniu urządzenia.

## Troubleshooting

Jeśli plugin wyświetla "Failed to open via path: ... Brak dostępu":
1. Sprawdź czy użytkownik należy do grupy plugdev: `groups $USER`
2. Sprawdź czy istnieją pliki `/dev/hidraw*`: `ls -la /dev/hidraw*`
3. Upewnij się że reguła udev jest aktywna: `udevadm info /dev/hidrawX | grep TAG`
4. Sprawdź czy nie ma konfliktu z innym oprogramowaniem (Razer Synapse, etc.)

## Troubleshooting — "Unknown error" przy ładowaniu pluginu

Jeśli OpenRGB pokazuje "Unknown error" zamiast wczytać plugin:
- Upewnij się że używasz Qt 6.4.2 (AppImage zawiera tę wersję)
- Sprawdź czy plugin jest skompilowany z poprawnym qmake: `~/Qt/6.4.2/gcc_64/bin/qmake`
- Spróbuj uruchomić OpenRGB z `--verbose` aby zobaczyć szczegółowy błąd

## Urządzenie
- VID:PID: `258A:0049`
- Product string: "GK850"
- Report size: 1032 bajty (Report ID 6)

## Repozytorium OpenRGB
https://gitlab.com/CalcProgrammer1/OpenRGB
