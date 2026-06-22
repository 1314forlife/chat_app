mod core;
mod domain;
mod infrastructure;
mod application;
mod utils;

use std::sync::Arc;
use application::handlers::ChatHandler;
use domain::services::ChatService;
use infrastructure::broadcast::BroadcastManager;
use infrastructure::repositories::{MemoryUserRepository, MemoryMessageRepository};
use infrastructure::network::WebSocketServer;

#[tokio::main]
async fn main() {
    tracing_subscriber::fmt::init();
    
    println!("🚀 Chat Server 启动中...");
    
    // 1. 创建 Repository
    let user_repo = Arc::new(MemoryUserRepository::new());
    // 修复：传入最大消息数 1000
    let message_repo = Arc::new(MemoryMessageRepository::new(1000));
    
    // 2. 创建 BroadcastManager
    let broadcast_manager = Arc::new(BroadcastManager::new());
    
    // 3. 创建 ChatService
    let chat_service = Arc::new(ChatService::new(
        user_repo,
        message_repo,
        broadcast_manager.clone(),
    ));
    
    // 4. 创建 ChatHandler
    let chat_handler = Arc::new(ChatHandler::new(chat_service));
    
    // 5. 创建 WebSocketServer
    let ws_server = Arc::new(WebSocketServer::new(
        broadcast_manager,
        chat_handler,
    ));
    
    // 6. 启动服务器
    let app = ws_server.create_router();
    let listener = tokio::net::TcpListener::bind("0.0.0.0:3000")
        .await
        .expect("无法绑定端口 3000");
    
    println!("✅ 服务器运行在 ws://0.0.0.0:3000/ws");
    println!("📡 等待客户端连接...");
    
    axum::serve(listener, app).await.expect("服务器启动失败");
}