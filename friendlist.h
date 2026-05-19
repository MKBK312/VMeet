#ifndef FRIENDLIST_H
#define FRIENDLIST_H

#include<QMenu>
#include <QDialog>
#include<QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QCloseEvent>
#include <QPushButton>
#include <QList>
#include<QLabel>
#include<QMap>
#include<QByteArray>
#include"frienditem.h"

namespace Ui {
class friendList;
}

class friendList : public QDialog
{
    Q_OBJECT
signals:
    void sig_offline();
    void sig_addFriend();

    // Meeting actions — handled by CKernel
    void sig_createRoom();
    void sig_joinRoom();
    void sig_enterRoom(int roomId);
    void sig_joinRoomById(int roomId);
    void sig_removeRoomFromList(int roomId);
    void sig_leaveRoom(int roomId);
    void sig_endMeeting(int roomId);
    void sig_roomChatMessage(int roomId, QString content);
    void sig_toggleMute(bool muted);
    void sig_toggleCamera(bool off);
    void sig_toggleScreenShare();

public:
    explicit friendList(QWidget *parent = nullptr);
    ~friendList();

    // User info
    void setUserInfo(QString name, QString feeling, int iconId);

    // Friend list (聊天 tab)
    void addFriend(frienditem* item);

    // Room lobby (会议 tab)
    void addRoomToLobby(int roomId, int roomNumber, QString roomName, int memberCount, int joined);
    void clearRoomList();
    void updateRoomMemberCount(int roomId, int count);
    void refreshRoomItemInLobby(int roomId, int roomNumber, QString roomName, int memberCount, int joined);

    // Room view (embedded)
    void showRoomView(int roomId, int roomNumber, QString roomName, bool isCreator = false);
    void showRoomLobby();
    void displayRoomMessage(QString senderName, QString content);
    void displayVideoFrame(int userId, QByteArray jpegData);
    void clearVideoFrame(int userId);
    void showLocalPIP(int userId, QByteArray jpegData);
    int getCurrentRoomId() const { return m_currentRoomId; }

    // Member panel
    void updateRoomMembers(int roomId, const QStringList& members, const QList<int>& userIds, int totalCount);

    void closeEvent(QCloseEvent *event);
    bool eventFilter(QObject *obj, QEvent *event) override;

    // Audio/video controls
    void setMuted(bool muted);
    void setCameraOff(bool off);

private slots:
    // Left nav
    void on_pb_navMeeting_clicked();
    void on_pb_navChat_clicked();
    void on_pb_navContacts_clicked();
    void on_pb_navSettings_clicked();

    // Menu
    void on_pb_menu_clicked();
    void slot_menuTrigger(QAction *Action);

    // Lobby buttons
    void on_pb_createMeeting_clicked();
    void on_pb_joinMeeting_clicked();

    // Room view buttons
    void on_pb_backToLobby_clicked();
    void on_pb_leaveRoom_clicked();
    void on_pb_endMeeting_clicked();
    void on_pb_roomSend_clicked();

    // Chat tab
    void on_pb_addFriend_clicked();

    // Room item click handler
    void slot_roomItemClicked(int roomId);

    // Member panel + copy
    void on_pb_memberHandle_clicked();
    void on_pb_toggleMembers_clicked();
    void on_pb_copyRoomNumber_clicked();

    // Video grid
    void slot_videoClicked(int userId);
    void slot_closeEnlarged();

    // Right panel
    void slot_toggleChatPanel();

    // Bottom bar
    void slot_toggleMute();
    void slot_toggleCamera();

private:
    void setNavSelected(QPushButton* selected);
    void arrangeVideoGrid();

    Ui::friendList *ui;

    // Friend list in chat tab
    QVBoxLayout* m_playout;

    // Contacts list in contacts tab
    QVBoxLayout* m_pContactsLayout;

    // Room list in meeting lobby
    QVBoxLayout* m_pRoomListLayout;
    QMap<int, QWidget*> m_mapRoomItems;

    // Current room state
    int m_currentRoomId;
    int m_currentRoomNumber;
    QString m_currentRoomName;
    bool m_isRoomCreator;

    // Nav buttons
    QList<QPushButton*> m_navButtons;
    QMenu* m_pMenu;

    // --- Video Grid ---
    QWidget*     m_pVideoGridWidget;
    QGridLayout* m_pVideoGridLayout;
    QMap<int, QLabel*> m_mapVideoLabels;
    QMap<int, QByteArray> m_mapLastFrames;       // all frames
    QMap<int, QByteArray> m_mapLastScreenFrames;  // only screen (>20KB)
    QMap<int, QString> m_mapUserNames;
    QMap<int, bool> m_userHasScreen;

    // Enlarged view
    QLabel* m_pEnlargedVideo;
    int     m_enlargedUserId;
    QWidget* m_pEnlargedOverlay;

    // --- Right Panel ---
    QWidget* m_pRightPanel;
    QPushButton* m_pBtnCollapse;
    bool m_chatCollapsed;

    // --- Bottom Bar ---
    QWidget* m_pBottomBar;
    QPushButton* m_pBtnMute;
    QPushButton* m_pBtnCamera;
    bool m_muted;
    bool m_cameraOff;
};

#endif // FRIENDLIST_H
