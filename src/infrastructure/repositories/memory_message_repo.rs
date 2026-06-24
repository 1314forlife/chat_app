use std::collections::VecDeque;
use std::sync::RwLock;
use crate::core::models::message::Message;
use crate::core::error::{Result, ChatError};
use crate::domain::traits::MessageRepository;

/// 内存版消息存储（用于开发和测试）
pub struct MemoryMessageRepository {
    messages: RwLock<VecDeque<Message>>,
    max_size: usize,
}

impl MemoryMessageRepository {
    pub fn new(max_size: usize) -> Self {
        Self {
            messages: RwLock::new(VecDeque::with_capacity(max_size)),
            max_size,
        }
    }
}

impl MessageRepository for MemoryMessageRepository {
    fn save(&self, msg: &Message) -> Result<()> {
        let mut messages = self.messages.write().map_err(|_| ChatError::Internal("锁错误".to_string()))?;
        
        if messages.len() >= self.max_size {
            messages.pop_front();
        }
        messages.push_back(msg.clone());
        Ok(())
    }
    
    fn get_history(&self, user1: &str, user2: &str, limit: usize) -> Result<Vec<Message>> {
        let messages = self.messages.read().map_err(|_| ChatError::Internal("锁错误".to_string()))?;
        
        let mut result = Vec::new();
        for msg in messages.iter().rev() {
            if result.len() >= limit {
                break;
            }
            if (msg.from == user1 && msg.to == user2) || (msg.from == user2 && msg.to == user1) {
                result.push(msg.clone());
            }
        }
        result.reverse();
        Ok(result)
    }
}

impl Default for MemoryMessageRepository {
    fn default() -> Self {
        Self::new(1000)
    }
}