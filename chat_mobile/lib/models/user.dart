class User {
  final String id;
  final String username;
  final String nickname;
  final bool online;

  User({
    required this.id,
    required this.username,
    required this.nickname,
    this.online = false,
  });

  factory User.fromJson(Map<String, dynamic> json) {
    return User(
      id: json['id'] ?? '',
      username: json['username'] ?? '',
      nickname: json['nickname'] ?? '',
      online: json['online'] ?? false,
    );
  }

  Map<String, dynamic> toJson() {
    return {
      'id': id,
      'username': username,
      'nickname': nickname,
      'online': online,
    };
  }

  User copyWith({
    String? id,
    String? username,
    String? nickname,
    bool? online,
  }) {
    return User(
      id: id ?? this.id,
      username: username ?? this.username,
      nickname: nickname ?? this.nickname,
      online: online ?? this.online,
    );
  }
}
