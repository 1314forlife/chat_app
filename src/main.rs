mod core;
mod domain;
mod infrastructure;
mod application;
mod utils;

use std::sync::Arc;
use application::handlers::ChatHandler;
use domain::services::ChatService;
use infrastructure::broadcast::BroadcastManager;
use infrastructure::repositories::{SqliteUserRepository, MemoryMessageRepository};
use infrastructure::network::WebSocketServer;
use infrastructure::redis_client;
use infrastructure::db;
use domain::traits::MessageHandler;

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    tracing_subscriber::fmt::init();
    
    println!("🚀 Chat Server 启动中...");
    
    // 1. 初始化 Redis
    let redis_conn = redis_client::create_redis_client().await?;
    let redis_conn = Arc::new(tokio::sync::Mutex::new(redis_conn));
    println!("✅ Redis 连接成功");
    
    // 2. 初始化 SQLite 数据库
    let db_pool = Arc::new(db::create_db_pool().await?);
    
    // 3. 创建 Repository（使用 SQLite）
    let user_repo = Arc::new(SqliteUserRepository::new(db_pool));
    let message_repo = Arc::new(MemoryMessageRepository::new(1000));
    
    let broadcast_manager = Arc::new(BroadcastManager::new());
    
    let chat_service = Arc::new(ChatService::new(
        user_repo,
        message_repo,
        broadcast_manager.clone() as Arc<dyn MessageHandler>,
        redis_conn.clone(),
    ));
    
    let chat_handler = Arc::new(ChatHandler::new(
        chat_service,
        broadcast_manager.clone() as Arc<dyn MessageHandler>,
    ));
    
    let ws_server = Arc::new(WebSocketServer::new(
        broadcast_manager,
        chat_handler,
    ));
    
    let app = ws_server.create_router();
    let listener = tokio::net::TcpListener::bind("0.0.0.0:3000")
        .await
        .expect("无法绑定端口 3000");
    
    println!("✅ 服务器运行在 ws://0.0.0.0:3000/ws");
    println!("📡 等待客户端连接...");
    
    axum::serve(listener, app).await.expect("服务器启动失败");
    
    Ok(())
}