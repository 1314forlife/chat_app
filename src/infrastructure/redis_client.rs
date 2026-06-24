use redis::AsyncCommands;
use std::sync::Arc;
use tokio::sync::Mutex;

pub type RedisClient = redis::aio::MultiplexedConnection;

pub async fn create_redis_client() -> Result<RedisClient, redis::RedisError> {
    let client = redis::Client::open("redis://127.0.0.1:6379")?;
    let conn = client.get_multiplexed_tokio_connection().await?;
    Ok(conn)
}

/// 设置用户在线状态 + 记录 session
pub async fn set_user_online(
    conn: &mut RedisClient,
    username: &str,
    conn_id: &str,
    user_data: &serde_json::Value,
) -> Result<(), redis::RedisError> {
    let key = format!("user:{}:online", username);
    let session_key = format!("user:{}:session", username);
    
    let _: () = conn.set_ex(&key, user_data.to_string(), 300).await?;
    let _: () = conn.set_ex(&session_key, conn_id, 300).await?;
    
    Ok(())
}

/// 获取用户在线状态
pub async fn get_user_online(
    conn: &mut RedisClient,
    username: &str,
) -> Result<Option<String>, redis::RedisError> {
    let key = format!("user:{}:online", username);
    let value: Option<String> = conn.get(&key).await?;
    Ok(value)
}

/// 获取用户的 session (conn_id)
pub async fn get_user_session(
    conn: &mut RedisClient,
    username: &str,
) -> Result<Option<String>, redis::RedisError> {
    let key = format!("user:{}:session", username);
    let value: Option<String> = conn.get(&key).await?;
    Ok(value)
}

/// 移除用户在线状态 + session
pub async fn remove_user_online(
    conn: &mut RedisClient,
    username: &str,
) -> Result<(), redis::RedisError> {
    let key = format!("user:{}:online", username);
    let session_key = format!("user:{}:session", username);
    
    let _: () = conn.del(&key).await?;
    let _: () = conn.del(&session_key).await?;
    
    println!("🗑️ Redis 已清除用户: {}", username);
    Ok(())
}

/// 获取所有在线用户
pub async fn get_all_online_users(
    conn: &mut RedisClient,
) -> Result<Vec<String>, redis::RedisError> {
    let keys: Vec<String> = conn.keys("user:*:online").await?;
    let mut users = Vec::new();
    for key in keys {
        if let Some(username) = key.strip_prefix("user:").and_then(|s| s.strip_suffix(":online")) {
            users.push(username.to_string());
        }
    }
    Ok(users)
}