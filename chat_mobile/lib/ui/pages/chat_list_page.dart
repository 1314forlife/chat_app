import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../../viewmodels/chat_viewmodel.dart';
import '../../viewmodels/auth_viewmodel.dart';
import 'chat_detail_page.dart';

class ChatListPage extends StatefulWidget {
  const ChatListPage({super.key});

  @override
  State<ChatListPage> createState() => _ChatListPageState();
}

class _ChatListPageState extends State<ChatListPage> {
  @override
  void initState() {
    super.initState();
    // 进入页面时加载好友列表
    WidgetsBinding.instance.addPostFrameCallback((_) {
      final chatVM = Provider.of<ChatViewModel>(context, listen: false);
      chatVM.loadUsers();
    });
  }

  @override
  Widget build(BuildContext context) {
    final authVM = Provider.of<AuthViewModel>(context);
    final chatVM = Provider.of<ChatViewModel>(context);
    final users = chatVM.onlineUsers;
    final unreadCounts = chatVM.unreadCounts;

    return Scaffold(
      appBar: AppBar(
        title: const Text('消息'),
        backgroundColor: Colors.transparent,
        elevation: 0,
        actions: [
          IconButton(
            icon: const Icon(Icons.refresh),
            onPressed: () => chatVM.loadUsers(),
          ),
          IconButton(
            icon: const Icon(Icons.logout),
            onPressed: () => authVM.logout(),
          ),
        ],
      ),
      body: Container(
        decoration: const BoxDecoration(
          gradient: LinearGradient(
            begin: Alignment.topLeft,
            end: Alignment.bottomRight,
            colors: [Color(0xFF0f0c29), Color(0xFF302b63), Color(0xFF24243e)],
          ),
        ),
        child: users.isEmpty
            ? _buildEmptyState()
            : _buildUserList(users, unreadCounts, chatVM),
      ),
    );
  }

  Widget _buildEmptyState() {
    return Center(
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Icon(
            Icons.people_outline,
            size: 80,
            color: Colors.white.withOpacity(0.2),
          ),
          const SizedBox(height: 16),
          Text(
            '暂无在线好友',
            style: TextStyle(
              color: Colors.white.withOpacity(0.4),
              fontSize: 16,
            ),
          ),
          const SizedBox(height: 8),
          Text(
            '点击刷新按钮获取在线用户',
            style: TextStyle(
              color: Colors.white.withOpacity(0.2),
              fontSize: 12,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildUserList(
    List users,
    Map<String, int> unreadCounts,
    ChatViewModel chatVM,
  ) {
    return ListView.builder(
      padding: const EdgeInsets.symmetric(vertical: 8),
      itemCount: users.length,
      itemBuilder: (context, index) {
        final user = users[index];
        final unread = unreadCounts[user.username] ?? 0;

        return ListTile(
          leading: CircleAvatar(
            backgroundColor: Colors.purple.withOpacity(0.3),
            child: Text(
              user.nickname.isNotEmpty ? user.nickname[0].toUpperCase() : '?',
              style: const TextStyle(color: Colors.white),
            ),
          ),
          title: Text(
            user.nickname,
            style: const TextStyle(color: Colors.white),
          ),
          subtitle: Text(
            '🟢 在线',
            style: TextStyle(
              color: Colors.green.withOpacity(0.7),
              fontSize: 12,
            ),
          ),
          trailing: unread > 0
              ? Container(
                  padding: const EdgeInsets.all(6),
                  decoration: const BoxDecoration(
                    color: Colors.red,
                    shape: BoxShape.circle,
                  ),
                  child: Text(
                    '$unread',
                    style: const TextStyle(
                      color: Colors.white,
                      fontSize: 12,
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                )
              : null,
          onTap: () {
            // 跳转到聊天详情页
            Navigator.push(
              context,
              MaterialPageRoute(
                builder: (context) => ChatDetailPage(contact: user),
              ),
            );
          },
        );
      },
    );
  }
}
