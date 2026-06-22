cmake_minimum_required(VERSION 3.16)

project(chat_client VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

# ============================================================
# 源文件列表
# ============================================================
set(SOURCES
    src/main.cpp
    src/common/Logger.cpp
    src/domain/entities/User.cpp
    src/domain/entities/Message.cpp
    src/domain/usecases/LoginUseCase.cpp
    src/domain/usecases/SendMessageUseCase.cpp
    src/domain/usecases/GetContactsUseCase.cpp
    src/application/controllers/ChatController.cpp
    src/application/controllers/LoginController.cpp
    src/infrastructure/network/WebSocketClient.cpp
    src/infrastructure/network/ConnectionManager.cpp
    src/infrastructure/repositories/MemoryUserRepository.cpp
    src/infrastructure/repositories/MemoryMessageRepository.cpp
    src/infrastructure/serialization/JsonSerializer.cpp
    src/ui/MainWindow.cpp
    src/ui/LoginDialog.cpp
    src/ui/ChatWidget.cpp
    src/ui/ContactList.cpp
    src/ui/MessageBubble.cpp
)

# ============================================================
# 头文件列表
# ============================================================
set(HEADERS
    src/common/Constants.h
    src/common/Logger.h
    src/common/Observer.h
    src/common/Singleton.h
    src/domain/entities/User.h
    src/domain/entities/Message.h
    src/domain/repositories/IUserRepository.h
    src/domain/repositories/IMessageRepository.h
    src/domain/services/IAuthService.h
    src/domain/services/IChatService.h
    src/domain/usecases/LoginUseCase.h
    src/domain/usecases/SendMessageUseCase.h
    src/domain/usecases/GetContactsUseCase.h
    src/application/controllers/ChatController.h
    src/application/controllers/LoginController.h
    src/application/dto/LoginRequest.h
    src/application/dto/LoginResponse.h
    src/application/dto/SendMessageRequest.h
    src/application/dto/UserDTO.h
    src/application/interfaces/IMessageService.h
    src/application/interfaces/IUserService.h
    src/infrastructure/network/WebSocketClient.h
    src/infrastructure/network/ConnectionManager.h
    src/infrastructure/repositories/MemoryUserRepository.h
    src/infrastructure/repositories/MemoryMessageRepository.h
    src/infrastructure/serialization/IMessageSerializer.h
    src/infrastructure/serialization/JsonSerializer.h
    src/ui/MainWindow.h
    src/ui/LoginDialog.h
    src/ui/ChatWidget.h
    src/ui/ContactList.h
    src/ui/MessageBubble.h
)

# ============================================================
# 资源文件
# ============================================================
set(RESOURCES
    resources/style.qss
)

# ============================================================
# 可执行文件
# ============================================================
qt_add_executable(chat_client ${SOURCES} ${HEADERS} ${RESOURCES})

# ============================================================
# 链接库
# ============================================================
target_link_libraries(chat_client PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
)

# ============================================================
# 包含目录
# ============================================================
target_include_directories(chat_client PRIVATE src)