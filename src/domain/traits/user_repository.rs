use crate::core::models::user::User;
use crate::core::error::Result;

/// 用户存储 trait
/// 定义用户数据的存储和查询行为
pub trait UserRepository: Send + Sync {
    /// 保存用户
    fn save(&self, user: &User) -> Result<()>;
    
    /// 根据 ID 查找用户
    fn find_by_id(&self, id: &str) -> Result<Option<User>>;
    
    /// 根据用户名查找用户
    fn find_by_username(&self, username: &str) -> Result<Option<User>>;
    
    /// 获取所有在线用户
    fn get_online_users(&self) -> Result<Vec<User>>;
    
    /// 更新用户在线状态
    fn set_online(&self, id: &str, online: bool) -> Result<()>;
}