FROM alpine AS builder
RUN apk add --allow-untrusted --update --no-cache curl ca-certificates
WORKDIR /
RUN curl -fsSL github.com/lmangani/dhtd/releases/latest/download/dhtd-static -O && chmod +x dhtd-static

FROM alpine
COPY --from=builder /etc/ssl/certs/ca-certificates.crt /etc/ssl/certs/
COPY --from=builder /dhtd-static /dhtd
CMD ["/dhtd", "--peer", "bttracker.debian.org:6881", "--peer", "router.bittorrent.com:6881"]
