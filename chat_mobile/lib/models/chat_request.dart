class ChatRequest {
  final String cmd;
  final Map<String, dynamic> data;

  ChatRequest({required this.cmd, this.data = const {}});

  Map<String, dynamic> toJson() {
    return {'cmd': cmd, 'data': data};
  }
}

class ChatResponse {
  final String status;
  final Map<String, dynamic>? data;
  final String? message;

  ChatResponse({required this.status, this.data, this.message});

  factory ChatResponse.fromJson(Map<String, dynamic> json) {
    return ChatResponse(
      status: json['status'] ?? 'error',
      data: json['data'],
      message: json['message'],
    );
  }

  bool get isSuccess => status == 'success';
}
