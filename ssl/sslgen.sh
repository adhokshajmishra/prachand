#!/bin/sh

if [ "$#" -ne 1 ]
then
  echo "Usage: Must supply a domain"
  exit 1
fi

DOMAIN=$1
IP_OR_DOMAIN=""
if echo "$DOMAIN" | grep -Eq '^([0-9]{1,3}\.){3}[0-9]{1,3}$'; then
  IP_OR_DOMAIN="IP.1=$DOMAIN"
else
  IP_OR_DOMAIN="DNS.1=$DOMAIN"
fi


cd ~/certs

openssl genrsa -out $DOMAIN.key 2048
openssl req -new -key $DOMAIN.key -out $DOMAIN.csr

cat > $DOMAIN.ext << EOF
authorityKeyIdentifier=keyid,issuer
basicConstraints=CA:FALSE
keyUsage = digitalSignature, nonRepudiation, keyEncipherment, dataEncipherment
subjectAltName = @alt_names
[alt_names]
$IP_OR_DOMAIN

EOF

openssl x509 -req -in $DOMAIN.csr -CA ./myCA.pem -CAkey ./myCA.key -CAcreateserial -out $DOMAIN.crt -days 825 -sha256 -extfile $DOMAIN.ext