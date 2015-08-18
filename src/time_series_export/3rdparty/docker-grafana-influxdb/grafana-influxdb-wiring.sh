#!/bin/bash -e

# Usage: $ ./grafana-influxdb-wiring.sh name_of_database

# Took from:
# https://gist.github.com/leehambley/9741431695da3787f6b3

# Used for this script to talk to the Grafana API
function grafana_url {
  echo -e http://localhost:3000/api/
}
 
# Used for this script to talk to the InfluxDB API
function influxfb_local_url {
  echo -e "http://localhost:8086/"
}
 
# Used for Grafana to script to talk to the InfluxDB API
function influxfb_remote_url {
  echo -e "http://localhost:8086/"
}
 
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# It is pitch black. You are likely to be eaten by a grue.
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
 
COOKIEJAR=$(mktemp)
trap 'unlink ${COOKIEJAR}' EXIT
 
function usage {
  echo "Usage: ${0} database_name"
  exit 1
}
 
function influx_has_database {
  curl --silent "$(influxfb_local_url)db?u=root&p=root" | grep --silent "$1"
}
 
function influx_create_database {
  curl --silent --data-binary "{\"name\":\"${1}\"}" "$(influxfb_local_url)db?u=root&p=root"
  curl --silent --data-binary "{\"name\":\"${1}\",\"password\":\"${1}\"}" "$(influxfb_local_url)db/${1}/users?u=root&p=root"
}
 
function setup_influxdb {
  if influx_has_database "$1"; then
    info "InfluxDB: Database ${1} already exists"
  else
    if influx_create_database "$1"; then
      success "InfluxDB: Database ${1} created"
    else
      error "InfluxDB: Database ${1} could not be created"
    fi
  fi
}
 
function setup_grafana_session {
  if ! curl -H 'Content-Type: application/json;charset=UTF-8' \
    --data-binary '{"user":"admin","email":"","password":"admin"}' \
    --cookie-jar "$COOKIEJAR" \
    'http://localhost:3000/login' > /dev/null 2>&1 ; then
    echo
    error "Grafana Session: Couldn't store cookies at ${COOKIEJAR}"
  fi
}
 
function grafana_has_data_source {
  setup_grafana_session
  curl --silent --cookie "$COOKIEJAR" "$(grafana_url)datasources" \
    | grep "{\"name\":\"${1}\"}" --silent
}
 
function grafana_create_data_source {
  setup_grafana_session
  curl --cookie "$COOKIEJAR" \
       -X POST \
       --silent \
       -H 'Content-Type: application/json;charset=UTF-8' \
       --data-binary "{\"name\":\"${1}\",\"type\":\"influxdb_08\",\"url\":\"$(influxfb_remote_url)\",\"access\":\"proxy\",\"database\":\"${1}\",\"user\":\"${1}\",\"password\":\"${1}\"}" \
       "$(grafana_url)datasources" 2>&1 | grep 'Datasource added' --silent;
}
 
function setup_grafana {
  if grafana_has_data_source "$1"; then
    info "Grafana: Data source ${1} already exists"
  else
    if grafana_create_data_source "$1"; then
      success "Grafana: Data source ${1} created"
    else
      error "Grafana: Data source ${1} could not be created"
    fi
  fi
}
 
function success {
  echo "$(tput setaf 2)""$*""$(tput sgr0)"
}
 
function info {
  echo "$(tput setaf 3)""$*""$(tput sgr0)"
}
 
function error {
  echo "$(tput setaf 1)""$*""$(tput sgr0)" 1>&2
}
 
function setup {
  setup_influxdb "$1"
  setup_grafana "$1"
}
 
function fill_influxdb_sample_data {
  for _ in $(seq 3600); do
    curl -X POST \
      "$(influxfb_local_url)db/${1}/series?u=${1}&p=${1}" \
      -d "[{\"name\":\"data\",\"columns\":[\"foo\",\"bar\"],\"points\":[[$((RANDOM%200+100)),$((RANDOM%300+50))]]}]";
    sleep 1;
  done
}
 
#if [ "$#" -ne 1 ]; then
#  usage
#else
  setup "serenity" #"$1"
#fi
