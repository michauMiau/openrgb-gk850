#!/bin/bash
# Setup script for GK850W plugin - adds udev rule for keyboard access
# This provides access to hidraw devices for VID:PID 258a:0049

echo "Setting up udev rules for GK850W keyboard..."

sudo tee /etc/udev/rules.d/99-gk850w-plugin.rules << 'EOF'
SUBSYSTEMS=="usb|hidraw", ATTRS{idVendor}=="258a", ATTRS{idProduct}=="0049", TAG+="uaccess"
EOF

echo "Reloading udev rules..."
sudo udevadm control --reload-rules
sudo udevadm trigger

echo ""
echo "✅ UDEV rule installed!"
echo ""
echo "📋 Next steps:"
echo "   1. Add user to plugdev group: sudo usermod -aG plugdev \$USER"
echo "   2. Log out and log back in (or run: sg plugdev bash)"
echo "   3. Test with: ls -la /dev/hidraw* | grep gk"
