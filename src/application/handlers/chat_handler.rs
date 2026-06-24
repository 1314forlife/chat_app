use std::sync::Arc;
use serde_json::Value;
use crate::core::error::Result;
use crate::domain::services::ChatService;
use crate::application::dto::*;

pub struct ChatHandler {
    chat_service: Arc<ChatService>,
}

impl ChatHandler {
    pub fn new(chat_service: Arc<ChatService>) -> Self {
        Self { chat_service }
    }
    
    pub fn handle_request(&self, user_id: &str, request: ChatRequest) -> Result<ChatResponse> {
        println!("📋 handle_request: user_id={}, cmd={}", user_id, request.cmd);
        match request.cmd.as_str() {
            "login" => self.handle_login(user_id, request.data),
            "send" => self.handle_send(user_id, request.data),
            "history" => self.handle_history(user_id, request.data),
            "users" => self.handle_users(user_id),
            "logout" => self.handle_logout(user_id),
            _ => Ok(ChatResponse::error("未知命令".to_string())),
        }
    }
    
    fn handle_login(&self, user_id: &str, data: Value) -> Result<ChatResponse> {
        println!("🔐 处理登录: user_id={}", user_id);
        let login_data: LoginData = serde_json::from_value(data)?;
        let user = self.chat_service.login(&login_data.username, &login_data.nickname)?;
        Ok(ChatResponse::success(serde_json::to_value(user)?))
    }
    
    fn handle_send(&self, user_id: &str, data: Value) -> Result<ChatResponse> {
        println!("📤 处理发送: from={}", user_id);
        let send_data: SendData = serde_json::from_value(data)?;
        self.chat_service.send_message(user_id, &send_data.to, &send_data.content)?;
        Ok(ChatResponse::success(serde_json::json!({
            "status": "sent",
            "to": send_data.to,
        })))
    }
    
    fn handle_history(&self, user_id: &str, data: Value) -> Result<ChatResponse> {
        let history_data: HistoryData = serde_json::from_value(data)?;
        let limit = history_data.limit.unwrap_or(50);
        let messages = self.chat_service.get_history(user_id, &history_data.to, limit)?;
        Ok(ChatResponse::success(serde_json::to_value(messages)?))
    }
    
    fn handle_users(&self, _user_id: &str) -> Result<ChatResponse> {
        let users = self.chat_service.get_online_users()?;
        Ok(ChatResponse::success(serde_json::to_value(users)?))
    }
    
    fn handle_logout(&self, user_id: &str) -> Result<ChatResponse> {
        self.chat_service.logout(user_id)?;
        Ok(ChatResponse::success(serde_json::json!({
            "status": "logged out"
        })))
    }
}