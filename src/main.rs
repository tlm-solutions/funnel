extern crate serde_json;

mod connection;

pub use telegrams::{
    R09GrpcTelegram, 
    R09WebSocketTelegram,
    ReceivesTelegramsServer,
    ReturnCode,
    ReceivesTelegrams
};

pub use stop_names::{
    InterRegional, 
    TransmissionPosition, 
    TelegramType
};

use connection::connection_loop;

use std::env;
use tonic::{transport::Server, Request, Response, Status};
use bus::Bus;
use std::sync::{Mutex, Arc};


#[derive(Clone)]
pub struct TelegramProcessor {
    pub stops_lookup: InterRegional,
    sender: Arc<Mutex<Bus<R09GrpcTelegram>>>
}

impl TelegramProcessor {
    fn new(bus: Arc<Mutex<Bus<R09GrpcTelegram>>>) -> TelegramProcessor {
        let default_stops = String::from("../stops.json");
        let stops_file = env::var("STOPS_FILE").unwrap_or(default_stops);

        println!("Reading File: {}", &stops_file);
        let res = InterRegional::from(stops_file.as_str())
            .expect("cannot parse stops.json");

        TelegramProcessor {
            stops_lookup: res,
            sender: bus
        }
    }
}

#[tonic::async_trait]
impl ReceivesTelegrams for TelegramProcessor {
    async fn receive_r09(&self, request: Request<R09GrpcTelegram>,
    ) -> Result<Response<ReturnCode>, Status> {
        let extracted = request.into_inner().clone();

        self.sender.lock().unwrap().broadcast(extracted);

        let reply = ReturnCode { status: 0 };
        Ok(Response::new(reply))
    }
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let default_grpc_port = String::from("127.0.0.1:50051");
    let grpc_port = env::var("GRPC_HOST").unwrap_or(default_grpc_port);
    let addr = grpc_port.parse()?;
    let global_bus = Arc::new(Mutex::new(Bus::new(20)));
    
    let arc_clone = Arc::clone(&global_bus);
    tokio::spawn(async move {
        connection_loop(arc_clone).await;
    });

    let telegram_processor = TelegramProcessor::new(global_bus);
    Server::builder()
        .add_service(ReceivesTelegramsServer::new(telegram_processor))
        .serve(addr)
        .await?;

    Ok(())
}
