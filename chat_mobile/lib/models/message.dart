class Message {
  final String id;
  final String from;
  final String to;
  final String content;
  final int timestamp;
  final bool isRead;

  Message({
    required this.id,
    required this.from,
    required this.to,
    required this.content,
    required this.timestamp,
    this.isRead = false,
  });

  factory Message.fromJson(Map<String, dynamic> json) {
    return Message(
      id: json['id'] ?? '',
      from: json['from'] ?? '',
      to: json['to'] ?? '',
      content: json['content'] ?? '',
      timestamp: json['timestamp'] ?? 0,
      isRead: json['isRead'] ?? false,
    );
  }

  Map<String, dynamic> toJson() {
    return {
      'id': id,
      'from': from,
      'to': to,
      'content': content,
      'timestamp': timestamp,
      'isRead': isRead,
    };
  }
}
