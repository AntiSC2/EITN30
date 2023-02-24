sudo ip tuntap add mode tun dev tun0
sudo ip addr add 10.0.0.1/24 peer 10.0.0.2 dev tun0
sudo ip link set tun0 up

sudo iptables -t nat -A POSTROUTING -o tun0 -j MASQUERADE
sudo iptables -A FORWARD -i wlan0 -o tun0 -m state --state RELATED,ESTABLISHED -j ACCEPT
sudo iptables -A FORWARD -i tun0 -o wlan0 -j ACCEPT