FROM golang:1.21-alpine AS builder
WORKDIR /build
COPY go.mod go.sum ./
COPY pkg/ ./pkg/
COPY cmd/container/ ./cmd/container/
RUN CGO_ENABLED=0 go build -o /container ./cmd/container

FROM alpine:latest
COPY --from=builder /container /container
CMD ["/container"]
