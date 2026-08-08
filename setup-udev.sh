#!/bin/bash
# Setup script for GK850W plugin - adds user to plugdev group
# Does NOT create new udev rules (uses existing OpenRGB defaults)

echo "Checking system configuration..."

# Check if user is in plugdev group
if groups "$USER" | grep -q plugdev; then
    echo "✅ User '$USER' is already in 'plugdev' group"
else
    echo "❌ User '$USER' is NOT in 'plugdev' group"
    echo ""
    echo "Run the following command to add user to plugdev:"
    echo "  sudo usermod -aG plugdev $USER"
    echo ""
    echo "Then log out and log back in, or run: sg plugdev bash"
fi

# Check udev rules exist
if [ -f /etc/udev/rules.d/60-openrgb.rules ] || ls /lib/udev/rules.d/*openrgb* &>/dev/null; then
    echo "✅ OpenRGB udev rules found"
else
    echo "⚠️  No OpenRGB udev rules found"
    echo ""
    echo "Install OpenRGB package to get default rules:"
    echo "  sudo apt install openrgb-client openrgb-server"
    echo ""
    echo "Or manually create rules file:"
    echo "  sudo tee /etc/udev/rules.d/60-openrgb-hid.rules << 'EOF'"
    echo 'SUBSYSTEM=="hidraw", KERNEL=="hidraw*", MODE="0660", GROUP="plugdev"'
    echo "EOF"
fi

echo ""
echo "After adding to plugdev group, reload udev rules:"
echo "  sudo udevadm control --reload-rules && sudo udevadm trigger"
echo ""
echo "Test device access:"
echo "  ls -la /dev/hidraw* | head -10"
