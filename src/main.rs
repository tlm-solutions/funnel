extern crate serde_json;

mod connection;

pub use telegrams::{
    R09GrpcTelegram, 
    R09WebSocketTelegram,
    ReceivesTelegramsServer,
    ReturnCode,
    ReceivesTelegrams
};

pub use connection::{connection_loop, ConnectionPool, ProtectedState, Socket };
pub use stop_names::{InterRegional, TransmissionPosition, TelegramType};

use std::env;
use tonic::{transport::Server, Request, Response, Status};


#[derive(Clone)]
pub struct TelegramProcessor {
    pub connections: ConnectionPool,
    pub stops_lookup: InterRegional
}

impl TelegramProcessor {
    fn new(list: ConnectionPool) -> TelegramProcessor {
        let default_stops = String::from("../stops.json");
        let stops_file = env::var("STOPS_FILE").unwrap_or(default_stops);

        println!("Reading File: {}", &stops_file);
        let res = InterRegional::from(stops_file.as_str()).expect("cannot parse stops.json");

        TelegramProcessor {
            connections: list,
            stops_lookup: res,
        }
    }

    fn stop_meta_data(&self, junction: u32, region: u32) -> TransmissionPosition {
        match self.stops_lookup.get_approximate_position(&region, &junction) {
            Some(found_stop) => {
                return found_stop.clone();
            }
            _ => {}
        }
        TransmissionPosition {
            dhid: None,
            name: None,
            telegram_type: TelegramType::DoorClosed,
            direction: 0,
            lat: 0f64,
            lon: 0f64,
        }
    }
}

#[tonic::async_trait]
impl ReceivesTelegrams for TelegramProcessor {
    async fn receive_r09(&self, request: Request<R09GrpcTelegram>,
    ) -> Result<Response<ReturnCode>, Status> {
        let extracted = request.into_inner().clone();
        let stop_meta_information =
            self.stop_meta_data(extracted.junction, extracted.region as u32);

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
