use thiserror::Error;

#[derive(Error, Debug)]
pub enum ChatError {
    #[error("网络错误: {0}")]
    Network(String),
    
    #[error("消息解析错误: {0}")]
    ParseError(String),
    
    #[error("用户不存在: {0}")]
    UserNotFound(String),
    
    #[error("认证失败: {0}")]
    AuthFailed(String),
    
    #[error("内部错误: {0}")]
    Internal(String),
}

pub type Result<T> = std::result::Result<T, ChatError>;

// 添加 From 转换，使 ? 能正常工作
impl From<serde_json::Error> for ChatError {
    fn from(err: serde_json::Error) -> Self {
        ChatError::ParseError(err.to_string())
    }
}

impl From<std::io::Error> for ChatError {
    fn from(err: std::io::Error) -> Self {
        ChatError::Network(err.to_string())
    }
}