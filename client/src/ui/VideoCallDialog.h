#pragma once
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPoint>

class VideoCallDialog : public QDialog {
    Q_OBJECT
public:
    explicit VideoCallDialog(const QString &peerName, bool isCaller, QWidget *parent = nullptr);
    ~VideoCallDialog() = default;

    // 暴露两个 Label 供 WebRTCManager 刷新画面
    QLabel* localVideoLabel() const { return m_localVideo; }
    QLabel* remoteVideoLabel() const { return m_remoteVideo; }

signals:
    void hangUpClicked(); // 挂断/拒绝信号
    void acceptClicked(); // ✨ 新增：接听信号

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QLabel *m_remoteVideo; // 远端大画面
    QLabel *m_localVideo;  // 本地小预览

    // ✨ 按钮组合重构
    QPushButton *m_hangUpBtn; // 挂断/拒绝
    QPushButton *m_acceptBtn; // ✨ 新增：接听按钮

    bool m_dragging = false;
    QPoint m_dragStartPos;
    bool m_isCaller;          // ✨ 保存身份状态
};