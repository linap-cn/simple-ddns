# Simple-ddns
## 简介 ##
c语言实现的通用动态域名更新程序。
-------------
## 用法 ##
Usage: ddns [options]  
  -f  --config file     Read config from file.  
  -h  --help            Display this usage information.  
  -u  --user username   User Name to login.  
  -p  --pass password   Password to login.  
  -H  --host hostname   Hostname of the ddns server.  
  -U  --url url         Url without hostname to send get request.  
  -t  --time [s]        Optional. Time between two request. Default:900.  
  -A  --agent [s]       Optional. User-Agent. Default:ddnsv0.1.  

  如果是systemd，可以使用# make install自动设置为service，修改配置文件/etc/ddns.conf，通过# systemctl start ddns启动
-------------
## TODO： ##
1.  配置文件中可以替换[ip][user][pass]等参数。
2.  兼容其他版本系统的install-sh