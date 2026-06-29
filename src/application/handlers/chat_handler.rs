use std::sync::Arc;
use serde_json::Value;
use crate::core::error::Result;
use crate::core::models::message::Message;
use crate::domain::services::ChatService;
use crate::application::dto::*;
use crate::domain::traits::MessageHandler;

pub struct ChatHandler {
    chat_service: Arc<ChatService>,
    message_handler: Arc<dyn MessageHandler>,
}

impl ChatHandler {
    pub fn new(chat_service: Arc<ChatService>, message_handler: Arc<dyn MessageHandler>) -> Self {
        Self {
            chat_service,
            message_handler,
        }
    }
    
    pub async fn handle_request(&self, user_id: &str, request: ChatRequest) -> Result<ChatResponse> {
        match request.cmd.as_str() {
            "register" => self.handle_register(request.data).await,
            "login" => self.handle_login(request.data, user_id).await,
            "send" => self.handle_send(user_id, request.data).await,
            "history" => self.handle_history(user_id, request.data).await,
            "users" => self.handle_users().await,
            "logout" => self.handle_logout(request.data).await,
            _ => Ok(ChatResponse::error("未知命令".to_string())),
        }
    }
    
    async fn handle_register(&self, data: Value) -> Result<ChatResponse> {
        let register_data: LoginData = serde_json::from_value(data)?;
        let username = register_data.username;
        let nickname = register_data.nickname.unwrap_or_else(|| username.clone());
        
        let user = self.chat_service.register(&username, &nickname).await?;
        
        Ok(ChatResponse::success(serde_json::to_value(user)?))
    }
    
    async fn handle_login(&self, data: Value, conn_id: &str) -> Result<ChatResponse> {
        let login_data: LoginData = serde_json::from_value(data)?;
        let username = login_data.username;
        
        let user = self.chat_service.login(&username, conn_id).await?;
        
        // ✅ 修复：正确构造 user_online 事件
        let event_json = serde_json::json!({
            "event": "user_online",
            "user": {
                "id": user.id,
                "username": user.username,
                "nickname": user.nickname,
                "online": user.online
            }
        });
        let event_str = serde_json::to_string(&event_json).unwrap();
        let event_msg = Message::new(
            "system".to_string(),
            "all".to_string(),
            event_str,
        );
        let _ = self.message_handler.broadcast(&event_msg);
        
        Ok(ChatResponse::success(serde_json::to_value(user)?))
    }
    
    async fn handle_send(&self, user_id: &str, data: Value) -> Result<ChatResponse> {
        let send_data: SendData = serde_json::from_value(data)?;
        self.chat_service.send_message(user_id, &send_data.to, &send_data.content).await?;
        Ok(ChatResponse::success(serde_json::json!({
            "status": "sent",
            "to": send_data.to,
        })))
    }
    
    async fn handle_history(&self, user_id: &str, data: Value) -> Result<ChatResponse> {
        let history_data: HistoryData = serde_json::from_value(data)?;
        let limit = history_data.limit.unwrap_or(50);
        let messages = self.chat_service.get_history(user_id, &history_data.to, limit).await?;
        Ok(ChatResponse::success(serde_json::to_value(messages)?))
    }
    
    async fn handle_users(&self) -> Result<ChatResponse> {
        let users = self.chat_service.get_online_users().await?;
        let data = serde_json::json!({
            "users": users
        });
        Ok(ChatResponse::success(data))
    }
    
    async fn handle_logout(&self, data: Value) -> Result<ChatResponse> {
        let logout_data: LogoutData = serde_json::from_value(data)?;
        let username = logout_data.username;
        
        println!("🚪 登出请求: username={}", username);
        
        let user = self.chat_service.get_user_by_username(&username).await;
        if let Some(user) = user {
            self.chat_service.logout(&username).await?;
            println!("✅ 用户 {} 已登出", username);
            
            let event_json = serde_json::json!({
                "event": "user_offline",
                "user_id": user.id,
                "username": user.username
            });
            let event_str = serde_json::to_string(&event_json).unwrap();
            let event_msg = Message::new(
                "system".to_string(),
                "all".to_string(),
                event_str,
            );
            let _ = self.message_handler.broadcast(&event_msg);
        } else {
            println!("❌ 用户 {} 不存在", username);
        }
        
        Ok(ChatResponse::success(serde_json::json!({
            "status": "logged out"
        })))
    }
}