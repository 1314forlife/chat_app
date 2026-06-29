// lib/main.dart
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'services/websocket_service.dart';
import 'services/auth_service.dart';
import 'viewmodels/auth_viewmodel.dart';
import 'viewmodels/chat_viewmodel.dart';
import 'ui/pages/login_page.dart';
import 'ui/pages/chat_list_page.dart';
import 'ui/pages/chat_detail_page.dart';
import 'models/user.dart';

void main() {
  // ✅ 在 main 函数中创建单例
  final wsService = WebSocketService()..connect();
  final authService = AuthService(wsService);

  runApp(MyApp(wsService: wsService, authService: authService));
}

class MyApp extends StatelessWidget {
  final WebSocketService wsService;
  final AuthService authService;

  const MyApp({super.key, required this.wsService, required this.authService});

  @override
  Widget build(BuildContext context) {
    return MultiProvider(
      providers: [
        // ✅ 使用已有的实例，不重新创建
        Provider<WebSocketService>.value(value: wsService),
        ChangeNotifierProvider<AuthService>.value(value: authService),
        ChangeNotifierProvider<AuthViewModel>(
          create: (context) => AuthViewModel(authService),
        ),
        ChangeNotifierProvider<ChatViewModel>(
          create: (context) => ChatViewModel(wsService),
        ),
      ],
      child: MaterialApp(
        title: 'Rust Chat',
        theme: ThemeData.dark().copyWith(
          primaryColor: Colors.purple,
          scaffoldBackgroundColor: const Color(0xFF0f0c29),
          appBarTheme: const AppBarTheme(
            backgroundColor: Color(0xFF1a1a3e),
            elevation: 0,
          ),
        ),
        initialRoute: '/',
        routes: {
          '/': (context) => Consumer<AuthViewModel>(
            builder: (context, authVM, _) {
              if (authVM.isLoggedIn) {
                return const ChatListPage();
              }
              return const LoginPage();
            },
          ),
          '/login': (context) => const LoginPage(),
          '/chat_list': (context) => const ChatListPage(),
        },
        onGenerateRoute: (settings) {
          if (settings.name == '/chat_detail') {
            final user = settings.arguments as User;
            return MaterialPageRoute(
              builder: (context) => ChatDetailPage(contact: user),
            );
          }
          return null;
        },
      ),
    );
  }
}
