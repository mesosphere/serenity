FROM	ubuntu:14.04

ENV GRAFANA_VERSION 2.1.0-pre1.linux-x64
ENV INFLUXDB_VERSION 0.8.8

# Prevent some error messages
ENV DEBIAN_FRONTEND noninteractive

ENV TERM=xterm

RUN		apt-get -y update && apt-get -y upgrade

# ---------------- #
#   Installation   #
# ---------------- #

# Install all prerequisites
RUN 	apt-get -y install wget nginx-light supervisor curl


# Install Grafana to /src/grafana
RUN		mkdir -p src/grafana && cd src/grafana && \
			wget http://grafanarel.s3.amazonaws.com/builds/grafana-${GRAFANA_VERSION}.tar.gz -O grafana.tar.gz && \
			tar xzf grafana.tar.gz --strip-components=1 && rm grafana.tar.gz

# Install InfluxDB
RUN		wget http://s3.amazonaws.com/influxdb/influxdb_${INFLUXDB_VERSION}_amd64.deb && \
			dpkg -i influxdb_${INFLUXDB_VERSION}_amd64.deb && rm influxdb_${INFLUXDB_VERSION}_amd64.deb

# ----------------- #
#   Configuration   #
# ----------------- #

# Configure InfluxDB
ADD		influxdb/config.toml /etc/influxdb/config.toml
ADD		influxdb/run.sh /usr/local/bin/run_influxdb
# These two databases have to be created. These variables are used by set_influxdb.sh and set_grafana.sh
ENV		PRE_CREATE_DB data grafana
ENV		INFLUXDB_DATA_USER data
ENV		INFLUXDB_DATA_PW data
ENV		INFLUXDB_GRAFANA_USER grafana
ENV		INFLUXDB_GRAFANA_PW grafana
ENV		ROOT_PW root

# Configure Grafana
ADD		./grafana-influxdb-wiring.sh /grafana-influxdb-wiring.sh

# Configure nginx and supervisord
ADD		./supervisord.conf /etc/supervisor/conf.d/supervisord.conf

# ----------- #
#   Cleanup   #
# ----------- #

#RUN		apt-get autoremove -y wget && \
#			apt-get -y clean && \
#			rm -rf /var/lib/apt/lists/*

# ---------------- #
#   Expose Ports   #
# ---------------- #

# Grafana
EXPOSE	80

# Grafana
EXPOSE	3000

# InfluxDB Admin server
EXPOSE	8083

# InfluxDB HTTP API
EXPOSE	8086

# InfluxDB HTTPS API
EXPOSE	8084

# -------- #
#   Run!   #
# -------- #

CMD		["/usr/bin/supervisord"]