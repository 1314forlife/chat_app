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
        clients.insert(id.clone(), ClientConnection {
            id,
            user_id: None,
            sender,
        });
    }
    
    pub fn bind_user(&self, conn_id: &str, user_id: &str) -> Result<()> {
        let mut clients = self.clients.write().unwrap();
        if let Some(conn) = clients.get_mut(conn_id) {
            conn.user_id = Some(user_id.to_string());
            Ok(())
        } else {
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
        for conn in clients.values() {
            if conn.user_id.is_some() {
                let _ = conn.sender.send(message.to_string());
            }
        }
        Ok(())
    }
    
    pub fn remove(&self, conn_id: &str) -> Result<()> {
        let mut clients = self.clients.write().unwrap();
        clients.remove(conn_id);
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