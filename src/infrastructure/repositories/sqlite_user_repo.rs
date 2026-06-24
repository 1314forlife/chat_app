use std::sync::Arc;
use sqlx::SqlitePool;
use crate::core::models::user::User;
use crate::core::error::{Result, ChatError};
use crate::domain::traits::UserRepository;
use crate::infrastructure::db::UserDb;

pub struct SqliteUserRepository {
    pool: Arc<SqlitePool>,
}

impl SqliteUserRepository {
    pub fn new(pool: Arc<SqlitePool>) -> Self {
        Self { pool }
    }
}

impl UserRepository for SqliteUserRepository {
    fn save(&self, user: &User) -> Result<()> {
        let pool = self.pool.clone();
        let user_clone = user.clone();
        tokio::task::block_in_place(|| {
            tokio::runtime::Handle::current().block_on(async move {
                sqlx::query(
                    r#"
                    INSERT OR REPLACE INTO users (id, username, nickname, online)
                    VALUES (?, ?, ?, ?)
                    "#
                )
                .bind(&user_clone.id)
                .bind(&user_clone.username)
                .bind(&user_clone.nickname)
                .bind(if user_clone.online { 1 } else { 0 })
                .execute(&*pool)
                .await
                .map_err(|e| ChatError::Internal(e.to_string()))?;
                Ok::<(), ChatError>(())
            })
        })?;
        Ok(())
    }
    
    fn find_by_username(&self, username: &str) -> Result<Option<User>> {
        let pool = self.pool.clone();
        let username = username.to_string();
        let result = tokio::task::block_in_place(|| {
            tokio::runtime::Handle::current().block_on(async move {
                // ✅ 使用 query_as（不带 !）
                let row: Option<(String, String, String, i32)> = sqlx::query_as(
                    r#"
                    SELECT id, username, nickname, online FROM users WHERE username = ?
                    "#
                )
                .bind(&username)
                .fetch_optional(&*pool)
                .await
                .map_err(|e| ChatError::Internal(e.to_string()))?;
                
                Ok::<Option<(String, String, String, i32)>, ChatError>(row)
            })
        })?;
        
        Ok(result.map(|(id, username, nickname, online)| {
            let mut user = User::new(id, username, nickname);
            user.online = online == 1;
            user
        }))
    }
    
    fn find_by_id(&self, id: &str) -> Result<Option<User>> {
        let pool = self.pool.clone();
        let id = id.to_string();
        let result = tokio::task::block_in_place(|| {
            tokio::runtime::Handle::current().block_on(async move {
                let row: Option<(String, String, String, i32)> = sqlx::query_as(
                    r#"
                    SELECT id, username, nickname, online FROM users WHERE id = ?
                    "#
                )
                .bind(&id)
                .fetch_optional(&*pool)
                .await
                .map_err(|e| ChatError::Internal(e.to_string()))?;
                
                Ok::<Option<(String, String, String, i32)>, ChatError>(row)
            })
        })?;
        
        Ok(result.map(|(id, username, nickname, online)| {
            let mut user = User::new(id, username, nickname);
            user.online = online == 1;
            user
        }))
    }
    
    fn get_online_users(&self) -> Result<Vec<User>> {
        let pool = self.pool.clone();
        let result = tokio::task::block_in_place(|| {
            tokio::runtime::Handle::current().block_on(async move {
                let rows: Vec<(String, String, String, i32)> = sqlx::query_as(
                    r#"
                    SELECT id, username, nickname, online FROM users WHERE online = 1
                    "#
                )
                .fetch_all(&*pool)
                .await
                .map_err(|e| ChatError::Internal(e.to_string()))?;
                
                Ok::<Vec<(String, String, String, i32)>, ChatError>(rows)
            })
        })?;
        
        Ok(result.into_iter().map(|(id, username, nickname, online)| {
            let mut user = User::new(id, username, nickname);
            user.online = online == 1;
            user
        }).collect())
    }
    
    fn set_online(&self, id: &str, online: bool) -> Result<()> {
        let pool = self.pool.clone();
        let id = id.to_string();
        tokio::task::block_in_place(|| {
            tokio::runtime::Handle::current().block_on(async move {
                sqlx::query(
                    r#"
                    UPDATE users SET online = ? WHERE id = ?
                    "#
                )
                .bind(if online { 1 } else { 0 })
                .bind(&id)
                .execute(&*pool)
                .await
                .map_err(|e| ChatError::Internal(e.to_string()))?;
                Ok::<(), ChatError>(())
            })
        })?;
        Ok(())
    }
}