# erpodev_log

## eidolon

### main
1. daemonize
2. client (send,recv bytes)
2. netmon, sysmon threads
3. heartbeat
--- 
#### todo / known issues
1. exiting doesnt work for most cases other than heartbeat
2. config support needed
3. watchdog needed
4. protected log / anti delete / recovery 

### netmon
1. pcap sniffer
2. packet handler 
3. supported types ftp, http, https, ssh 
--- 
#### todo / known issues
1. add other protocol support
2. overlap of messages / threading error?
3. config support needed

### sysmon
1. nothing as of now
--- 
#### todo / known issues
1. revamp needed

### logger
1. log event
2. initializer
3. formatted log event
4. log file and folder
--- 
#### todo / known issues
1. nothing here

## server
1. command support (help, kick, status, stop)
2. buggy client status recorder, need better kicking, status client indexing
--- 
#### todo / known issues
1. config needed

