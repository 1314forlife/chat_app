// lib/services/websocket_service.dart
import 'dart:convert';
import 'package:web_socket_channel/web_socket_channel.dart';
import '../config/app_config.dart';

typedef OnMessageCallback = void Function(Map<String, dynamic> data);

class WebSocketService {
  // ✨ 【核心修改】Dart 标准单例模式
  static final WebSocketService _instance = WebSocketService._internal();

  // 工厂构造函数，让外部调用 WebSocketService() 时永远返回同一个静态实例
  factory WebSocketService() => _instance;

  // 私有的内部构造函数
  WebSocketService._internal();

  WebSocketChannel? _channel;
  final List<OnMessageCallback> _listeners = [];

  bool get isConnected => _channel != null;

  void connect() {
    // 现在的单例状态下，这行代码能完美防止重复连接！
    if (_channel != null) {
      print('ℹ️ WebSocket 已经连接，跳过重复创建');
      return;
    }

    try {
      print('🌐 正在连接到 WebSocket: ${AppConfig.wsUrl}');
      _channel = WebSocketChannel.connect(Uri.parse(AppConfig.wsUrl));

      _channel!.stream.listen(
        (message) {
          final data = jsonDecode(message);
          // 通知所有监听器
          for (var listener in _listeners) {
            listener(data);
          }
        },
        onError: (error) {
          print('❌ WebSocket 错误: $error');
          _channel = null;
        },
        onDone: () {
          print('🔌 WebSocket 连接在其底层断开 (onDone)');
          _channel = null;
        },
      );
      print('✅ WebSocket 连接成功');
    } catch (e) {
      print('❌ WebSocket 连接失败: $e');
      _channel = null;
    }
  }

  void disconnect() {
    print('🔌 主动断开 WebSocket 连接');
    _channel?.sink.close();
    _channel = null;
  }

  void send(Map<String, dynamic> data) {
    if (_channel == null) {
      print('❌ WebSocket 未连接，无法发送');
      return;
    }
    final json = jsonEncode(data);
    _channel!.sink.add(json);
    print('📤 发送: $json');
  }

  void addListener(OnMessageCallback callback) {
    if (!_listeners.contains(callback)) {
      _listeners.add(callback);
      print('✅ 添加 WebSocket 监听器，当前总数: ${_listeners.length}');
    }
  }

  void removeListener(OnMessageCallback callback) {
    _listeners.remove(callback);
    print('🗑️ 移除 WebSocket 监听器，当前总数: ${_listeners.length}');
  }

  void onMessage(OnMessageCallback callback) {
    addListener(callback);
  }
}
