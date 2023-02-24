sudo ip tuntap add mode tun dev tun0
sudo ip addr add 10.0.0.2/24 peer 10.0.0.1 dev tun0
sudo ip link set tun0 up

sudo ip route add default via 10.0.0.1 dev tun0