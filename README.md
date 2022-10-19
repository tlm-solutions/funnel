# Funnel

![built with nix](https://builtwithnix.org/badge.svg)

Service which filters and sends out data to all listening clients over websockets. This service expects the data to be sent over grpc.

## Configuration

### Environment

- **GRPC_PORT**: mendatory under which the grpc server runs
- **WEBSOCKET_PORT**: mendatory under which port the websocket port runs


### Filter 


```json
{
    "lines": [9, 11],
    "positions": [],
    "regions": [0]
}
```

This would only let telegrams from line 9 and 11 from region 0 (dresden). This Json 
just needs to be send to the websocket sending it again will overwrite the previous filter.

