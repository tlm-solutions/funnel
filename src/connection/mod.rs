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
    tokio::sync::broadcast,
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

pub async fn connection_loop(wrapped_tx: Arc<Mutex<broadcast::Sender<R09GrpcTelegram>>>) {
    let default_websock_port = String::from("127.0.0.1:9001");
    let websocket_port = env::var("DEFAULT_WEBSOCKET_HOST").unwrap_or(default_websock_port);

    let server = TcpListener::bind(websocket_port).await.unwrap();

    while let Ok((tcp, addr)) = server.accept().await {
        println!("New socket connection on address {}!", addr);
        let mut new_receiver: broadcast::Receiver<R09GrpcTelegram>;
        let receiver_count: usize;

        {
            let mut tx = wrapped_tx.lock().unwrap();
            receiver_count = tx.receiver_count();
            new_receiver = tx.subscribe();
        }

        println!("Current receiver count is {}.", receiver_count);
        tokio::spawn(accept_connection(tcp, new_receiver));
    }
}


async fn accept_connection(mut stream: TcpStream, mut receiver: broadcast::Receiver<R09GrpcTelegram>) {
    loop {
        let data = receiver.recv().await.unwrap();
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
