use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ChatRequest {
    pub cmd: String,
    pub data: serde_json::Value,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ChatResponse {
    pub status: String,
    pub data: serde_json::Value,
    pub message: Option<String>,
}

impl ChatResponse {
    pub fn success(data: serde_json::Value) -> Self {
        Self {
            status: "success".to_string(),
            data,
            message: None,
        }
    }
    
    pub fn error(msg: String) -> Self {
        Self {
            status: "error".to_string(),
            data: serde_json::json!({}),
            message: Some(msg),
        }
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct LoginData {
    pub username: String,
    pub nickname: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct SendData {
    pub to: String,
    pub content: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct HistoryData {
    pub to: String,
    pub limit: Option<usize>,
}