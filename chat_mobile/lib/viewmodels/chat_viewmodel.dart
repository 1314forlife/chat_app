import 'package:flutter/material.dart';
import '../services/websocket_service.dart';
import '../models/user.dart';
import '../models/message.dart';

class ChatViewModel extends ChangeNotifier {
  final WebSocketService _wsService;
  List<User> _onlineUsers = [];
  List<Message> _messages = [];
  String? _currentContact;
  bool _isLoading = false;
  String? _error;

  List<User> get onlineUsers => _onlineUsers;
  List<Message> get messages => _messages;
  String? get currentContact => _currentContact;
  bool get isLoading => _isLoading;
  String? get error => _error;

  // 未读消息数
  Map<String, int> _unreadCounts = {};
  Map<String, int> get unreadCounts => _unreadCounts;

  ChatViewModel(this._wsService) {
    _wsService.onMessage(_handleMessage);
    // 连接后自动获取用户列表
    loadUsers();
  }

  void _handleMessage(Map<String, dynamic> data) {
    print('📨 ChatViewModel 收到消息: $data');

    // 处理命令响应 (有 cmd 字段)
    if (data['cmd'] != null) {
      _handleCommandResponse(data);
      return;
    }

    // 处理事件 (有 event 字段)
    if (data['event'] != null) {
      _handleEvent(data);
      return;
    }

    // 处理直接消息 (有 from 和 content)
    if (data['from'] != null && data['content'] != null) {
      _handleNewMessage(data);
    }
  }

  void _handleCommandResponse(Map<String, dynamic> data) {
    final cmd = data['cmd'];
    final status = data['status'];
    final responseData = data['data'];

    if (status == 'error') {
      _error = data['message'] ?? '未知错误';
      notifyListeners();
      return;
    }

    switch (cmd) {
      case 'users':
        if (responseData != null && responseData['users'] != null) {
          final usersList = responseData['users'] as List;
          _onlineUsers = usersList
              .map((u) => User.fromJson(u as Map<String, dynamic>))
              .toList();
          print('✅ 在线用户列表已更新: ${_onlineUsers.length} 人');
          notifyListeners();
        }
        break;

      case 'history':
        if (responseData != null && responseData is List) {
          _messages = responseData
              .map((m) => Message.fromJson(m as Map<String, dynamic>))
              .toList();
          print('✅ 历史消息已加载: ${_messages.length} 条');
          _isLoading = false;
          notifyListeners();
        }
        break;

      case 'send':
        if (responseData != null && responseData['status'] == 'sent') {
          print('✅ 消息发送成功');
        }
        break;
    }
  }

  void _handleEvent(Map<String, dynamic> data) {
    final event = data['event'];

    switch (event) {
      case 'user_online':
        final user = User.fromJson(data['user']);
        if (!_onlineUsers.any((u) => u.username == user.username)) {
          _onlineUsers.add(user);
          print('🟢 用户上线: ${user.nickname}');
          notifyListeners();
        }
        break;

      case 'user_offline':
        final username = data['username'];
        _onlineUsers.removeWhere((u) => u.username == username);
        print('🔴 用户下线: $username');
        notifyListeners();
        break;

      case 'messages_read':
        final from = data['from'];
        _unreadCounts.remove(from);
        notifyListeners();
        break;
    }
  }

  void _handleNewMessage(Map<String, dynamic> data) {
    final msg = Message(
      id: data['id'] ?? 'local-${DateTime.now().millisecondsSinceEpoch}',
      from: data['from'] ?? '',
      to: data['to'] ?? '',
      content: data['content'] ?? '',
      timestamp:
          data['timestamp'] ?? DateTime.now().millisecondsSinceEpoch ~/ 1000,
      isRead: data['isRead'] ?? false,
    );

    // 如果消息发送给当前聊天对象，添加到消息列表
    if (msg.from == _currentContact || msg.to == _currentContact) {
      _messages.add(msg);
      notifyListeners();
      _markMessagesRead(msg.from);
    } else {
      // 增加未读计数
      _unreadCounts[msg.from] = (_unreadCounts[msg.from] ?? 0) + 1;
      notifyListeners();
    }

    print('📩 新消息: ${msg.from} -> ${msg.to}: ${msg.content}');
  }

  void _markMessagesRead(String from) {
    _wsService.send({
      'cmd': 'mark_read',
      'data': {'from': from},
    });
  }

  void loadUsers() {
    if (!_wsService.isConnected) {
      print('⚠️ WebSocket 未连接');
      return;
    }
    _wsService.send({'cmd': 'users', 'data': {}});
  }

  void setCurrentContact(String username) {
    _currentContact = username;
    _unreadCounts.remove(username);
    _loadHistory(username);
    notifyListeners();
  }

  void sendMessage(String content) {
    if (_currentContact == null || content.isEmpty) return;

    // 临时消息（本地显示）
    final tempMsg = Message(
      id: 'local-${DateTime.now().millisecondsSinceEpoch}',
      from: 'me',
      to: _currentContact!,
      content: content,
      timestamp: DateTime.now().millisecondsSinceEpoch ~/ 1000,
      isRead: false,
    );
    _messages.add(tempMsg);
    notifyListeners();

    // 发送到服务器
    _wsService.send({
      'cmd': 'send',
      'data': {'to': _currentContact, 'content': content},
    });
  }

  void _loadHistory(String contact) {
    if (!_wsService.isConnected) {
      print('⚠️ WebSocket 未连接');
      return;
    }
    _isLoading = true;
    notifyListeners();
    _wsService.send({
      'cmd': 'history',
      'data': {'to': contact, 'limit': 50},
    });

    // 超时保护
    Future.delayed(const Duration(seconds: 3), () {
      _isLoading = false;
      notifyListeners();
    });
  }

  void clearError() {
    _error = null;
    notifyListeners();
  }

  @override
  void dispose() {
    super.dispose();
  }
}
