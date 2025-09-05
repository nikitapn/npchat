#/bin/env bash

set -e

while getopts ":w:h:scd" opt; do
  case $opt in
     w)
       APP_DIR=${OPTARG}
       ;;
     h)
       HOSTNAME=${OPTARG}
       ;;
     s)
       DEPLOY_SERVER=1
       ;;
     c)
       DEPLOY_CLIENT=1
       ;;
     d)
       DEPLOY_DATA=1
       ;;
     *)
       echo "Invalid option: -${OPTARG}."
       exit 1
       ;;
  esac
done

[ -z $APP_DIR ] && echo 'app dir is not specified' && exit 1
[ -z $HOSTNAME ] && echo 'hostname is required' && exit 1

shift $(expr $OPTIND - 1)

[ -z $DEPLOY_SERVER ] && [ -z $DEPLOY_CLIENT ] && [ -z $DEPLOY_DATA ] && echo 'at least one of "-s -c -d" must be specified' && exit 1
[ -z $1 ] && echo '"user@server:/path/to/temp" is not set' && exit 1

TEMP_DIR=$(echo $1 | sed -n 's/.*:\(.*\).*/\1/p')
SERVER=$(echo $1 | sed -n 's/\(.*\):.*/\1/p')

[ -z $SERVER ] || [ -z $TEMP_DIR ] && echo 'incorrect input' && exit 1

if [ "$DEPLOY_DATA" ]; then
  scp artifacts/npchat-data.tar.gz $1
  ssh $SERVER "cd $TEMP_DIR && \
    tar -xzf npchat-data.tar.gz -C $APP_DIR \
  "
fi;

if [ "$DEPLOY_CLIENT" ]; then
  scp artifacts/npchat-client.tar.gz $1
  ssh $SERVER "cd $TEMP_DIR && \
    tar -xzf npchat-client.tar.gz -C $APP_DIR \
  "
fi;

if [ "$DEPLOY_SERVER" ]; then
  scp artifacts/npchat-server.tar.gz $1
  ssh $SERVER "cd $TEMP_DIR &&
    tar -xzf npchat-server.tar.gz && \
    cd out && \
    docker build -t cpp-runtime-env -f Dockerfile.runtime . && \
    docker rm -f npchat &> /dev/null &&
    docker run -d --name npchat \
      -v ${APP_DIR}:/app \
      -w /app \
      -p 8443:443 \
      cpp-runtime-env \
      npchat \
        --hostname $HOSTNAME \
        --port 443 \
        --http-dir www \
        --data-dir data \
        --public-cert cert/fullchain.pem \
        --private-key cert/privkey.pem \
        --dh-params cert/ssl-dhparams.pem \
  "
fi;