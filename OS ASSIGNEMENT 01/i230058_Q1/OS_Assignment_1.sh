#!/bin/bash

# OS_Assignment_1.sh
# Name: Shah Faisal
# Roll: 23I-0058

echo "23I-0058-SHAH-FAISAL"

create_user() {
    sudo useradd -m OS_Assignment_1
    echo "OS_Assignment_1:12345" | sudo chpasswd
    sudo usermod -aG sudo OS_Assignment_1
    echo "User OS_Assignment_1 is created, and administrator privileges are assigned"
}
list_apps() {
    echo "Listing installed applications:"
    apt list --installed 2>/dev/null | head -20
}
install_app() {
    echo "Installing Dropbox..."
    sudo apt update
    wget -O /tmp/dropbox.deb "https://www.dropbox.com/download?dl=packages/ubuntu/dropbox_2020.03.04_amd64.deb"
    sudo dpkg -i /tmp/dropbox.deb
    sudo apt-get install -f -y
    echo "Dropbox installation completed"
    rm /tmp/dropbox.deb
}

configure_network() {
    echo "Configuring network settings..."
    if [ -d /etc/netplan ]; then
        NETPLAN_FILE="/etc/netplan/01-netcfg.yaml"
	#Create netplan configuration
        sudo tee $NETPLAN_FILE > /dev/null <<EOF
network:
  version: 2
  ethernets:
    eth0:
      addresses:
        - 10.0.0.1/24
      routes:
        - to: default
          via: 10.0.0.254
      nameservers:
        addresses: [8.8.8.8]
EOF
        sudo netplan apply
    else
        sudo tee -a /etc/network/interfaces > /dev/null <<EOF
auto eth0
iface eth0 inet static
    address 10.0.0.1
    netmask 255.255.255.0
    gateway 10.0.0.254
    dns-nameservers 8.8.8.8
EOF
        sudo systemctl restart networking
    fi 
    echo "Network configured with:"
    echo "IP Address: 10.0.0.1"
    echo "Subnet Mask: 255.255.255.0"
    echo "Gateway: 10.0.0.254"
    echo "DNS: 8.8.8.8"
}
show_help() {
    echo "Available switches:"
    echo "  -uc     : Create a new user (OS_Assignment_1) with admin privileges"
    echo "  -ld     : List all installed applications"
    echo "  -ins    : Install Dropbox application"
    echo "  -ipcon  : Configure network settings (10.0.0.1/24)"
    echo "  -help   : Display this help information"
    echo ""
    echo "Usage examples:"
    echo "  ./OS_Assignment_1.sh -uc"
    echo "  ./OS_Assignment_1.sh -ld"
    echo "  ./OS_Assignment_1.sh -ins"
    echo "  ./OS_Assignment_1.sh -ipcon"
}
case "$1" in
    -uc)
        create_user
        ;;
    -ld)
        list_apps
        ;;
    -ins)
        install_app
        ;;
    -ipcon)
        configure_network
        ;;
    -help)
        show_help
        ;;
    *)
        echo "Error: Invalid option '$1'"
        echo "Use ./OS_Assignment_1.sh -help for usage information"
        exit 1
        ;;
esac