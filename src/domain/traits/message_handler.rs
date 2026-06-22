use crate::core::models::message::Message;
use crate::core::error::Result;

/// 消息处理器 trait
/// 定义处理消息的行为
pub trait MessageHandler: Send + Sync {
    /// 处理接收到的消息
    fn handle_message(&self, msg: &Message) -> Result<()>;
    
    /// 广播消息给所有客户端
    fn broadcast(&self, msg: &Message) -> Result<()>;
}