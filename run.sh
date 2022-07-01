
#make && sudo ./hping3 -S -p 80 --vxlan-source-addr 172.29.0.134 --vxlan-dest-addr 172.29.0.129 --vxlan-source-port 4789 --vxlan-dest-port 4789 --vxlan-vni 128512 172.29.0.128 --spoof 172.29.0.135 --vxlan-source-mac 55:55:55:55:55:55 --vxlan-dest-mac dd:00:55:ff:ff:ff


#make && sudo ./hping3 -S -p 80 --vxlan-source-addr 172.29.0.134 --vxlan-dest-addr 172.29.0.129 --vxlan-source-port 4789 --vxlan-dest-port 4789 --vxlan-vni 128512 172.29.0.128 --spoof 172.29.0.135 --vxlan-source-mac dd:00:55:ff:ff:ff



#sudo time  ./hping3 -S -p 80 --vxlan-source-addr 172.29.0.134 --vxlan-dest-addr 172.29.0.129 --vxlan-source-port 4789 --vxlan-dest-port 4789 --vxlan-vni 128512 172.29.0.128 --vxlan-source-mac 55:55:42:42:55:55 --vxlan-dest-mac dd:00:55:ff:ff:ff -c 10


#sudo time  ./hping3 -1 --vxlan-source-addr 172.29.0.134 --vxlan-dest-addr 172.29.0.129 --vxlan-source-port 4789 --vxlan-dest-port 4789 --vxlan-vni 128512 172.29.0.128 --vxlan-source-mac 55:55:42:42:55:55 --vxlan-dest-mac dd:00:55:ff:ff:ff -c 10


make && (echo IPv6;sudo ./hping3 --inet6 -I eth0 -D -c 1 www.kramse.org) || exit 1

echo;echo

make && (echo IPv4;sudo ./hping3 -I eth0 -D -c 1 www.kramse.org)
