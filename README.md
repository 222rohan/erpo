# erpo
erpo is a centralized anti-cheat detection system for Linux-based exam environments. 
---

## **features**
it consists of a stealth client daemon called 'eidolon' that monitors system and network activity (that would normally be considered as malpractice in an exam, ie: ssh, ftp, usb, etc.) and a server that receives logs. eidolon does not interfere with regular system activities.

### **server**
- saves logs sent by eidolons
- cli

### **eidolon**
logs activity on a local file, also sends the same to the server. 

## **limitations**
- does not work on wsl
- not fully tamper free

## **future**
- use of LKMs for kernel level monitoring
- tamper free eidolon using a watchdog daemon 'cerberus'


## **acknowledgement**
thanks to open-source tools like `libpcap`, `auditd`, and `inotify`.