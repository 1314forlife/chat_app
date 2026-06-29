use std::sync::Arc;
use tokio::sync::Mutex;
use crate::core::models::message::Message;
use crate::core::models::user::User;
use crate::core::error::{Result, ChatError};
use crate::domain::traits::{MessageHandler, UserRepository, MessageRepository};
use crate::infrastructure::redis_client::{self, RedisClient};

pub struct ChatService {
    user_repo: Arc<dyn UserRepository>,
    message_repo: Arc<dyn MessageRepository>,
    message_handler: Arc<dyn MessageHandler>,
    redis_conn: Arc<Mutex<RedisClient>>,
}

impl ChatService {
    pub fn new(
        user_repo: Arc<dyn UserRepository>,
        message_repo: Arc<dyn MessageRepository>,
        message_handler: Arc<dyn MessageHandler>,
        redis_conn: Arc<Mutex<RedisClient>>,
    ) -> Self {
        Self {
            user_repo,
            message_repo,
            message_handler,
            redis_conn,
        }
    }
    
    pub async fn send_message(&self, from: &str, to: &str, content: &str) -> Result<()> {
        println!("📤 发送消息: from={}, to={}, content={}", from, to, content);
        
        let mut conn = self.redis_conn.lock().await;
        let online = redis_client::get_user_online(&mut conn, from).await
            .map_err(|e| ChatError::Internal(e.to_string()))?;
        
        if online.is_none() {
            return Err(ChatError::AuthFailed(format!("用户 {} 不在线", from)));
        }
        
        let _to_user = self.user_repo.find_by_username(to)?
            .ok_or_else(|| ChatError::UserNotFound(to.to_string()))?;
        
        let msg = Message::new(from.to_string(), to.to_string(), content.to_string());
        self.message_repo.save(&msg)?;
        self.message_handler.handle_message(&msg)?;
        Ok(())
    }
    
    pub async fn register(&self, username: &str, nickname: &str) -> Result<User> {
        println!("📝 注册: username={}, nickname={}", username, nickname);
        
        if let Some(_) = self.user_repo.find_by_username(username)? {
            return Err(ChatError::AuthFailed(format!("用户 {} 已存在", username)));
        }
        
        let id = format!("user_{}_{}", username, 
            std::time::SystemTime::now()
                .duration_since(std::time::UNIX_EPOCH)
                .unwrap_or_default()
                .as_nanos());
        let user = User::new(id, username.to_string(), nickname.to_string());
        self.user_repo.save(&user)?;
        self.user_repo.set_online(&user.id, true)?;
        
        Ok(user)
    }
    
    /// ✅ 登录：同时写入 online 和 session
    pub async fn login(&self, username: &str, conn_id: &str) -> Result<User> {
        println!("🔐 登录: username={}, conn_id={}", username, conn_id);
        
        // 检查用户是否存在
        let mut user = self.user_repo.find_by_username(username)?
            .ok_or_else(|| ChatError::AuthFailed(format!("用户 {} 不存在，请先注册", username)))?;
        
        // 检查是否已在别处登录
        let mut conn = self.redis_conn.lock().await;
        let existing = redis_client::get_user_online(&mut conn, username).await
            .map_err(|e| ChatError::Internal(e.to_string()))?;
        
        if existing.is_some() {
            // ✅ 检查 session 是否匹配当前连接
            let old_session = redis_client::get_user_session(&mut conn, username).await
                .map_err(|e| ChatError::Internal(e.to_string()))?;
            
            if let Some(old_conn_id) = old_session {
                if old_conn_id != conn_id {
                    return Err(ChatError::AuthFailed(format!("用户 {} 已在别处登录", username)));
                }
            } else {
                return Err(ChatError::AuthFailed(format!("用户 {} 已在别处登录", username)));
            }
        }
        
        user.online = true;
        self.user_repo.set_online(&user.id, true)?;
        
        // ✅ 写入 online + session
        let user_json = serde_json::to_value(&user)
            .map_err(|e| ChatError::Internal(e.to_string()))?;
        redis_client::set_user_online(&mut conn, username, conn_id, &user_json).await
            .map_err(|e| ChatError::Internal(e.to_string()))?;
        println!("✅ 用户 {} 登录成功，session: {}", username, conn_id);
        
        Ok(user)
    }
    
    /// ✅ 登出：清除 online + session
    pub async fn logout(&self, username: &str) -> Result<()> {
        println!("🔐 登出: username={}", username);
        
        let user = self.user_repo.find_by_username(username)?
            .ok_or_else(|| ChatError::UserNotFound(username.to_string()))?;
        
        let mut conn = self.redis_conn.lock().await;
        redis_client::remove_user_online(&mut conn, username).await
            .map_err(|e| ChatError::Internal(e.to_string()))?;
        println!("🗑️ 已从 Redis 移除用户: {}", username);
        
        self.user_repo.set_online(&user.id, false)?;
        Ok(())
    }
    
    pub async fn get_user_by_id(&self, user_id: &str) -> Option<User> {
        self.user_repo.find_by_id(user_id).unwrap_or(None)
    }
    
    pub async fn get_user_by_username(&self, username: &str) -> Option<User> {
        self.user_repo.find_by_username(username).unwrap_or(None)
    }
    
    pub async fn get_history(&self, user1: &str, user2: &str, limit: usize) -> Result<Vec<Message>> {
        self.message_repo.get_history(user1, user2, limit)
    }
    
    pub async fn get_online_users(&self) -> Result<Vec<User>> {
        let mut conn = self.redis_conn.lock().await;
        let usernames = redis_client::get_all_online_users(&mut conn).await
            .map_err(|e| ChatError::Internal(e.to_string()))?;
        
        let mut users = Vec::new();
        for username in usernames {
            if let Some(user) = self.user_repo.find_by_username(&username)? {
                users.push(user);
            }
        }
        Ok(users)
    }
}