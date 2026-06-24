use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct User {
    pub id: String,
    pub username: String,
    pub nickname: String,
    pub online: bool,
}

impl User {
    pub fn new(id: String, username: String, nickname: String) -> Self {
        Self {
            id,
            username,
            nickname,
            online: false,
        }
    }
}