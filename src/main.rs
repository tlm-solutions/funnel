extern crate serde_json;

mod connection;
mod telegram;

pub use connection::{connection_loop, ConnectionPool, ProtectedState, Socket };
pub use telegram::{
    ReceivesTelegrams, ReceivesTelegramsServer, ReducedTelegram, ReturnCode, WebSocketTelegram,
};

use std::collections::HashMap;
use std::env;
use std::fs;

use serde::{Deserialize, Serialize};
use tonic::{transport::Server, Request, Response, Status};

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct Stop {
    lat: f64,
    lon: f64,
    name: String,
}

#[derive(Clone)]
pub struct TelegramProcessor {
    pub connections: ConnectionPool,
    pub stops_lookup: HashMap<u32, HashMap<u32, Stop>>,
}

impl TelegramProcessor {
    fn new(list: ConnectionPool) -> TelegramProcessor {
        let default_stops = String::from("../stops.json");
        let stops_file = env::var("STOPS_FILE").unwrap_or(default_stops);

        println!("Reading File: {}", &stops_file);
        let data = fs::read_to_string(stops_file).expect("Unable to read file");
        let res: HashMap<u32, HashMap<u32, Stop>> =
            serde_json::from_str(&data).expect("Unable to parse");
        TelegramProcessor {
            connections: list,
            stops_lookup: res,
        }
    }

    fn stop_meta_data(&self, junction: u32, region: u32) -> Stop {
        match self.stops_lookup.get(&region) {
            Some(regional_stops) => match regional_stops.get(&junction) {
                Some(found_stop) => {
                    return found_stop.clone();
                }
                _ => {}
            },
            _ => {}
        }
        Stop {
            lat: 0f64,
            lon: 0f64,
            name: String::from(""),
        }
    }
}

#[tonic::async_trait]
impl ReceivesTelegrams for TelegramProcessor {
    async fn receive_new(&self, request: Request<ReducedTelegram>,
    ) -> Result<Response<ReturnCode>, Status> {
        let extracted = request.into_inner().clone();
        let stop_meta_information =
            self.stop_meta_data(extracted.position_id, extracted.region_code);

        self.connections
            .write_all(&extracted, &stop_meta_information)
            .await;

        let reply = ReturnCode { status: 0 };
        Ok(Response::new(reply))
    }
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let default_grpc_port = String::from("127.0.0.1:50051");
    let grpc_port = env::var("GRPC_HOST").unwrap_or(default_grpc_port);

    let addr = grpc_port.parse()?;
    let list: ConnectionPool = ConnectionPool::new();
    let list_clone = list.clone();

    tokio::spawn(async move {
        connection_loop(list_clone).await;
    });

    let telegram_processor = TelegramProcessor::new(list.clone());
    Server::builder()
        .add_service(ReceivesTelegramsServer::new(telegram_processor))
        .serve(addr)
        .await?;

    Ok(())
}
