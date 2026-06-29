import 'package:flutter/material.dart';
import '../services/auth_service.dart';

class AuthViewModel extends ChangeNotifier {
  final AuthService _authService;

  AuthViewModel(this._authService);

  bool get isLoading => _authService.isLoading;
  String? get error => _authService.error;
  bool get isLoggedIn => _authService.isLoggedIn;
  String? get currentUsername => _authService.currentUser?.username;

  Future<bool> login(String username, String password) async {
    final result = await _authService.login(username, password);
    notifyListeners(); // ✅ 添加
    return result;
  }

  Future<bool> register(
    String username,
    String password,
    String nickname,
  ) async {
    final result = await _authService.register(username, password, nickname);
    notifyListeners(); // ✅ 添加
    return result;
  }

  void logout() {
    _authService.logout();
    notifyListeners();
  }
}
