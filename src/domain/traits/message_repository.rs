use crate::core::models::message::Message;
use crate::core::error::Result;

/// 消息存储 trait
pub trait MessageRepository: Send + Sync {
    /// 保存消息
    fn save(&self, msg: &Message) -> Result<()>;
    
    /// 获取两个用户之间的历史消息
    fn get_history(&self, user1: &str, user2: &str, limit: usize) -> Result<Vec<Message>>;
}