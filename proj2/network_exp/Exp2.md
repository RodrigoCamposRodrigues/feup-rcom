# 2.2 Implement two bridges in a switch

## Steps

1. Abrir o GKTerm no Tux42 e configurar a baudrate para 115200;
2. Resetar as configurações do switch com o seguinte comando:
 
     ```bash
     > /system reset-configuration
     > y
     ```
3. Conectar o Tux42_E0 ao switch (porta 24) e configurar a ligação com os seguintes comandos:
 
     ```bash
     $ ifconfig eth0 up
     $ ifconfig eth0 172.16.41.1/24
     ```
4. Criar 2 bridges no switch
 
     ```bash
     > /interface bridge add name=bridge40
     > /interface bridge add name=bridge41
     ```
5. Eliminar as portas as quais o Tux42, 43 e 44 estão ligados por defeito
 
     ```bash
     > /interface bridge port remove [find interface=ether1] 
     > /interface bridge port remove [find interface=ether2] 
     > /interface bridge port remove [find interface=ether24] 
     ```
6. Adicionar as novas portas
 
     ```bash
     > /interface bridge port add bridge=bridge40 interface=ether1
     > /interface bridge port add bridge=bridge40 interface=ether2 
     > /interface bridge port add bridge=bridge41 interface=ether24
     ```
7. Começar a captura do eth0 do Tux53
 
8. Desde o Tux43, começar a captura do eth0 e fazer ping para o Tux44 e Tux42. (Tux44 - ping feito com sucesso, Tux42 - Network is unreachable)
 
   ```bash
   $ ping 172.16.40.254
   $ ping 172.16.41.1
   ```
![Alt text](/img/exp2-ping-tux43-to-44-and-42.png)

9. Começar captura do eth0 no Tux43. No Tux43, executar o seguinte comando e observar e guardar os resultados:

   ```bash
   $ ping -b 172.16.40.255
   ``` 
![Alt text](/img/exp2-ping-broadcast-from-tux43.png)

10. Começar captura do eth0 no Tux42. No Tux42, executar o seguinte comando e observar e guardar os resultados:

   ```bash
   $ ping -b 172.16.41.255
   ``` 

![Alt text](/img/exp2-ping-broadcast-from-tux42.png)