# oray-ddns
花生壳简单的linux下c语言实现，默认调整为本地ip，账号下所有域名  
用法：./oray 用户名 密码  
如果是systemd，可以使用# make install自动设置为service，修改配置文件/etc/oray.conf，通过# systemctl start oray启动  
或者使用启动脚本$ ./oray.sh