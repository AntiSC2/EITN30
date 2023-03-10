sudo ip tuntap add mode tun dev tun0
sudo ip addr add 10.0.0.1/24 dev tun0
sudo ip link set dev tun0 mtu 1000
sudo ip link set tun0 up

sudo iptables -t nat -A POSTROUTING -o wlan0 -j MASQUERADE
sudo iptables -A FORWARD -i wlan0 -o tun0 -m state --state RELATED,ESTABLISHED -j ACCEPT
sudo iptables -A FORWARD -i tun0 -o wlan0 -j ACCEPT

cmake -DCMAKE_BUILD_TYPE=Release -DMTU=1000 -DBASE=TRUE src/.