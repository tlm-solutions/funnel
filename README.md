# Funnel

![built with nix](https://builtwithnix.org/badge.svg)

Service which filters and sends out data to all listening clients over websockets. This service expects the data to be sent over grpc.

## Configuration

### Environment

- **GRPC_PORT**: mendatory under which the grpc server runs
- **WEBSOCKET_PORT**: mendatory under which port the websocket port runs



