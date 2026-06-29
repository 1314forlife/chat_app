// lib/services/auth_service.dart
import 'dart:async';
import 'package:flutter/material.dart';
import '../models/user.dart';
import 'websocket_service.dart';

class AuthService extends ChangeNotifier {
  final WebSocketService _wsService;
  User? _currentUser;
  bool _isLoading = false;
  String? _error;
  Completer<bool>? _loginCompleter;

  User? get currentUser => _currentUser;
  bool get isLoading => _isLoading;
  String? get error => _error;
  bool get isLoggedIn => _currentUser != null;

  AuthService(this._wsService) {
    _wsService.addListener(_handleMessage);
  }

  void _handleMessage(Map<String, dynamic> data) {
    print('📩 AuthService 收到消息: $data');

    // ✅ 判断是否是登录/注册响应
    // 响应格式: {"data": {...}, "status": "success"}
    if (data['status'] != null && data['data'] != null) {
      final innerData = data['data'];
      if (innerData is Map<String, dynamic> && innerData['id'] != null) {
        if (data['status'] == 'success') {
          _currentUser = User.fromJson(innerData);
          _loginCompleter?.complete(true);
          print('✅ 登录/注册成功: ${_currentUser?.username}');
        } else {
          _error = data['message'] ?? '操作失败';
          _loginCompleter?.complete(false);
          print('❌ 登录/注册失败: $_error');
        }
        return;
      }
    }

    // 其他消息（如 user_online, user_offline）不处理，留给 ChatViewModel
    print('📨 AuthService 忽略其他消息');
  }

  Future<bool> login(String username, String password) async {
    _isLoading = true;
    _error = null;
    notifyListeners();

    try {
      if (!_wsService.isConnected) {
        _wsService.connect();
        await Future.delayed(const Duration(milliseconds: 500));
      }

      _loginCompleter = Completer<bool>();

      _wsService.send({
        'cmd': 'login',
        'data': {'username': username},
      });

      final result = await _loginCompleter!.future.timeout(
        const Duration(seconds: 10),
        onTimeout: () {
          _error = '登录超时';
          return false;
        },
      );

      _isLoading = false;
      notifyListeners();
      return result;
    } catch (e) {
      _error = e.toString();
      _isLoading = false;
      notifyListeners();
      return false;
    } finally {
      _loginCompleter = null;
    }
  }

  Future<bool> register(
    String username,
    String password,
    String nickname,
  ) async {
    _isLoading = true;
    _error = null;
    notifyListeners();

    try {
      if (!_wsService.isConnected) {
        _wsService.connect();
        await Future.delayed(const Duration(milliseconds: 500));
      }

      _loginCompleter = Completer<bool>();

      _wsService.send({
        'cmd': 'register',
        'data': {'username': username, 'nickname': nickname},
      });

      final result = await _loginCompleter!.future.timeout(
        const Duration(seconds: 10),
        onTimeout: () {
          _error = '注册超时';
          return false;
        },
      );

      _isLoading = false;
      notifyListeners();
      return result;
    } catch (e) {
      _error = e.toString();
      _isLoading = false;
      notifyListeners();
      return false;
    } finally {
      _loginCompleter = null;
    }
  }

  void logout() {
    if (_currentUser != null) {
      _wsService.send({
        'cmd': 'logout',
        'data': {'username': _currentUser!.username},
      });
    }
    _currentUser = null;
    _wsService.disconnect();
    notifyListeners();
  }
}
