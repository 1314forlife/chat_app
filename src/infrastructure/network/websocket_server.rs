use std::sync::Arc;
use std::collections::HashMap;
use tokio::sync::Mutex;
use axum::{
    extract::ws::{Message, WebSocket, WebSocketUpgrade},
    response::IntoResponse,
    routing::get,
    Router,
};
use tokio::sync::mpsc;
use uuid::Uuid;

use crate::core::error::Result;
use crate::application::handlers::ChatHandler;
use crate::application::dto::ChatRequest;
use crate::infrastructure::broadcast::BroadcastManager;

pub struct WebSocketServer {
    broadcast_manager: Arc<BroadcastManager>,
    chat_handler: Arc<ChatHandler>,
    connections: Arc<Mutex<HashMap<String, String>>>,
}

impl WebSocketServer {
    pub fn new(broadcast_manager: Arc<BroadcastManager>, chat_handler: Arc<ChatHandler>) -> Self {
        Self {
            broadcast_manager,
            chat_handler,
            connections: Arc::new(Mutex::new(HashMap::new())),
        }
    }
    
    pub fn create_router(self: Arc<Self>) -> Router {
        Router::new()
            .route("/ws", get(ws_handler))
            .with_state(self)
    }
    
    async fn handle_connection(&self, mut socket: WebSocket, conn_id: String) -> Result<()> {
        println!("🔌 新连接: conn_id={}", conn_id);
        let (tx, mut rx) = mpsc::unbounded_channel();
        
        self.broadcast_manager.register(conn_id.clone(), tx);
        
        loop {
            tokio::select! {
                Some(msg) = socket.recv() => {
                    if let Ok(Message::Text(text)) = msg {
                        println!("📩 收到消息: {}", text);
                        let response = self.parse_and_handle(&conn_id, &text).await;
                        match response {
                            Ok(resp) => {
                                if let Ok(json) = serde_json::to_string(&resp) {
                                    println!("📤 发送响应: {}", json);
                                    let _ = socket.send(Message::Text(json)).await;
                                }
                            }
                            Err(e) => {
                                let err_msg = serde_json::json!({
                                    "status": "error",
                                    "message": e.to_string()
                                });
                                let err_str = err_msg.to_string();
                                println!("❌ 错误响应: {}", err_str);
                                let _ = socket.send(Message::Text(err_str)).await;
                            }
                        }
                    }
                }
                Some(msg) = rx.recv() => {
                    println!("📡 广播消息: {}", msg);
                    if socket.send(Message::Text(msg)).await.is_err() {
                        println!("⚠️ 广播发送失败，连接可能已断开");
                        break;
                    }
                }
                else => {
                    println!("⏰ 无事件，退出循环");
                    break;
                }
            }
        }
        
        // ✅ 连接断开时自动登出
        let username = {
            let mut conns = self.connections.lock().await;
            conns.remove(&conn_id)
        };
        
        println!("🔌 连接断开: conn_id={}, username={:?}", conn_id, username);
        
        if let Some(username) = username {
            println!("🚪 用户 {} 断开连接，自动登出", username);
            let logout_request = ChatRequest {
                cmd: "logout".to_string(),
                data: serde_json::json!({ "username": username }),
            };
            let _ = self.chat_handler.handle_request(&username, logout_request).await;
        }
        
        // ✅ 确保从 broadcast_manager 移除
        let _ = self.broadcast_manager.remove(&conn_id);
        
        Ok(())
    }
    
    async fn parse_and_handle(&self, conn_id: &str, text: &str) -> Result<serde_json::Value> {
        println!("🔍 解析请求: {}", text);
        let request: ChatRequest = serde_json::from_str(text)?;
        println!("🔍 请求命令: {}", request.cmd);
        
        // 如果是 login 请求，先清理该用户的旧连接
        if request.cmd == "login" {
            if let Ok(login_data) = serde_json::from_value::<crate::application::dto::LoginData>(request.data.clone()) {
                let response = self.chat_handler.handle_request(&login_data.username, request).await?;
                
                if let Err(e) = self.broadcast_manager.bind_user(conn_id, &login_data.username) {
                    println!("❌ 绑定用户失败: {}", e);
                } else {
                    println!("✅ 绑定用户: {} -> {}", conn_id, login_data.username);
                }
                
                let mut conns = self.connections.lock().await;
                conns.insert(conn_id.to_string(), login_data.username);
                return Ok(serde_json::to_value(&response)?);
            }
        }
        
        // 非登录请求
        let user_id = {
            let conns = self.connections.lock().await;
            conns.get(conn_id).cloned()
        };
        let user_id = user_id.unwrap_or_else(|| "default_user".to_string());
        println!("🔍 当前用户: {}", user_id);
        
        let response = self.chat_handler.handle_request(&user_id, request).await?;
        println!("🔍 响应: {:?}", response);
        Ok(serde_json::to_value(&response)?)
    }
}

async fn ws_handler(
    ws: WebSocketUpgrade,
    axum::extract::State(state): axum::extract::State<Arc<WebSocketServer>>,
) -> impl IntoResponse {
    ws.on_upgrade(move |socket| {
        let conn_id = Uuid::new_v4().to_string();
        let server = state.clone();
        async move {
            if let Err(e) = server.handle_connection(socket, conn_id).await {
                eprintln!("连接处理错误: {}", e);
            }
        }
    })
}