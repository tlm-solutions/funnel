// This example explores how to properly close a connection.
//
use telegrams::{R09WebSocketTelegram, R09GrpcTelegram};

use std::net::TcpStream;
use std::net::TcpListener;
use std::io::Write;
//use tokio::net::TcpStream;
use {
    serde::{Deserialize, Serialize},
    std::{
        env,
        sync::{Arc, Mutex},
    },
    //tokio::net::TcpListener,
    tokio::sync::broadcast,
    //tokio::io::AsyncWriteExt,
    tungstenite::accept
};
use tungstenite::WebSocket;
use tungstenite::Message;

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

    let server = TcpListener::bind(websocket_port).unwrap();

    for stream in server.incoming() {
        let mut websocket = accept(stream.unwrap()).unwrap();
        println!("New socket connection on address!");
        let mut new_receiver: broadcast::Receiver<R09GrpcTelegram>;
        let receiver_count: usize;

        {
            let mut tx = wrapped_tx.lock().unwrap();
            receiver_count = tx.receiver_count();
            new_receiver = tx.subscribe();
        }

        println!("Current receiver count is {}.", receiver_count);
        tokio::spawn(accept_connection(websocket, new_receiver));
    }
}


async fn accept_connection(mut stream: WebSocket<TcpStream>, mut receiver: broadcast::Receiver<R09GrpcTelegram>) {
    println!("Entering Loop");
    loop {
        println!("Getting Data");
        let data = receiver.recv().await.unwrap();
        let json_to_string = serde_json::to_string(&data).unwrap();

        //let msg = websocket.read_message().unwrap();

        // We do not want to send back ping/pong messages.
        //if msg.is_binary() || msg.is_text() {
        //    websocket.write_message(msg).unwrap();
        //}
        
        println!("writing message !");
        match stream.write_message(Message::Text(json_to_string)) {
            Err(e) => {
                println!("Error {:?}", e);
                break;
            }
            Ok(data) => {
                println!("Data {:?}", data);
            }

        }
    }
    println!("Error Terminating ... ");

}
