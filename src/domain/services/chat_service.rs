use std::sync::Arc;
use std::time::{SystemTime, UNIX_EPOCH};
use crate::core::models::message::Message;
use crate::core::models::user::User;
use crate::core::error::{Result, ChatError};
use crate::domain::traits::{MessageHandler, UserRepository, MessageRepository};

pub struct ChatService {
    user_repo: Arc<dyn UserRepository>,
    message_repo: Arc<dyn MessageRepository>,
    message_handler: Arc<dyn MessageHandler>,
}

impl ChatService {
    pub fn new(
        user_repo: Arc<dyn UserRepository>,
        message_repo: Arc<dyn MessageRepository>,
        message_handler: Arc<dyn MessageHandler>,
    ) -> Self {
        Self {
            user_repo,
            message_repo,
            message_handler,
        }
    }
    
    pub fn send_message(&self, from: &str, to: &str, content: &str) -> Result<()> {
        println!("📤 发送消息: from={}, to={}, content={}", from, to, content);
        
        let user = self.user_repo.find_by_username(from)?
            .ok_or_else(|| ChatError::UserNotFound(from.to_string()))?;
        println!("✅ 发送者: {:?}", user);
        
        if !user.online {
            return Err(ChatError::AuthFailed(format!("用户 {} 不在线", from)));
        }
        
        let to_user = self.user_repo.find_by_username(to)?
            .ok_or_else(|| ChatError::UserNotFound(to.to_string()))?;
        println!("✅ 接收者: {:?}", to_user);
        
        let msg = Message::new(from.to_string(), to.to_string(), content.to_string());
        println!("✅ 消息创建: {:?}", msg);
        
        self.message_repo.save(&msg)?;
        println!("✅ 消息已保存");
        
        self.message_handler.handle_message(&msg)?;
        println!("✅ 消息已广播");
        
        Ok(())
    }
    
    pub fn login(&self, username: &str, nickname: &str) -> Result<User> {
        println!("🔐 登录: username={}, nickname={}", username, nickname);
        
        if let Some(mut user) = self.user_repo.find_by_username(username)? {
            user.online = true;
            self.user_repo.set_online(&user.id, true)?;
            println!("✅ 用户已存在: {:?}", user);
            return Ok(user);
        }
        
        // 使用纳秒时间戳 + 随机数生成唯一 ID
        let now = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap_or_default()
            .as_nanos();
        let id = format!("user_{}_{}", username, now);
        
        let user = User::new(id, username.to_string(), nickname.to_string());
        self.user_repo.save(&user)?;
        self.user_repo.set_online(&user.id, true)?;
        println!("✅ 新用户创建: {:?}", user);
        
        Ok(user)
    }
    
    pub fn logout(&self, user_id: &str) -> Result<()> {
        println!("🔐 登出: user_id={}", user_id);
        self.user_repo.set_online(user_id, false)?;
        Ok(())
    }
    
    pub fn get_history(&self, user1: &str, user2: &str, limit: usize) -> Result<Vec<Message>> {
        println!("📜 获取历史: user1={}, user2={}, limit={}", user1, user2, limit);
        self.message_repo.get_history(user1, user2, limit)
    }
    
    pub fn get_online_users(&self) -> Result<Vec<User>> {
        self.user_repo.get_online_users()
    }
}