class AppConfig {
  static const String serverHost = '192.168.0.103';
  static const int serverPort = 3000;

  static String get wsUrl => 'ws://$serverHost:$serverPort/ws';

  // STUN/TURN 服务器（用于 WebRTC）
  static const List<Map<String, String>> iceServers = [
    {'urls': 'stun:stun.l.google.com:19302'},
    {'urls': 'stun:stun1.l.google.com:19302'},
  ];

  static const int historyLimit = 50;

  // 重连配置
  static const int maxReconnectAttempts = 5;
  static const int reconnectIntervalSeconds = 3;
}
