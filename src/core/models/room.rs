use serde::{Deserialize, Serialize};
use std::collections::HashSet;

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Room {
    pub id: String,
    pub name: String,
    pub members: HashSet<String>,
}

impl Room {
    pub fn new(id: String, name: String) -> Self {
        Self {
            id,
            name,
            members: HashSet::new(),
        }
    }
    
    pub fn add_member(&mut self, user_id: String) {
        self.members.insert(user_id);
    }
    
    pub fn remove_member(&mut self, user_id: &str) {
        self.members.remove(user_id);
    }
}