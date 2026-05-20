#include "friendlist.h"
#include "ui_friendlist.h"
#include <QDebug>
#include <QHBoxLayout>
#include <QLabel>
#include <QTime>
#include <QApplication>
#include <QClipboard>
#include <QTimer>

friendList::friendList(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::friendList),
    m_currentRoomId(0),
    m_currentRoomNumber(0),
    m_isRoomCreator(false),
    m_pVideoGridWidget(nullptr),
    m_pVideoGridLayout(nullptr),
    m_pEnlargedVideo(nullptr),
    m_enlargedUserId(0),
    m_pEnlargedOverlay(nullptr),
    m_pRightPanel(nullptr),
    m_pBtnCollapse(nullptr),
    m_chatCollapsed(false),
    m_pBottomBar(nullptr),
    m_pBtnMute(nullptr),
    m_pBtnCamera(nullptr),
    m_muted(false),
    m_cameraOff(false)
{
    ui->setupUi(this);
    this->resize(1600, 900);

    // Friend list layout (聊天 tab - wdg_list in page_chat)
    m_playout = new QVBoxLayout;
    m_playout->setSpacing(3);
    m_playout->setContentsMargins(0, 0, 0, 0);
    ui->wdg_list->setLayout(m_playout);

    // Contacts list layout (联系人 tab - wdg_contactsList in page_contacts)
    m_pContactsLayout = new QVBoxLayout;
    m_pContactsLayout->setSpacing(3);
    m_pContactsLayout->setContentsMargins(0, 0, 0, 0);
    ui->wdg_contactsList->setLayout(m_pContactsLayout);

    // Room list layout (会议 tab lobby)
    m_pRoomListLayout = new QVBoxLayout;
    m_pRoomListLayout->setSpacing(4);
    m_pRoomListLayout->setContentsMargins(0, 0, 0, 0);
    ui->wdg_roomListInLobby->setLayout(m_pRoomListLayout);

    // Start on meeting lobby
    ui->wdg_meetingStack->setCurrentIndex(0);
    ui->wdg_contentStack->setCurrentIndex(0);

    // Set up left nav buttons
    m_navButtons.clear();
    m_navButtons.append(ui->pb_navMeeting);
    m_navButtons.append(ui->pb_navChat);
    m_navButtons.append(ui->pb_navContacts);

    // Default: select 会议 nav
    setNavSelected(ui->pb_navMeeting);

    // Menu
    m_pMenu = new QMenu(this);
    m_pMenu->addAction("添加好友");
    m_pMenu->addAction("系统设置");
    connect(m_pMenu, &QMenu::triggered,
            this, &friendList::slot_menuTrigger);
}

friendList::~friendList()
{
    if (m_playout) {
        delete m_playout;
        m_playout = nullptr;
    }
    if (m_pContactsLayout) {
        delete m_pContactsLayout;
        m_pContactsLayout = nullptr;
    }
    if (m_pRoomListLayout) {
        delete m_pRoomListLayout;
        m_pRoomListLayout = nullptr;
    }
    delete ui;
}

// ===== Nav Selection Visual =====
void friendList::setNavSelected(QPushButton *selected)
{
    for (int i = 0; i < m_navButtons.size(); ++i) {
        QPushButton* btn = m_navButtons[i];
        if (btn == selected) {
            // Selected: white text, left border accent #1677FF
            btn->setStyleSheet(
                "QPushButton {"
                "  color: #FFFFFF;"
                "  background-color: #0F172A;"
                "  border: none;"
                "  border-left: 3px solid #1677FF;"
                "  font-family: 'Microsoft YaHei';"
                "  font-size: 13px;"
                "  font-weight: bold;"
                "  text-align: center;"
                "  padding: 0px;"
                "}");
        } else {
            // Unselected: muted gray text
            btn->setStyleSheet(
                "QPushButton {"
                "  color: #94A3B8;"
                "  background-color: #0F172A;"
                "  border: none;"
                "  border-left: 3px solid transparent;"
                "  font-family: 'Microsoft YaHei';"
                "  font-size: 13px;"
                "  font-weight: normal;"
                "  text-align: center;"
                "  padding: 0px;"
                "}"
                "QPushButton:hover {"
                "  color: #E2E8F0;"
                "  background-color: #1E293B;"
                "}");
        }
    }

    // Settings always uses muted style
    ui->pb_navSettings->setStyleSheet(
        "QPushButton {"
        "  color: #94A3B8;"
        "  background-color: #0F172A;"
        "  border: none;"
        "  font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "  color: #E2E8F0;"
        "  background-color: #1E293B;"
        "  border-radius: 6px;"
        "}");

    // Menu button uses muted style
    ui->pb_menu->setStyleSheet(
        "QPushButton {"
        "  color: #94A3B8;"
        "  background-color: #0F172A;"
        "  border: none;"
        "  font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "  color: #E2E8F0;"
        "  background-color: #1E293B;"
        "  border-radius: 6px;"
        "}");
}

// ===== Left Nav Slots =====
void friendList::on_pb_navMeeting_clicked()
{
    qDebug() << __func__;
    ui->wdg_contentStack->setCurrentIndex(0);
    setNavSelected(ui->pb_navMeeting);
}

void friendList::on_pb_navChat_clicked()
{
    qDebug() << __func__;
    ui->wdg_contentStack->setCurrentIndex(1);
    setNavSelected(ui->pb_navChat);
}

void friendList::on_pb_navContacts_clicked()
{
    qDebug() << __func__;
    ui->wdg_contentStack->setCurrentIndex(2);
    setNavSelected(ui->pb_navContacts);
}

void friendList::on_pb_navSettings_clicked()
{
    qDebug() << __func__;
    // Placeholder for settings dialog
}

// ===== User Info =====
void friendList::setUserInfo(QString name, QString feeling, int iconId)
{
    (void)iconId; // no longer used — avatar is always blue
    ui->lb_name->setText(name);
    (void)feeling;  // feeling not shown in top-left; use ChatDialog instead
    qDebug() << name << " " << feeling;

    // Round blue avatar — DingTalk blue #1677FF
    QString initial = name.isEmpty() ? QString("?") : QString(name.at(0));
    ui->pb_icon->setStyleSheet(
        QString("QPushButton#pb_icon {"
                "background-color: #1677FF;"
                "color: #FFFFFF;"
                "border: 2px solid #1677FF;"
                "border-radius: 24px;"
                "font-size: 20px;"
                "font-weight: bold;"
                "}"));
    ui->pb_icon->setText(initial);

    // Style nav labels for dark background
    ui->lb_name->setStyleSheet(
        "QLabel { color: #FFFFFF; background: transparent; font-weight: bold; }");
    ui->le_feeling->setStyleSheet(
        "QLineEdit { color: #94A3B8; background: transparent; border: none; font-size: 11px; }");
}

// ===== Friend List (聊天 tab) =====
void friendList::addFriend(frienditem *item)
{
    m_playout->addWidget(item);
}

// ===== Room Lobby (会议 tab) =====
void friendList::addRoomToLobby(int roomId, int roomNumber, QString roomName, int memberCount, int joined)
{
    //qDebug() << __func__ << roomId << roomName << memberCount << joined;

    // Remove old item if exists
    if (m_mapRoomItems.contains(roomId)) {
        QWidget* oldItem = m_mapRoomItems[roomId];
        m_pRoomListLayout->removeWidget(oldItem);
        delete oldItem;
        m_mapRoomItems.remove(roomId);
    }

    // Create room item widget
    QWidget* wdg = new QWidget(ui->wdg_roomListInLobby);
    wdg->setMinimumHeight(64);
    wdg->setStyleSheet(
        "QWidget { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 10px; }"
        "QWidget:hover { background-color: #F0F5FF; border-color: #93C5FD; }");

    QHBoxLayout* hLayout = new QHBoxLayout(wdg);
    hLayout->setContentsMargins(16, 8, 16, 8);
    hLayout->setSpacing(12);

    // Room avatar circle
    QPushButton* pbRoomIcon = new QPushButton(wdg);
    pbRoomIcon->setFixedSize(44, 44);
    QStringList colors;
    colors << "#FF6B6B" << "#4ECDC4" << "#45B7D1" << "#96CEB4" << "#FFEAA7"
           << "#DDA0DD" << "#98D8C7" << "#F7DC6F" << "#BB8FCE" << "#85C1E9"
           << "#F8C471" << "#82E0AA";
    QString bgColor = colors[roomId % colors.size()];
    QString initial = roomName.isEmpty() ? QString("#") : QString(roomName.at(0));
    pbRoomIcon->setStyleSheet(
        QString("QPushButton {"
                "background-color: %1; color: #FFFFFF;"
                "border: none; border-radius: 22px;"
                "font-size: 18px; font-weight: bold;"
                "}").arg(bgColor));
    pbRoomIcon->setText(initial);
    hLayout->addWidget(pbRoomIcon);

    // Room name row
    QHBoxLayout* nameRow = new QHBoxLayout();
    nameRow->setSpacing(6);
    nameRow->setContentsMargins(0, 0, 0, 0);

    QLabel* lbRoomName = new QLabel(roomName, wdg);
    lbRoomName->setFont(QFont("Microsoft YaHei", 13));
    lbRoomName->setStyleSheet("color: #1E293B; background: transparent; border: none; font-weight: bold;");
    nameRow->addWidget(lbRoomName);
    nameRow->addStretch();

    // Room info: name row + number + member count
    QVBoxLayout* textLayout = new QVBoxLayout();
    textLayout->setSpacing(2);
    textLayout->addLayout(nameRow);

    QLabel* lbRoomNumber = new QLabel(QString("房间号：%1").arg(roomNumber), wdg);
    lbRoomNumber->setFont(QFont("Microsoft YaHei", 9));
    lbRoomNumber->setStyleSheet("color: #94A3B8; background: transparent; border: none;");
    textLayout->addWidget(lbRoomNumber);

    QLabel* lbMemberCount = new QLabel(QString("%1 人在线").arg(memberCount), wdg);
    lbMemberCount->setFont(QFont("Microsoft YaHei", 11));
    lbMemberCount->setStyleSheet("color: #64748B; background: transparent; border: none;");
    textLayout->addWidget(lbMemberCount);

    hLayout->addLayout(textLayout);
    hLayout->addStretch();

    // Action button: 进入 or 加入
    if (joined) {
        QPushButton* pbEnter = new QPushButton("进入", wdg);
        pbEnter->setStyleSheet(
            "QPushButton {"
            "background-color: #1677FF; color: #FFFFFF;"
            "border: none; border-radius: 8px;"
            "padding: 6px 20px; font-size: 13px; font-weight: bold;"
            "}"
            "QPushButton:hover { background-color: #3D8FFF; }");
        pbEnter->setFixedWidth(72);
        connect(pbEnter, &QPushButton::clicked, this, [this, roomId]() {
            Q_EMIT sig_enterRoom(roomId);
        });
        hLayout->addWidget(pbEnter);
    } else {
        QPushButton* pbJoin = new QPushButton("加入", wdg);
        pbJoin->setStyleSheet(
            "QPushButton {"
            "background-color: #10B981; color: #FFFFFF;"
            "border: none; border-radius: 8px;"
            "padding: 6px 20px; font-size: 13px; font-weight: bold;"
            "}"
            "QPushButton:hover { background-color: #34D399; }");
        pbJoin->setFixedWidth(72);
        connect(pbJoin, &QPushButton::clicked, this, [this, roomId]() {
            Q_EMIT sig_joinRoomById(roomId);
        });
        hLayout->addWidget(pbJoin);
    }

    // Delete button
    QPushButton* pbDelete = new QPushButton("x", wdg);
    pbDelete->setFixedSize(26, 26);
    pbDelete->setStyleSheet(
        "QPushButton {"
        "background-color: transparent; color: #94A3B8;"
        "border: 1px solid #CBD5E1; border-radius: 13px;"
        "font-size: 10px; font-weight: bold; padding: 0px;"
        "}"
        "QPushButton:hover { background-color: #FEE2E2; color: #EF4444; border-color: #EF4444; }");
    pbDelete->setToolTip("从列表中移除");
    connect(pbDelete, &QPushButton::clicked, this, [this, roomId]() {
        if (m_mapRoomItems.contains(roomId)) {
            QWidget* item = m_mapRoomItems[roomId];
            m_pRoomListLayout->removeWidget(item);
            delete item;
            m_mapRoomItems.remove(roomId);
        }
        Q_EMIT sig_removeRoomFromList(roomId);
    });
    hLayout->addWidget(pbDelete);

    m_pRoomListLayout->addWidget(wdg);
    m_mapRoomItems[roomId] = wdg;
}

void friendList::clearRoomList()
{
    qDebug() << __func__;
    for (QMap<int, QWidget*>::iterator it = m_mapRoomItems.begin(); it != m_mapRoomItems.end(); ++it) {
        m_pRoomListLayout->removeWidget(it.value());
        delete it.value();
    }
    m_mapRoomItems.clear();
}

void friendList::updateRoomMemberCount(int roomId, int count)
{
    // Update the room card in lobby with new member count
    if (m_mapRoomItems.contains(roomId)) {
        // Remove old card and recreate via refreshRoomItemInLobby
        // which is called from CKernel with full data
    }
    (void)roomId;
    (void)count;
}

void friendList::updateRoomMembers(int roomId, const QStringList& members, const QList<int>& userIds, int totalCount)
{
    (void)roomId;
    (void)totalCount;
    ui->lw_roomMembers->clear();

    // Populate userId->name map for video labels
    int count = qMin(members.size(), userIds.size());
    for (int i = 0; i < count; ++i) {
        m_mapUserNames[userIds[i]] = members[i];
    }

    // Ensure every member has a video cell (even without camera)
    if (m_pVideoGridLayout) {
        for (int i = 0; i < count; ++i) {
            int uid = userIds[i];
            if (!m_mapVideoLabels.contains(uid)) {
                // Create container (same as displayVideoFrame)
                QWidget* container = new QWidget(m_pVideoGridWidget);
                container->setMinimumSize(200, 150);
                container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
                container->setStyleSheet("background:#12192B;border:2px solid #334155;border-radius:8px;");
                container->setCursor(Qt::PointingHandCursor);
                container->setProperty("videoUserId", uid);

                QLabel* videoLabel = new QLabel(container);
                videoLabel->setAlignment(Qt::AlignCenter);
                videoLabel->setScaledContents(false);
                videoLabel->setGeometry(0, 0, 1, 1);
                videoLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
                videoLabel->setText(members[i]);
                videoLabel->setStyleSheet("QLabel{background:#12192B;border:none;border-radius:8px;color:#64748B;font-size:18px;font-weight:bold;}");

                QLabel* nameLabel = new QLabel(members[i], container);
                nameLabel->setStyleSheet("QLabel{color:#FFF;background:rgba(0,0,0,0.6);border:none;border-radius:4px;padding:3px 8px;font-size:12px;}");
                nameLabel->adjustSize();
                nameLabel->move(6, 6);

                container->installEventFilter(this);
                container->setProperty("nameLabel", QVariant::fromValue<QWidget*>(nameLabel));
                m_mapVideoLabels[uid] = videoLabel;
            }
        }
        arrangeVideoGrid();
    }

    for (int i = 0; i < members.size(); ++i) {
        const QString& name = members[i];
        QListWidgetItem* item = new QListWidgetItem;
        item->setSizeHint(QSize(0, 40));
        if (i < userIds.size())
            item->setData(Qt::UserRole, userIds[i]);

        QWidget* wdg = new QWidget;
        wdg->setCursor(Qt::PointingHandCursor);
        QHBoxLayout* layout = new QHBoxLayout(wdg);
        layout->setContentsMargins(4, 2, 4, 2);
        layout->setSpacing(8);

        QString initial = name.isEmpty() ? "?" : QString(name.at(0));
        QPushButton* pb_avatar = new QPushButton(initial, wdg);
        pb_avatar->setFixedSize(24, 24);
        pb_avatar->setStyleSheet(
            "QPushButton {"
            "  background-color: #1677FF; color: #FFFFFF;"
            "  border: none; border-radius: 14px;"
            "  font-size: 13px; font-weight: bold;"
            "}");
        layout->addWidget(pb_avatar);

        QLabel* lb_name = new QLabel(name, wdg);
        lb_name->setStyleSheet("color:#E2E8F0;background:transparent;border:none;font-size:12px;");
        layout->addWidget(lb_name);
        layout->addStretch();

        ui->lw_roomMembers->addItem(item);
        ui->lw_roomMembers->setItemWidget(item, wdg);
    }

    // Auto-enlarge host (first member) on room entry
    if (m_enlargedUserId == 0 && userIds.size() > 0 && m_pVideoGridLayout) {
        slot_videoClicked(userIds.first());
    }
}

void friendList::refreshRoomItemInLobby(int roomId, int roomNumber, QString roomName, int memberCount, int joined)
{
    qDebug() << __func__ << roomId << roomNumber << roomName << memberCount << joined;
    // Delegate to addRoomToLobby which handles remove+recreate
    addRoomToLobby(roomId, roomNumber, roomName, memberCount, joined);
}

// ===== Room View (embedded in 会议 tab) =====
void friendList::showRoomView(int roomId, int roomNumber, QString roomName, bool isCreator)
{
    qDebug() << __func__ << roomId << roomNumber << roomName;
    m_currentRoomId = roomId;
    m_currentRoomName = roomName;
    m_currentRoomNumber = roomNumber;
    m_isRoomCreator = isCreator;
    m_muted = true;
    m_cameraOff = true;

    QWidget* page = ui->page_roomView;
    // Remove old layout if exists
    if (page->layout()) {
        QLayoutItem* item;
        while ((item = page->layout()->takeAt(0))) {
            if (item->widget()) item->widget()->setParent(nullptr);
            delete item;
        }
        delete page->layout();
    }

    QVBoxLayout* root = new QVBoxLayout(page);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    root->setAlignment(Qt::AlignTop);

    // ===== TOP BAR =====
    QWidget* topBar = new QWidget(page);
    topBar->setFixedHeight(56);
    topBar->setStyleSheet("background: #0F172A;");
    QHBoxLayout* topLay = new QHBoxLayout(topBar);
    topLay->setContentsMargins(16, 8, 16, 8);

    // Room name (left)
    QLabel* lbRoomName = new QLabel(roomName, topBar);
    lbRoomName->setStyleSheet("color: #F1F5F9; font-size: 16px; font-weight: bold; background: transparent;");
    topLay->addWidget(lbRoomName);
    topLay->addStretch(1);

    // Room number + copy (center)
    QLabel* lbRoomNum = new QLabel(QString("房间号 %1").arg(roomNumber), topBar);
    lbRoomNum->setStyleSheet("color: #3B82F6; font-size: 14px; background: transparent;");
    topLay->addWidget(lbRoomNum);
    QPushButton* pbCopy = new QPushButton("复制", topBar);
    pbCopy->setStyleSheet("QPushButton{color:#94A3B8;background:#1E293B;border:none;border-radius:4px;padding:4px 12px;font-size:12px;}QPushButton:hover{color:#E2E8F0;background:#334155;}");
    pbCopy->setCursor(Qt::PointingHandCursor);
    int rn = roomNumber;
    connect(pbCopy, &QPushButton::clicked, [rn]() {
        QApplication::clipboard()->setText(QString::number(rn));
    });
    topLay->addWidget(pbCopy);

    topLay->addStretch(1);

    // Exit button (right, always visible)
    QPushButton* pbExit = new QPushButton("退出会议", topBar);
    pbExit->setStyleSheet("QPushButton{color:#94A3B8;background:#1E293B;border:1px solid #475569;border-radius:6px;padding:6px 16px;font-size:13px;}QPushButton:hover{color:#E2E8F0;background:#334155;}");
    pbExit->setCursor(Qt::PointingHandCursor);
    connect(pbExit, &QPushButton::clicked, this, &friendList::on_pb_leaveRoom_clicked);
    topLay->addWidget(pbExit);

    // End meeting (right, host only)
    QPushButton* pbEnd = new QPushButton("结束会议", topBar);
    pbEnd->setStyleSheet("QPushButton{background:#EF4444;color:#FFF;border:none;border-radius:6px;padding:6px 16px;font-size:13px;font-weight:bold;}QPushButton:hover{background:#DC2626;}");
    pbEnd->setCursor(Qt::PointingHandCursor);
    pbEnd->setVisible(isCreator);
    connect(pbEnd, &QPushButton::clicked, this, &friendList::on_pb_endMeeting_clicked);
    topLay->addWidget(pbEnd);

    root->addWidget(topBar);

    // ===== CENTER: Video Grid + Right Panel =====
    QWidget* center = new QWidget(page);
    QHBoxLayout* centerLay = new QHBoxLayout(center);
    centerLay->setContentsMargins(0, 0, 0, 0);
    centerLay->setSpacing(0);

    // Video grid widget
    m_pVideoGridWidget = new QWidget(center);
    m_pVideoGridWidget->setStyleSheet("background: #0A0E27;");
    m_pVideoGridLayout = new QGridLayout(m_pVideoGridWidget);
    m_pVideoGridLayout->setContentsMargins(8, 8, 8, 8);
    m_pVideoGridLayout->setSpacing(6);
    centerLay->addWidget(m_pVideoGridWidget, 1);

    // Enlarged overlay (hidden initially)
    m_pEnlargedOverlay = new QWidget(m_pVideoGridWidget);
    m_pEnlargedOverlay->setStyleSheet("background: #0A0E27;");
    m_pEnlargedOverlay->setVisible(false);
    QVBoxLayout* overlayLay = new QVBoxLayout(m_pEnlargedOverlay);
    QPushButton* pbBack = new QPushButton("返回网格", m_pEnlargedOverlay);
    pbBack->setFixedSize(100, 36);
    pbBack->setStyleSheet("QPushButton{color:#E2E8F0;background:#1E293B;border:1px solid #475569;border-radius:6px;font-size:13px;}QPushButton:hover{background:#334155;}");
    pbBack->setCursor(Qt::PointingHandCursor);
    connect(pbBack, &QPushButton::clicked, this, &friendList::slot_closeEnlarged);
    overlayLay->addWidget(pbBack);
    m_pEnlargedVideo = new QLabel(m_pEnlargedOverlay);
    m_pEnlargedVideo->setAlignment(Qt::AlignCenter);
    m_pEnlargedVideo->setStyleSheet("background: #0A0E27;");
    m_pEnlargedVideo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    overlayLay->addWidget(m_pEnlargedVideo, 1);
    m_enlargedUserId = 0;

    // Right panel
    m_pRightPanel = new QWidget(center);
    m_pRightPanel->setFixedWidth(320);
    m_pRightPanel->setStyleSheet("background: #1E293B;");
    QVBoxLayout* rightLay = new QVBoxLayout(m_pRightPanel);
    rightLay->setContentsMargins(0, 0, 0, 0);
    rightLay->setSpacing(0);

    // Collapse button
    m_pBtnCollapse = new QPushButton("》", m_pRightPanel);
    m_pBtnCollapse->setFixedSize(28, 60);
    m_pBtnCollapse->setCursor(Qt::PointingHandCursor);
    m_pBtnCollapse->setStyleSheet("QPushButton{color:#94A3B8;background:#1E293B;border:none;font-size:14px;}QPushButton:hover{color:#E2E8F0;background:#334155;}");
    connect(m_pBtnCollapse, &QPushButton::clicked, this, &friendList::slot_toggleChatPanel);
    m_chatCollapsed = false;

    // Chat area (reuse existing widgets)
    QWidget* chatArea = new QWidget(m_pRightPanel);
    QVBoxLayout* chatLay = new QVBoxLayout(chatArea);
    chatLay->setContentsMargins(8, 8, 8, 8);
    QLabel* lbChatTitle = new QLabel("聊天", chatArea);
    lbChatTitle->setStyleSheet("color:#F1F5F9;font-size:14px;font-weight:bold;background:transparent;");
    chatLay->addWidget(lbChatTitle);
    // Reparent existing chat widgets
    ui->tb_roomChat->setParent(chatArea);
    ui->tb_roomChat->clear();
    ui->tb_roomChat->append(QString("=== 欢迎进入会议室【%1】===").arg(roomName));
    ui->tb_roomChat->append("");
    ui->tb_roomChat->setStyleSheet("QTextBrowser{color:#E2E8F0;background:#0F172A;border:1px solid #334155;border-radius:6px;font-size:12px;}");
    chatLay->addWidget(ui->tb_roomChat, 1);
    QWidget* inputRow = new QWidget(chatArea);
    QHBoxLayout* inputLay = new QHBoxLayout(inputRow);
    inputLay->setContentsMargins(0, 4, 0, 0);
    ui->te_roomInput->setParent(inputRow);
    ui->te_roomInput->clear();
    ui->te_roomInput->setFixedHeight(40);
    ui->te_roomInput->setStyleSheet("QTextEdit{color:#E2E8F0;background:#0F172A;border:1px solid #334155;border-radius:6px;font-size:12px;padding:4px;}");
    inputLay->addWidget(ui->te_roomInput, 1);
    QPushButton* pbSend = new QPushButton("发送", inputRow);
    pbSend->setFixedSize(52, 36);
    pbSend->setCursor(Qt::PointingHandCursor);
    pbSend->setStyleSheet("QPushButton{color:#FFF;background:#3B82F6;border:none;border-radius:6px;font-size:12px;font-weight:bold;padding:0px;}QPushButton:hover{background:#2563EB;}");
    connect(pbSend, &QPushButton::clicked, this, &friendList::on_pb_roomSend_clicked);
    inputLay->addWidget(pbSend);
    chatLay->addWidget(inputRow);
    rightLay->addWidget(chatArea, 7);

    // Member panel
    QWidget* memberArea = new QWidget(m_pRightPanel);
    QVBoxLayout* memberLay = new QVBoxLayout(memberArea);
    memberLay->setContentsMargins(8, 4, 8, 8);
    QLabel* lbMembers = new QLabel("参会成员", memberArea);
    lbMembers->setStyleSheet("color:#F1F5F9;font-size:14px;font-weight:bold;background:transparent;");
    memberLay->addWidget(lbMembers);
    ui->lw_roomMembers->setParent(memberArea);
    ui->lw_roomMembers->clear();
    ui->lw_roomMembers->setStyleSheet("QListWidget{color:#E2E8F0;background:#0F172A;border:1px solid #334155;border-radius:6px;font-size:12px;}QListWidget::item{padding:2px 4px;}");
    connect(ui->lw_roomMembers, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        int uid = item->data(Qt::UserRole).toInt();
        if (uid > 0) slot_videoClicked(uid);
    });
    memberLay->addWidget(ui->lw_roomMembers, 1);
    rightLay->addWidget(memberArea, 3);

    centerLay->addWidget(m_pRightPanel);

    // Collapse button — drawer handle on LEFT edge of right panel
    m_pBtnCollapse->setParent(page);
    QTimer::singleShot(0, [this]() {
        if (m_pRightPanel && m_pBtnCollapse) {
            int y = m_pRightPanel->y() + m_pRightPanel->height() / 2 - 30;
            m_pBtnCollapse->move(m_pRightPanel->x(), y);
        }
    });
    m_pBtnCollapse->raise();

    root->addWidget(center, 1);

    // ===== BOTTOM BAR =====
    m_pBottomBar = new QWidget(page);
    m_pBottomBar->setFixedHeight(64);
    m_pBottomBar->setStyleSheet("background: #0F172A;");
    QHBoxLayout* botLay = new QHBoxLayout(m_pBottomBar);
    botLay->setContentsMargins(16, 12, 16, 12);
    botLay->setAlignment(Qt::AlignCenter);
    botLay->setSpacing(24);

    auto makeCtrlBtn = [](const QString& text, QWidget* parent) {
        QPushButton* b = new QPushButton(text, parent);
        b->setFixedSize(120, 40);
        b->setCursor(Qt::PointingHandCursor);
        b->setStyleSheet("QPushButton{color:#E2E8F0;background:#1E293B;border:1px solid #475569;border-radius:8px;font-size:13px;}QPushButton:hover{background:#334155;}");
        return b;
    };

    m_pBtnMute = makeCtrlBtn("已静音", m_pBottomBar);
    m_pBtnMute->setStyleSheet("QPushButton{color:#FFF;background:#EF4444;border:none;border-radius:8px;font-size:13px;}QPushButton:hover{background:#DC2626;}");
    connect(m_pBtnMute, &QPushButton::clicked, this, &friendList::slot_toggleMute);
    botLay->addWidget(m_pBtnMute);

    m_pBtnCamera = makeCtrlBtn("已关闭摄像头", m_pBottomBar);
    m_pBtnCamera->setStyleSheet("QPushButton{color:#FFF;background:#EF4444;border:none;border-radius:8px;font-size:13px;}QPushButton:hover{background:#DC2626;}");
    connect(m_pBtnCamera, &QPushButton::clicked, this, &friendList::slot_toggleCamera);
    botLay->addWidget(m_pBtnCamera);

    QPushButton* pbShare = makeCtrlBtn("共享屏幕", m_pBottomBar);
    pbShare->setEnabled(true);
    connect(pbShare, &QPushButton::clicked, [this, pbShare]() {
        static bool sharing = false;
        sharing = !sharing;
        pbShare->setText(sharing ? "停止共享" : "共享屏幕");
        pbShare->setStyleSheet(sharing ?
            "QPushButton{color:#FFF;background:#22C55E;border:none;border-radius:8px;font-size:13px;}QPushButton:hover{background:#16A34A;}" :
            "QPushButton{color:#E2E8F0;background:#1E293B;border:1px solid #475569;border-radius:8px;font-size:13px;}QPushButton:hover{background:#334155;}");
        Q_EMIT sig_toggleScreenShare();
    });
    botLay->addWidget(pbShare);

    root->addWidget(m_pBottomBar);

    ui->wdg_meetingStack->setCurrentIndex(1);
    arrangeVideoGrid();
}

void friendList::showRoomLobby()
{
    qDebug() << __func__;
    m_currentRoomId = 0;
    m_currentRoomName.clear();
    // Clean up video grid
    for (auto it = m_mapVideoLabels.begin(); it != m_mapVideoLabels.end(); ++it)
        delete it.value();
    m_mapVideoLabels.clear();
    m_mapLastFrames.clear();
    m_pVideoGridWidget = nullptr;
    m_pVideoGridLayout = nullptr;
    m_pEnlargedVideo = nullptr;
    m_pEnlargedOverlay = nullptr;
    m_pRightPanel = nullptr;
    m_pBtnCollapse = nullptr;
    m_pBottomBar = nullptr;
    m_pBtnMute = nullptr;
    m_pBtnCamera = nullptr;
    // Reparent chat widgets back to avoid dangling pointers
    if (ui->tb_roomChat) ui->tb_roomChat->setParent(this);
    if (ui->te_roomInput) ui->te_roomInput->setParent(this);
    if (ui->lw_roomMembers) ui->lw_roomMembers->setParent(this);
    ui->wdg_meetingStack->setCurrentIndex(0);
}

void friendList::displayRoomMessage(QString senderName, QString content)
{
    ui->tb_roomChat->append(
        QString("【%1】 %2").arg(senderName).arg(QTime::currentTime().toString("hh:mm:ss")));
    ui->tb_roomChat->append(content);
}

void friendList::displayVideoFrame(int userId, QByteArray jpegData, bool isScreen)
{
    m_mapLastFrames[userId] = jpegData;

    bool isEnlarged = (m_enlargedUserId == userId && m_pEnlargedOverlay && m_pEnlargedOverlay->isVisible());

    if (isScreen) {
        // Screen share frame → store separately, display as main content
        m_userHasScreen[userId] = true;
        m_mapLastScreenFrames[userId] = jpegData;

        if (isEnlarged) {
            QPixmap pix;
            if (pix.loadFromData(jpegData, "JPEG"))
                m_pEnlargedVideo->setPixmap(pix.scaled(m_pEnlargedVideo->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            return;
        }
        // Fall through to grid display for screen
    } else {
        // Camera frame
        if (m_userHasScreen.value(userId, false)) {
            // User is screen sharing → show camera as PIP
            if (isEnlarged) {
                // Update enlarged view PIP continuously
                QLabel* pip = m_pEnlargedOverlay->findChild<QLabel*>("enlargedPip");
                if (!pip) {
                    pip = new QLabel(m_pEnlargedOverlay);
                    pip->setObjectName("enlargedPip");
                    pip->setFixedSize(240, 160);
                    pip->setStyleSheet("QLabel{background:#12192B;border:2px solid #3B82F6;border-radius:8px;}");
                }
                QPixmap px;
                if (px.loadFromData(jpegData, "JPEG")) {
                    pip->setPixmap(px.scaled(240, 160, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                    pip->move(m_pEnlargedOverlay->width() - 248, 8);
                    pip->raise();
                    pip->show();
                }
            } else {
                showLocalPIP(userId, jpegData);
            }
            return;
        }

        // Normal camera (no screen sharing) → show in grid/enlarged
        if (isEnlarged) {
            QPixmap pix;
            if (pix.loadFromData(jpegData, "JPEG"))
                m_pEnlargedVideo->setPixmap(pix.scaled(m_pEnlargedVideo->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            return;
        }
    }

    // ===== Grid display (both camera-only and screen share) =====
    if (!m_mapVideoLabels.contains(userId)) {
        // Container widget: video + name overlay
        QWidget* container = new QWidget(m_pVideoGridWidget ? (QWidget*)m_pVideoGridWidget : (QWidget*)this);
        container->setMinimumSize(200, 150);
        container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        container->setStyleSheet("background:#12192B;border:2px solid #334155;border-radius:8px;");
        container->setCursor(Qt::PointingHandCursor);
        container->setProperty("videoUserId", userId);

        QLabel* videoLabel = new QLabel(container);
        videoLabel->setAlignment(Qt::AlignCenter);
        videoLabel->setScaledContents(false);
        videoLabel->setGeometry(0, 0, container->width(), container->height());
        videoLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

        QLabel* nameLabel = new QLabel(container);
        nameLabel->setText(m_mapUserNames.value(userId, QString("用户%1").arg(userId)));
        nameLabel->setStyleSheet("QLabel{color:#FFF;background:rgba(0,0,0,0.6);border:none;border-radius:4px;padding:3px 8px;font-size:12px;}");
        nameLabel->adjustSize();
        nameLabel->move(6, container->height() - nameLabel->height() - 6);

        // Mouse click → enlarge
        container->installEventFilter(this);
        container->setProperty("nameLabel", QVariant::fromValue<QWidget*>(nameLabel));

        m_mapVideoLabels[userId] = videoLabel;
        arrangeVideoGrid();
    }

    QLabel* label = m_mapVideoLabels[userId];
    QPixmap pix;
    if (pix.loadFromData(jpegData, "JPEG")) {
        label->setPixmap(pix.scaled(label->parentWidget()->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        label->setStyleSheet("QLabel{background:#12192B;border:none;border-radius:8px;}");
    }
}

void friendList::showLocalPIP(int userId, QByteArray jpegData)
{
    // Find the container for this user's video grid cell
    if (!m_mapVideoLabels.contains(userId)) return;
    QLabel* mainLabel = m_mapVideoLabels[userId];
    QWidget* container = mainLabel->parentWidget();
    if (!container) return;

    // Find or create PIP label inside this container
    QLabel* pip = container->findChild<QLabel*>("pipLabel");
    if (!pip) {
        pip = new QLabel(container);
        pip->setObjectName("pipLabel");
        pip->setFixedSize(120, 80);
        pip->setStyleSheet("QLabel{background:#12192B;border:2px solid #3B82F6;border-radius:4px;color:#64748B;font-size:10px;}");
        pip->setAlignment(Qt::AlignCenter);
        pip->setText("摄像头");
        pip->raise();
        pip->show();
    }
    // Position top-right of container
    pip->move(container->width() - 128, 4);
    QPixmap pix;
    if (pix.loadFromData(jpegData, "JPEG")) {
        pip->setPixmap(pix.scaled(120, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        pip->setText("");
        pip->setStyleSheet("QLabel{background:#12192B;border:2px solid #3B82F6;border-radius:4px;}");
    }
}

void friendList::clearVideoFrame(int userId)
{
    m_mapLastFrames.remove(userId);
    m_mapLastScreenFrames.remove(userId);
    m_userHasScreen.remove(userId);
    if (m_enlargedUserId == userId) slot_closeEnlarged();

    // Clear PIP if exists
    if (m_mapVideoLabels.contains(userId)) {
        QWidget* container = m_mapVideoLabels[userId]->parentWidget();
        if (container) {
            QLabel* pip = container->findChild<QLabel*>("pipLabel");
            if (pip) pip->hide();
        }
    }

    if (m_mapVideoLabels.contains(userId)) {
        QLabel* label = m_mapVideoLabels[userId];
        label->clear();
        label->setPixmap(QPixmap());
        QString name = m_mapUserNames.value(userId, QString("用户%1").arg(userId));
        label->setText(name);
        label->setStyleSheet("QLabel{background:#12192B;border:2px solid #334155;border-radius:8px;color:#64748B;font-size:14px;}");
    }
}

// ===== Video Grid =====

void friendList::arrangeVideoGrid()
{
    if (!m_pVideoGridLayout) return;
    // Clear grid
    QLayoutItem* item;
    while ((item = m_pVideoGridLayout->takeAt(0))) delete item;

    int count = m_mapVideoLabels.size();
    if (count == 0) return;

    int cols, rows;
    if (count == 1)      { cols = 1; rows = 1; }
    else if (count == 2) { cols = 2; rows = 1; }
    else if (count <= 4) { cols = 2; rows = 2; }
    else if (count <= 6) { cols = 3; rows = 2; }
    else                 { cols = 3; rows = 3; }

    int idx = 0;
    for (auto it = m_mapVideoLabels.begin(); it != m_mapVideoLabels.end(); ++it, ++idx) {
        int r = idx / cols;
        int c = idx % cols;
        QWidget* container = it.value()->parentWidget(); // videoLabel is inside a container
        if (container)
            m_pVideoGridLayout->addWidget(container, r, c);
    }
    // Re-raise enlarged overlay so it stays on top of grid
    if (m_pEnlargedOverlay && m_pEnlargedOverlay->isVisible()) {
        m_pEnlargedOverlay->raise();
        QLabel* pip = m_pEnlargedOverlay->findChild<QLabel*>("enlargedPip");
        if (pip) pip->raise();
    }
}

void friendList::slot_videoClicked(int userId)
{
    if (!m_pEnlargedOverlay || !m_pEnlargedVideo) return;

    m_enlargedUserId = userId;
    QString name = m_mapUserNames.value(userId, QString("用户%1").arg(userId));

    // Prefer screen frame for enlarge, fall back to camera frame
    qDebug() << "[DEBUG] slot_videoClicked userId:" << userId
             << "hasScreen:" << m_mapLastScreenFrames.contains(userId)
             << "screenSize:" << (m_mapLastScreenFrames.contains(userId) ? m_mapLastScreenFrames[userId].size() : -1)
             << "lastSize:" << (m_mapLastFrames.contains(userId) ? m_mapLastFrames[userId].size() : -1)
             << "userHasScreen:" << m_userHasScreen.value(userId, false);
    bool hasFrame = m_mapLastScreenFrames.contains(userId) || m_mapLastFrames.contains(userId);
    if (hasFrame) {
        QByteArray& data = m_mapLastScreenFrames.contains(userId)
            ? m_mapLastScreenFrames[userId] : m_mapLastFrames[userId];
        QPixmap pix;
        if (pix.loadFromData(data, "JPEG")) {
            m_pEnlargedVideo->setPixmap(pix.scaled(m_pEnlargedVideo->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            // Name overlay on enlarged video
            m_pEnlargedVideo->setText("");
            m_pEnlargedVideo->setStyleSheet("QLabel{background:#0A0E27;}");
        }
    } else {
        m_pEnlargedVideo->clear();
        m_pEnlargedVideo->setPixmap(QPixmap());
        m_pEnlargedVideo->setText(name + "\n摄像头未开启");
        m_pEnlargedVideo->setStyleSheet("QLabel{background:#0A0E27;color:#64748B;font-size:24px;font-weight:bold;}");
    }
    // PIP in enlarged view
    QLabel* pip = m_pEnlargedOverlay->findChild<QLabel*>("enlargedPip");
    if (m_userHasScreen.value(userId, false) && m_mapVideoLabels.contains(userId)) {
        QWidget* c = m_mapVideoLabels[userId]->parentWidget();
        if (c) { QLabel* cp = c->findChild<QLabel*>("pipLabel");
        if (cp && cp->pixmap() && !cp->pixmap()->isNull()) {
            if (!pip) { pip = new QLabel(m_pEnlargedOverlay); pip->setObjectName("enlargedPip");
                pip->setFixedSize(240, 160); pip->setStyleSheet("QLabel{background:#12192B;border:2px solid #3B82F6;border-radius:8px;}"); }
            pip->setPixmap(cp->pixmap()->scaled(240, 160, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            pip->move(m_pEnlargedOverlay->width() - 248, 8); pip->raise(); pip->show();
        }}
    } else { if (pip) pip->hide(); }

    m_pEnlargedOverlay->setGeometry(m_pVideoGridWidget->rect());
    m_pEnlargedOverlay->setVisible(true);
    m_pEnlargedOverlay->raise();
}

void friendList::slot_closeEnlarged()
{
    m_enlargedUserId = 0;
    if (m_pEnlargedOverlay)
        m_pEnlargedOverlay->setVisible(false);
}

// ===== Right Panel =====

void friendList::slot_toggleChatPanel()
{
    if (!m_pRightPanel || !m_pBtnCollapse) return;
    m_chatCollapsed = !m_chatCollapsed;
    m_pRightPanel->setVisible(!m_chatCollapsed);
    m_pBtnCollapse->setText(m_chatCollapsed ? "《" : "》");

    // Force layout recalculation so video grid fills the freed space
    QWidget* center = m_pVideoGridWidget ? m_pVideoGridWidget->parentWidget() : nullptr;
    if (center && center->layout()) {
        center->layout()->invalidate();
        center->layout()->activate();
    }
    if (m_pVideoGridWidget) {
        m_pVideoGridWidget->updateGeometry();
    }

    // Delay positioning until layout reflow completes
    QTimer::singleShot(0, [this]() {
        if (!m_pBtnCollapse) return;
        QWidget* page = m_pBtnCollapse->parentWidget();
        if (!page) return;

        if (m_chatCollapsed) {
            // Panel hidden → handle at right edge of screen, vertically centered
            int y = page->height() / 2 - 30;
            m_pBtnCollapse->move(page->width() - 28, y);
        } else {
            // Panel visible → handle at left edge of panel, vertically centered
            if (m_pRightPanel) {
                int y = m_pRightPanel->y() + m_pRightPanel->height() / 2 - 30;
                m_pBtnCollapse->move(m_pRightPanel->x(), y);
            }
        }
        m_pBtnCollapse->raise();

        // Update enlarged overlay to match new video grid size
        if (m_pEnlargedOverlay && m_pEnlargedOverlay->isVisible() && m_pVideoGridWidget) {
            m_pEnlargedOverlay->setGeometry(m_pVideoGridWidget->rect());
            QLabel* pip = m_pEnlargedOverlay->findChild<QLabel*>("enlargedPip");
            if (pip) pip->move(m_pEnlargedOverlay->width() - 248, 8);
        }

        // Reposition grid PIPs to new container size
        for (auto it = m_mapVideoLabels.begin(); it != m_mapVideoLabels.end(); ++it) {
            QWidget* container = it.value()->parentWidget();
            if (container) {
                QLabel* pip = container->findChild<QLabel*>("pipLabel");
                if (pip && pip->isVisible())
                    pip->move(container->width() - 128, 4);
            }
        }
    });
}

// ===== Bottom Bar =====

void friendList::slot_toggleMute()
{
    m_muted = !m_muted;
    m_pBtnMute->setText(m_muted ? "已静音" : "静音");
    m_pBtnMute->setStyleSheet(m_muted ?
        "QPushButton{color:#FFF;background:#EF4444;border:none;border-radius:8px;font-size:13px;}QPushButton:hover{background:#DC2626;}" :
        "QPushButton{color:#E2E8F0;background:#1E293B;border:1px solid #475569;border-radius:8px;font-size:13px;}QPushButton:hover{background:#334155;}");
    Q_EMIT sig_toggleMute(m_muted);
}

void friendList::slot_toggleCamera()
{
    m_cameraOff = !m_cameraOff;
    m_pBtnCamera->setText(m_cameraOff ? "已关闭摄像头" : "关闭摄像头");
    m_pBtnCamera->setStyleSheet(m_cameraOff ?
        "QPushButton{color:#FFF;background:#EF4444;border:none;border-radius:8px;font-size:13px;}QPushButton:hover{background:#DC2626;}" :
        "QPushButton{color:#E2E8F0;background:#1E293B;border:1px solid #475569;border-radius:8px;font-size:13px;}QPushButton:hover{background:#334155;}");
    Q_EMIT sig_toggleCamera(m_cameraOff);
}

void friendList::setMuted(bool muted) { m_muted = muted; }
void friendList::setCameraOff(bool off) { m_cameraOff = off; }

// ===== Button Handlers =====
void friendList::on_pb_createMeeting_clicked()
{
    qDebug() << __func__;
    Q_EMIT sig_createRoom();
}

void friendList::on_pb_joinMeeting_clicked()
{
    qDebug() << __func__;
    Q_EMIT sig_joinRoom();
}

void friendList::on_pb_backToLobby_clicked()
{
    qDebug() << __func__;
    if (m_currentRoomId > 0) {
        int roomId = m_currentRoomId;
        Q_EMIT sig_leaveRoom(roomId);
        showRoomLobby();
    }
}

void friendList::on_pb_leaveRoom_clicked()
{
    qDebug() << __func__;
    if (m_currentRoomId > 0) {
        int roomId = m_currentRoomId;
        showRoomLobby();
        Q_EMIT sig_leaveRoom(roomId);
    }
}

void friendList::on_pb_endMeeting_clicked()
{
    qDebug() << __func__;
    if (m_currentRoomId > 0 && m_isRoomCreator) {
        Q_EMIT sig_endMeeting(m_currentRoomId);
    }
}

void friendList::on_pb_roomSend_clicked()
{
    qDebug() << __func__;
    QString content = ui->te_roomInput->toPlainText();
    QString contentTmp = content;
    if (content.isEmpty() || contentTmp.remove(" ").isEmpty()) {
        ui->te_roomInput->setText("");
        return;
    }

    ui->tb_roomChat->append(QString("我 %1").arg(QTime::currentTime().toString("hh:mm:ss")));
    ui->tb_roomChat->append(content);
    ui->te_roomInput->setText("");

    Q_EMIT sig_roomChatMessage(m_currentRoomId, content);
}

void friendList::on_pb_addFriend_clicked()
{
    qDebug() << __func__;
    Q_EMIT sig_addFriend();
}

void friendList::slot_roomItemClicked(int roomId)
{
    qDebug() << __func__ << roomId;
    Q_EMIT sig_enterRoom(roomId);
}

// ===== Menu =====
void friendList::on_pb_menu_clicked()
{
    QPoint p = QCursor::pos();
    QSize size = m_pMenu->sizeHint();
    QPoint pa = QPoint(p.x(), p.y() - size.height());
    m_pMenu->exec(pa);
}

void friendList::slot_menuTrigger(QAction *Action)
{
    qDebug() << __func__;
    if (Action->text() == "添加好友") {
        Q_EMIT sig_addFriend();
    } else {
        qDebug() << "系统设置";
    }
}

// ===== Close =====
bool friendList::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        int uid = obj->property("videoUserId").toInt();
        if (uid > 0) {
            slot_videoClicked(uid);
            return true;
        }
    }
    if (event->type() == QEvent::Resize) {
        QWidget* w = qobject_cast<QWidget*>(obj);
        if (w && w->property("nameLabel").isValid()) {
            QLabel* nl = qobject_cast<QLabel*>(w->property("nameLabel").value<QWidget*>());
            if (nl) {
                nl->move(6, w->height() - nl->height() - 6);
                // Also resize child video label to match container
                const QList<QObject*>& children = w->children();
                for (QObject* c : children) {
                    QLabel* vl = qobject_cast<QLabel*>(c);
                    if (vl && vl != nl) {
                        vl->setGeometry(0, 0, w->width(), w->height());
                    }
                }
            }
        }
    }
    return QDialog::eventFilter(obj, event);
}

void friendList::closeEvent(QCloseEvent *event)
{
    event->ignore();
    if (QMessageBox::Yes == QMessageBox::question(this, "询问", "是否确认关闭？")) {
        Q_EMIT sig_offline();
    }
}

void friendList::on_pb_toggleMembers_clicked()
{
    // Forward to member panel toggle (compat with old moc)
    on_pb_memberHandle_clicked();
}

void friendList::on_pb_memberHandle_clicked()
{
    // Member panel now always visible in right panel — toggle via chat collapse instead
    slot_toggleChatPanel();
}

void friendList::on_pb_copyRoomNumber_clicked()
{
    QString roomNumberStr = QString::number(m_currentRoomNumber);
    QApplication::clipboard()->setText(roomNumberStr);
    qDebug() << __func__ << "copied room number:" << roomNumberStr;
}
