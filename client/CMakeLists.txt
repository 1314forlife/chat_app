cmake_minimum_required(VERSION 3.16)

project(chat_client VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Multimedia MultimediaWidgets)

set(THIRD_PARTY_DIR ${CMAKE_SOURCE_DIR}/third_party)

# ============================================================
# 📦 源文件与头文件
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
    src/webrtc/WebRTCTest.cpp
)

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
    src/webrtc/WebRTCTest.h
)

set(RESOURCES
    resources/resources.qrc
)

# ============================================================
# 🚀 先创建目标
# ============================================================
qt_add_executable(chat_client ${SOURCES} ${HEADERS}
    src/webrtc/FFmpegTest.h
    src/webrtc/FFmpegTest.cpp
    src/webrtc/VideoCapture.h
    src/webrtc/VideoCapture.cpp
    src/webrtc/VideoEncoder.h
    src/webrtc/VideoEncoder.cpp
    src/webrtc/VideoDecoder.h
    src/webrtc/VideoDecoder.cpp
    src/infrastructure/media/WebRTCManager.h
    src/infrastructure/media/WebRTCManager.cpp
    src/ui/VideoCallDialog.h
    src/ui/VideoCallDialog.cpp
    src/infrastructure/media/IpcCapture.h)

qt_add_resources(chat_client "resources"
    resources/resources.qrc
)

# ============================================================
# 🔍 然后设置包含路径（在 qt_add_executable 之后）
# ============================================================
target_include_directories(chat_client PRIVATE
    src
    ${THIRD_PARTY_DIR}/boost_1_82_0
    ${THIRD_PARTY_DIR}/websocketpp
    ${THIRD_PARTY_DIR}/libdatachannel/include
    ${THIRD_PARTY_DIR}/ffmpeg/include
    ${THIRD_PARTY_DIR}/opencv/include
)

# ============================================================
# 🛠️ 设置库路径（在 qt_add_executable 之后）
# ============================================================
set(DATACHANNEL_LIB_DIR ${THIRD_PARTY_DIR}/libdatachannel/lib)
set(FFMPEG_LIB_DIR ${THIRD_PARTY_DIR}/ffmpeg/lib)
set(OPENCV_LIB_DIR ${THIRD_PARTY_DIR}/opencv/lib)

target_link_libraries(chat_client PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Multimedia

    # WebRTC 动态库
    ${DATACHANNEL_LIB_DIR}/libdatachannel.dll.a
    ${DATACHANNEL_LIB_DIR}/libjuice.a
    ${DATACHANNEL_LIB_DIR}/libsrtp2.a

    # FFmpeg 库
    ${FFMPEG_LIB_DIR}/libavcodec.dll.a
    ${FFMPEG_LIB_DIR}/libavformat.dll.a
    ${FFMPEG_LIB_DIR}/libavutil.dll.a
    ${FFMPEG_LIB_DIR}/libswscale.dll.a
    ${FFMPEG_LIB_DIR}/libswresample.dll.a

    #Opencv 库
    ${OPENCV_LIB_DIR}/libopencv_core4100.dll.a
    ${OPENCV_LIB_DIR}/libopencv_videoio4100.dll.a
    ${OPENCV_LIB_DIR}/libopencv_imgproc4100.dll.a


    # Windows 系统库
    ws2_32
    iphlpapi
    crypt32
)