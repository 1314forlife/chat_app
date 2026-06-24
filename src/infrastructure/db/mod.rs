use sqlx::sqlite::SqlitePool;
use sqlx::SqlitePool as Pool;
use crate::core::models::user::User;

/// 创建 SQLite 数据库连接
pub async fn create_db_pool() -> Result<Pool, sqlx::Error> {
    let pool = SqlitePool::connect("sqlite:chat.db").await?;
    
    sqlx::query(
        r#"
        CREATE TABLE IF NOT EXISTS users (
            id TEXT PRIMARY KEY,
            username TEXT UNIQUE NOT NULL,
            nickname TEXT NOT NULL,
            online INTEGER DEFAULT 0
        )
        "#
    )
    .execute(&pool)
    .await?;
    
    println!("✅ 数据库初始化成功");
    Ok(pool)
}

/// 用户数据库模型
#[derive(Debug, Clone)]
pub struct UserDb {
    pub id: String,
    pub username: String,
    pub nickname: String,
    pub online: i32,
}

impl UserDb {
    pub fn to_user(&self) -> User {
        let mut user = User::new(
            self.id.clone(),
            self.username.clone(),
            self.nickname.clone(),
        );
        user.online = self.online == 1;
        user
    }
}