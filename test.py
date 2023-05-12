#!/usr/bin/env nix-shell
#!nix-shell -i python3.9 -p "python39.withPackages(ps: with ps; [ websockets ])"

import asyncio
import json
from websockets import connect

config = {
    "regions": [0,1],
}

raw_config = json.dumps(config);

async def hello(uri):
    async with connect(uri) as websocket:
        await websocket.send(raw_config)
        while True:
            data = json.loads(await websocket.recv())
            print(data)

#TODO: change this to new domain
asyncio.run(hello("wss://socket.staging.tlm.solutions"))
#asyncio.run(hello("ws://127.0.0.1:9001"))
