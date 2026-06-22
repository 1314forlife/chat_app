use std::collections::HashMap;
use std::sync::RwLock;
use crate::core::models::user::User;
use crate::core::error::{Result, ChatError};
use crate::domain::traits::UserRepository;

/// 内存版用户存储（用于开发和测试）
pub struct MemoryUserRepository {
    users: RwLock<HashMap<String, User>>,
}

impl MemoryUserRepository {
    pub fn new() -> Self {
        Self {
            users: RwLock::new(HashMap::new()),
        }
    }
}

impl UserRepository for MemoryUserRepository {
    fn save(&self, user: &User) -> Result<()> {
        let mut users = self.users.write().map_err(|_| ChatError::Internal("锁错误".to_string()))?;
        users.insert(user.id.clone(), user.clone());
        Ok(())
    }
    
    fn find_by_id(&self, id: &str) -> Result<Option<User>> {
        let users = self.users.read().map_err(|_| ChatError::Internal("锁错误".to_string()))?;
        Ok(users.get(id).cloned())
    }
    
    fn find_by_username(&self, username: &str) -> Result<Option<User>> {
        let users = self.users.read().map_err(|_| ChatError::Internal("锁错误".to_string()))?;
        for user in users.values() {
            if user.username == username {
                return Ok(Some(user.clone()));
            }
        }
        Ok(None)
    }
    
    fn get_online_users(&self) -> Result<Vec<User>> {
        let users = self.users.read().map_err(|_| ChatError::Internal("锁错误".to_string()))?;
        let mut online = Vec::new();
        for user in users.values() {
            if user.online {
                online.push(user.clone());
            }
        }
        Ok(online)
    }
    
    fn set_online(&self, id: &str, online: bool) -> Result<()> {
        let mut users = self.users.write().map_err(|_| ChatError::Internal("锁错误".to_string()))?;
        if let Some(user) = users.get_mut(id) {
            user.online = online;
        }
        Ok(())
    }
}

impl Default for MemoryUserRepository {
    fn default() -> Self {
        Self::new()
    }
}