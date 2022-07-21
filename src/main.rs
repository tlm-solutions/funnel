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
use std::sync::{Mutex, Arc};
use tokio::sync::broadcast;


#[derive(Clone)]
pub struct TelegramProcessor {
    sender: Arc<Mutex<broadcast::Sender<R09GrpcTelegram>>>
}

impl TelegramProcessor {
    fn new(tx: Arc<Mutex<broadcast::Sender<R09GrpcTelegram>>>) -> TelegramProcessor {
        TelegramProcessor {
            sender: tx
        }
    }
}

#[tonic::async_trait]
impl ReceivesTelegrams for TelegramProcessor {
    async fn receive_r09(&self, request: Request<R09GrpcTelegram>,
    ) -> Result<Response<ReturnCode>, Status> {
        let extracted = request.into_inner().clone();

        println!("Received Telegram from data-accumulator: {:?}", &extracted);
        self.sender.lock().unwrap().send(extracted);

        println!("Response");
        let reply = ReturnCode { status: 0 };
        Ok(Response::new(reply))
    }
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let default_grpc_port = String::from("127.0.0.1:50051");
    let grpc_port = env::var("GRPC_HOST").unwrap_or(default_grpc_port);
    let addr = grpc_port.parse()?;
    let (tx, mut rx) = broadcast::channel(20);
    let global_tx = Arc::new(Mutex::new(tx));

    tokio::spawn(async move {
        loop {
            println!("Received {:?} on broadcast channel", rx.recv().await.unwrap());
        }
    });
    
    let arc_clone = Arc::clone(&global_tx);
    tokio::spawn(async move {
        connection_loop(arc_clone).await;
    });

    let telegram_processor = TelegramProcessor::new(global_tx);

    println!("starting grpc server at addr: {:?}", &addr);
    Server::builder()
        .add_service(ReceivesTelegramsServer::new(telegram_processor))
        .serve(addr)
        .await?;

    Ok(())
}
