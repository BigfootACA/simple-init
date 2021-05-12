#include<stddef.h>
#include<sys/stat.h>
#include"assets.h"

static char _etc_profile[]=
	"if [ -z \"${IN_BASHRC}\" ]&&[ -n \"${BASH}\" ]\nthen source /etc/bashrc\nelse [ \"$(id -u)\" == 0 ]&&PS1='# '|"
	"|PS1='$ '\nfi\nunset IN_BASHRC\nexport PATH=\"/usr/bin\"\nif [ -r \"/etc/environment\" ]\nthen\twhile read -r "
	"i\n\tdo export \"${i}\"\n\tdone<\"/etc/environment\"\nfi\nif [ -r \"/etc/locale.conf\" ]\nthen\twhile read -r "
	"i\n\tdo export \"${i}\"\n\tdone<\"/etc/locale.conf\"\nfi\nfor i in /etc/profile.d/*.sh\ndo [ -r \"$i\" ]&&sour"
	"ce \"${i}\"\ndone\nunset i\n";

static char _etc_protocols[]=
	"ip 0 IP\nhopopt 0 HOPOPT\nicmp 1 ICMP\nigmp 2 IGMP\nggp 3 GGP\nipencap 4 IP-ENCAP\nst 5 ST\ntcp 6 TCP\negp 8 E"
	"GP\nigp 9 IGP\npup 12 PUP\nudp 17 UDP\nhmp 20 HMP\nxns-idp 22 XNS-IDP\nrdp 27 RDP\niso-tp4 29 ISO-TP4\ndccp 33"
	" DCCP\nxtp 36 XTP\nddp 37 DDP\nidpr-cmtp 38 IDPR-CMTP\nipv6 41 IPv6\nipv6-route 43 IPv6-Route\nipv6-frag 44 IP"
	"v6-Frag\nidrp 45 IDRP\nrsvp 46 RSVP\ngre 47 GRE\nesp 50 IPSEC-ESP\nah 51 IPSEC-AH\nskip 57 SKIP\nipv6-icmp 58 "
	"IPv6-ICMP\nipv6-nonxt 59 IPv6-NoNxt\nipv6-opts 60 IPv6-Opts\nrspf 73 RSPF CPHB\nvmtp 81 VMTP\neigrp 88 EIGRP\n"
	"ospf 89 OSPFIGP\nax.25 93 AX.25\nipip 94 IPIP\netherip 97 ETHERIP\nencap 98 ENCAP\npim 103 PIM\nipcomp 108 IPC"
	"OMP\nvrrp 112 VRRP\nl2tp 115 L2TP\nisis 124 ISIS\nsctp 132 SCTP\nfc 133 FC\nmobility-header 135 Mobility-Heade"
	"r \nudplite 136 UDPLite\nmpls-in-ip 137 MPLS-in-IP\nmanet 138\nhip 139 HIP\nshim6 140 Shim6\nwesp 141 WESP\nro"
	"hc 142 ROHC\n";

static char _etc_services[]=
	"tcpmux 1/tcp\necho 7/tcp\necho 7/udp\ndiscard 9/tcp sink null\ndiscard 9/udp sink null\nsystat 11/tcp users\nd"
	"aytime 13/tcp\ndaytime 13/udp\nnetstat 15/tcp\nqotd 17/tcp quote\nmsp 18/tcp\nmsp 18/udp\nchargen 19/tcp ttyts"
	"t source\nchargen 19/udp ttytst source\nftp-data 20/tcp\nftp 21/tcp\nfsp 21/udp fspd\nssh 22/tcp\nssh 22/udp\n"
	"telnet 23/tcp\nsmtp 25/tcp mail\ntime 37/tcp timserver\ntime 37/udp timserver\nrlp 39/udp resource\nnameserver"
	" 42/tcp name\nwhois 43/tcp nicname\nre-mail-ck 50/tcp\nre-mail-ck 50/udp\ndomain 53/tcp nameserver\ndomain 53/"
	"udp nameserver\nmtp 57/tcp\nbootps 67/tcp\nbootps 67/udp\nbootpc 68/tcp\nbootpc 68/udp\ntftp 69/udp\ngopher 70"
 	"/tcp\ngopher 70/udp\nrje 77/tcp netrjs\nfinger 79/tcp\nwww 80/tcp http\nwww 80/udp\nlink 87/tcp ttylink\nkerbe"
	"ros 88/tcp kerberos5 krb5\nkerberos 88/udp kerberos5 krb5\nsupdup 95/tcp\nhostnames 101/tcp hostname\niso-tsap"
	" 102/tcp tsap\ncsnet-ns 105/tcp cso-ns\ncsnet-ns 105/udp cso-ns\nrtelnet 107/tcp\nrtelnet 107/udp\npop-2 109/t"
	"cp postoffice\npop-2 109/udp\npop-3 110/tcp\npop-3 110/udp\nsunrpc 111/tcp portmapper\nsunrpc 111/udp portmapp"
	"er\nauth 113/tcp authentication tap ident\nsftp 115/tcp\nuucp-path 117/tcp\nnntp 119/tcp readnews untp\nntp 12"
	"3/tcp\nntp 123/udp\nnetbios-ns 137/tcp\nnetbios-ns 137/udp\nnetbios-dgm 138/tcp\nnetbios-dgm 138/udp\nnetbios-"
	"ssn 139/tcp\nnetbios-ssn 139/udp\nimap2 143/tcp\nimap2 143/udp\nsnmp 161/udp\nsnmp-trap 162/udp snmptrap\ncmip"
	"-man 163/tcp\ncmip-man 163/udp\ncmip-agent 164/tcp\ncmip-agent 164/udp\nxdmcp 177/tcp\nxdmcp 177/udp\nnextstep"
	" 178/tcp NeXTStep NextStep\nnextstep 178/udp NeXTStep NextStep\nbgp 179/tcp\nbgp 179/udp\nprospero 191/tcp\npr"
	"ospero 191/udp\nirc 194/tcp\nirc 194/udp\nsmux 199/tcp\nsmux 199/udp\nat-rtmp 201/tcp\nat-rtmp 201/udp\nat-nbp"
	" 202/tcp\nat-nbp 202/udp\nat-echo 204/tcp\nat-echo 204/udp\nat-zis 206/tcp\nat-zis 206/udp\nqmtp 209/tcp\nqmtp"
	" 209/udp\nz3950 210/tcp wais\nz3950 210/udp wais\nipx 213/tcp\nipx 213/udp\nimap3 220/tcp\nimap3 220/udp\nulis"
	"tserv 372/tcp\nulistserv 372/udp\nhttps 443/tcp\nhttps 443/udp\nsnpp 444/tcp\nsnpp 444/udp\nsaft 487/tcp\nsaft"
	" 487/udp\nnpmp-local 610/tcp dqs313qmaster\nnpmp-local 610/udp dqs313qmaster\nnpmp-gui 611/tcp dqs313execd\nnp"
	"mp-gui 611/udp dqs313execd\nhmmpind 612/tcp dqs313intercell\nhmmp-ind 612/udp dqs313intercell\nexec 512/tcp\nb"
	"iff 512/udp comsat\nlogin 513/tcp\nwho 513/udp whod\nshell 514/tcp cmd\nsyslog 514/udp\nprinter 515/tcp spoole"
	"r\ntalk 517/udp\nntalk 518/udp\nroute 520/udp router routed\ntimed 525/udp timeserver\ntempo 526/tcp newdate\n"
	"courier 530/tcp rpc\nconference 531/tcp chat\nnetnews 532/tcp readnews\nnetwall 533/udp\nuucp 540/tcp uucpd\na"
 	"fpovertcp 548/tcp\nafpovertcp 548/udp\nremotefs 556/tcp rfs_server rfs\nklogin 543/tcp\nkshell 544/tcp krcmd\n"
	"kerberos-adm 749/tcp\nwebster 765/tcp\nwebster 765/udp\nnfsdstatus 1110/tcp\nnfsd-keepalive 1110/udp\ningreslo"
	"ck 1524/tcp\ningreslock 1524/udp\nprospero-np 1525/tcp\nprospero-np 1525/udp\ndatametrics 1645/tcp oldradius\n"
	"datametrics 1645/udp old-radius\nsa-msg-port 1646/tcp old-radacct\nsa-msg-port 1646/udp old-radacct\nradius 18"
	"12/tcp\nradius 1812/udp\nradacct 1813/tcp\nradacct 1813/udp\nnfsd 2049/tcp nfs\nnfsd 2049/udp nfs\ncvspserver "
	"2401/tcp\ncvspserver 2401/udp\nmysql 3306/tcp\nmysql 3306/udp\nrfe 5002/tcp\nrfe 5002/udp\ncfengine 5308/tcp\n"
	"cfengine 5308/udp\nbbs 7000/tcp\nkerberos4 750/udp kerberos-iv kdc\nkerberos4 750/tcp kerberos-iv kdc\nkerbero"
	"s_master 751/udp\nkerberos_master 751/tcp\npasswd_server 752/udp\nkrb_prop 754/tcp\nkrbupdate 760/tcp kreg\nkp"
	"asswd 761/tcp kpwd\nkpop 1109/tcp\nknetd 2053/tcp\nzephyr-srv 2102/udp\nzephyr-clt 2103/udp\nzephyr-hm 2104/ud"
	"p\neklogin 2105/tcp\nsupfilesrv 871/tcp\nsupfiledbg 1127/tcp\nrtmp 1/ddp\nnbp 2/ddp\necho 4/ddp\nzip 6/ddp\npo"
	"ppassd 106/tcp\npoppassd 106/udp\nmailq 174/tcp\nmailq 174/tcp\nomirr 808/tcp omirrd\nomirr 808/udp omirrd\nrm"
	"tcfg 1236/tcp\nxtel 1313/tcp\ncoda_opcons 1355/udp\ncoda_venus 1363/udp\ncoda_auth 1357/udp\ncoda_udpsrv 1359/"
	"udp\ncoda_filesrv 1361/udp\ncodacon 1423/tcp venus.cmu\ncoda_aux1 1431/tcp\ncoda_aux1 1431/udp\ncoda_aux2 1433"
	"/tcp\ncoda_aux2 1433/udp\ncoda_aux3 1435/tcp\ncoda_aux3 1435/udp\ncfinger 2003/tcp\nafbackup 2988/tcp\nafbacku"
	"p 2988/udp\nicp 3130/tcp\nicp 3130/udp\npostgres 5432/tcp\npostgres 5432/udp\nfax 4557/tcp\nhylafax 4559/tcp\n"
	"noclog 5354/tcp\nnoclog 5354/udp\nhostmon 5355/tcp\nhostmon 5355/udp\nircd 6667/tcp\nircd 6667/udp\nwebcache 8"
	"080/tcp\nwebcache 8080/udp\ntproxy 8081/tcp\ntproxy 8081/udp\nmandelspawn 9359/udp mandelbrot\namanda 10080/ud"
	"p\namandaidx 10082/tcp\namidxtape 10083/tcp\nisdnlog 20011/tcp\nisdnlog 20011/udp\nvboxd 20012/tcp\nvboxd 2001"
	"2/udp\nbinkp 24554/tcp\nbinkp 24554/udp\nasp 27374/tcp\nasp 27374/udp\ntfido 60177/tcp\ntfido 60177/udp\nfido "
	"60179/tcp\nfido 60179/udp\n";

entry_dir assets_rootfs={
	.info.mode=0755,
	ASSET_SUBDIRS{
		ASSET_SIMPLE_DIR("dev",0755),
		ASSET_SIMPLE_DIR("sys",0555),
		ASSET_SIMPLE_DIR("proc",0555),
		ASSET_SIMPLE_DIR("run",0755),
		ASSET_SIMPLE_DIR("tmp",01777),
		ASSET_DIR{
			ASSET_SIMPLE_INFO("etc",0755|S_IFDIR),
			ASSET_SUBDIRS{&assets_terminfo,NULL},
			ASSET_SUBFILES{
				ASSET_SIMPLE_FILE("passwd",0644,"root:x:0:0::/:/bin/sh\n"),
				ASSET_SIMPLE_FILE("group",0644,"root:x:0:root\n"),
				ASSET_SIMPLE_FILE("shadow",0600,"root::::::::\n"),
				ASSET_SIMPLE_FILE("gshadow",0600,"root:::root\n"),
				ASSET_SIMPLE_FILE("hosts",0644,"127.0.0.1\tlocalhost\n"),
				ASSET_SIMPLE_FILE("shells",0644,"/bin/sh\n"),
				ASSET_SIMPLE_FILE("resolv.conf",0644,""),
				ASSET_SIMPLE_FILE("profile",0644,_etc_profile),
				ASSET_SIMPLE_FILE("services",0644,_etc_services),
				ASSET_SIMPLE_FILE("protocols",0644,_etc_protocols),
				ASSET_SYMLINK("mtab","../proc/self/mount"),
				NULL
			},
		},
		ASSET_DIR{
			ASSET_SIMPLE_INFO("usr",0755|S_IFDIR),
			ASSET_SUBDIRS{
				ASSET_SIMPLE_DIR("bin",0755),
				ASSET_SIMPLE_DIR("lib",0755),
				ASSET_DIR{
					ASSET_SIMPLE_INFO("share",0755|S_IFDIR),
					ASSET_SUBDIRS{&assets_terminfo,NULL},
					ASSET_SUBFILES{NULL}
				},
				NULL
			},
			ASSET_SUBFILES{
				ASSET_SYMLINK("sbin","bin"),
				ASSET_SYMLINK("lib64","lib"),
				NULL
			}
		},
		ASSET_DIR{
			ASSET_SIMPLE_INFO("var",0755|S_IFDIR),
			ASSET_SUBDIRS{
				ASSET_SIMPLE_DIR("log",0755),
				ASSET_SIMPLE_DIR("lib",0755),
				ASSET_SIMPLE_DIR("cache",0755),
				ASSET_SIMPLE_DIR("tmp",1777),
				NULL
			},
			ASSET_SUBFILES{
				ASSET_SYMLINK("run","../run"),
				ASSET_SYMLINK("lock","../run/lock"),
				NULL
			}
		},
		NULL
	},
	ASSET_SUBFILES{
		ASSET_SYMLINK("bin","usr/bin"),
		ASSET_SYMLINK("sbin","usr/bin"),
		ASSET_SYMLINK("lib","usr/lib"),
		ASSET_SYMLINK("lib64","usr/lib"),
		NULL
	}
};
