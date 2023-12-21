# 2.3 Configure a Router in Linux

## Steps

1. Ligar eth1 do Tux44 à porta 23 do switch. Configurar eth1 do Tux44

   ```bash
      ifconfig eth1 up
      ifconfig eth1 172.16.41.253/24
   ```
2. Eliminar as portas as quais o Tux44 esta ligado por defeito e adicionar a nova porta
   ```bash
   >     /interface bridge port remove [find interface=ether4]
   >     /interface bridge port add bridge=bridge51 interface=ether4
   ```
3. Ativar *ip forwarding* e desativar ICMP no Tux44

 ```bash
   sysctl net.ipv4.ip_forward=1
   sysctl net.ipv4.icmp_echo_ignore_broadcasts=0
 ```
4. Observar MAC e Ip no Tux44_eth0 (IP:172.16.40.254, MAC: 00:21:5a:73:78:76) e Tux44_eth1 (IP: 172.16.41.253, MAC: 00:c0:df:02:55:95)

 ```bash
     ifconfig
 ```
5. Adicionar as seguintes rotas no Tux42 e Tux43 para que estes consigam comunicar um com o outro através do Tux44

   ```bash
      route add -net  172.16.40.0/24 gw 172.16.41.253 
      route add -net  172.16.41.0/24 gw 172.16.40.254
   ```
 
6. Começar a captura no Tux 43 e fazer ping aos seguintes endereços desde o Tux43

Ping do Tux43 para o Tux44_eth0

 ```bash
     ping 172.16.40.254
 ```

![Alt text](/img/exp3-ping-tux43-to-tux44eth0.png)

Ping do Tux43 para o Tux44_eth1

```bash
   ping 172.16.41.253
```

![Alt text](/img/exp3-ping-tux43-to-tux44eth1.png)

Ping do Tux43 para o Tux42_eth0

```bash
   ping 172.16.41.1
```

![Alt text](/img/exp3-ping-tux43-to-tux42.png)

7. Começar captura do eth0 e eth1 do Tux44 no Wireshark
 
8. Limpar as tabelas ARP em todos os Tux
 ```bash
     arp -d 172.16.41.253 #Tux52
     arp -d 172.16.40.254 #Tux53
     arp -d 172.16.40.1 #Tux54
     arp -d 172.16.41.1 #Tux54
 ```
9. Fazer ping para o Tux42 a partir do Tux43

   ```bash
      ping 172.16.41.1
   ```
Tux44_eth0:
![Alt text](/img/exp3-tux44-eth0.png)

Tux44_eth1:
![Alt text](/img/exp3-ping-tux43-to-tux44eth1.png)
