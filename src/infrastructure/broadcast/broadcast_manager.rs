use std::collections::HashMap;
use std::sync::RwLock;
use tokio::sync::mpsc;

use crate::core::error::{Result, ChatError};
use crate::core::models::message::Message;
use crate::domain::traits::MessageHandler;

pub struct ClientConnection {
    pub id: String,
    pub user_id: Option<String>,
    pub sender: mpsc::UnboundedSender<String>,
}

pub struct BroadcastManager {
    clients: RwLock<HashMap<String, ClientConnection>>,
}

impl BroadcastManager {
    pub fn new() -> Self {
        Self {
            clients: RwLock::new(HashMap::new()),
        }
    }
    
    pub fn register(&self, id: String, sender: mpsc::UnboundedSender<String>) {
        let mut clients = self.clients.write().unwrap();
        println!("📝 注册连接: conn_id={}", id);
        clients.insert(id.clone(), ClientConnection {
            id,
            user_id: None,
            sender,
        });
        println!("📊 当前连接数: {}", clients.len());
    }
    
    pub fn bind_user(&self, conn_id: &str, user_id: &str) -> Result<()> {
        let mut clients = self.clients.write().unwrap();
        if let Some(conn) = clients.get_mut(conn_id) {
            conn.user_id = Some(user_id.to_string());
            println!("🔗 绑定用户: conn_id={}, user_id={}", conn_id, user_id);
            Ok(())
        } else {
            println!("❌ 绑定失败: conn_id={} 不存在", conn_id);
            Err(ChatError::Internal("连接不存在".to_string()))
        }
    }
    
    pub fn send_to_user(&self, user_id: &str, message: &str) -> Result<()> {
        let clients = self.clients.read().unwrap();
        for conn in clients.values() {
            if let Some(ref uid) = conn.user_id {
                if uid == user_id {
                    let _ = conn.sender.send(message.to_string());
                }
            }
        }
        Ok(())
    }
    
    pub fn broadcast(&self, message: &str) -> Result<()> {
        let clients = self.clients.read().unwrap();
        println!("📡 广播消息给 {} 个连接", clients.len());
        for conn in clients.values() {
            if let Some(ref user_id) = conn.user_id {
                println!("   📤 发送给用户: {}", user_id);
                if let Err(e) = conn.sender.send(message.to_string()) {
                    println!("   ⚠️ 发送失败: {}", e);
                }
            }
        }
        Ok(())
    }
    
    pub fn remove(&self, conn_id: &str) -> Result<()> {
        let mut clients = self.clients.write().unwrap();
        let removed = clients.remove(conn_id);
        println!("🗑️ 移除连接: conn_id={}, 成功={}, 剩余连接: {}", 
                 conn_id, removed.is_some(), clients.len());
        Ok(())
    }
    
    pub fn get_online_users(&self) -> Vec<String> {
        let clients = self.clients.read().unwrap();
        let mut users = Vec::new();
        for conn in clients.values() {
            if let Some(ref user_id) = conn.user_id {
                users.push(user_id.clone());
            }
        }
        users
    }
}

impl Default for BroadcastManager {
    fn default() -> Self {
        Self::new()
    }
}

impl MessageHandler for BroadcastManager {
    fn handle_message(&self, msg: &Message) -> Result<()> {
        let payload = serde_json::to_string(msg)?;
        self.broadcast(&payload)?;
        Ok(())
    }
    
    fn broadcast(&self, msg: &Message) -> Result<()> {
        let payload = serde_json::to_string(msg)?;
        self.broadcast(&payload)?;
        Ok(())
    }
}