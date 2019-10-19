# To install killall
yum install psmisc

-----------------------------------------------------

# cat /etc/systemd/system/fastmorph.service
[Unit]
Description=Fastmorph Service
#After=mariadb.service

[Service]
Type=idle
User=root
ExecStart=/home/mansur/morph6/systemd/fastmorph.sh
#ExecStop=/usr/bin/killall -9 fastmorph

[Install]
WantedBy=multi-user.target

-----------------------------------------------------

# cat /etc/systemd/system/fastngrams.service 
[Unit]
Description=Fastngrams Service
#After=mariadb.service

[Service]
Type=idle
User=root
ExecStart=/home/mansur/morph6/systemd/fastngrams.sh
#ExecStop=/usr/bin/killall -9 fastngrams

[Install]
WantedBy=multi-user.target

-----------------------------------------------------
