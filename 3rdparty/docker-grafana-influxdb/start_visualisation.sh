docker run -d -v /etc/localtime:/etc/localtime:ro -p 80:80 -p 8083:8083 -p 8084:8084 -p 8086:8086 --name grafana-influxdb_con grafana_influxdb
