// This example explores how to properly close a connection.
//
use telegrams::{R09WebSocketTelegram, R09GrpcTelegram};

use tokio::net::TcpStream;
use {
    serde::{Deserialize, Serialize},
    std::{
        env,
        sync::{Arc, Mutex},
    },
    tokio::net::TcpListener,
    bus::{Bus, BusReader},
    tokio::io::AsyncWriteExt
};

#[derive(Serialize, Deserialize, Debug)]
struct Filter {
    #[serde(default)]
    regions: Vec<u32>,
    #[serde(default)]
    junctions: Vec<u32>,
    #[serde(default)]
    lines: Vec<u32>,
}

impl Filter {
    pub fn fits(&self, telegram: &R09GrpcTelegram) -> bool {
        (self.regions.is_empty() || self.regions.contains(&(telegram.region as u32)))
            && (self.junctions.is_empty() || self.junctions.contains(&telegram.junction))
            && (self.lines.is_empty() || (telegram.line.is_some() && self.lines.contains(&telegram.line.unwrap())))
    }
}

pub async fn connection_loop(bus: Arc<Mutex<Bus<R09GrpcTelegram>>>) {
    let default_websock_port = String::from("127.0.0.1:9001");
    let websocket_port = env::var("DEFAULT_WEBSOCKET_HOST").unwrap_or(default_websock_port);

    let server = TcpListener::bind(websocket_port).await.unwrap();

    while let Ok((tcp, addr)) = server.accept().await {
        println!("New Socket Connection {}!", addr);
        let new_receiver = bus.lock().unwrap().add_rx();
        tokio::spawn(accept_connection(tcp, new_receiver));
    }
}


async fn accept_connection(mut stream: TcpStream, mut receiver: BusReader<R09GrpcTelegram>) {
    loop {
        let data = receiver.recv().unwrap();
        let json_to_string = serde_json::to_string(&data).unwrap();

        match stream.write(json_to_string.as_bytes()).await {
            Err(e) => {
                println!("Error {:?}", e);
                break;
            }
            Ok(_) => {}

        }
    }

}
