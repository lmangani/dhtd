FROM alpine as builder
RUN apk add --allow-untrusted --update --no-cache curl ca-certificates
WORKDIR /
RUN curl -fsSL github.com/lmangani/dhtd/releases/latest/download/dhtd-cosmo -O && chmod +x dhtd-cosmo

FROM scratch
COPY --from=builder /etc/ssl/certs/ca-certificates.crt /etc/ssl/certs/
COPY --from=builder /dhtd-cosmo /dhtd
CMD ["/dhtd", "--peer", "bttracker.debian.org:6881", "--peer", "router.bittorrent.com:6881"]
