#include "roomlist.h"
#include "ui_roomlist.h"
#include <QDebug>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QMessageBox>

// ========== RoomItem ==========
RoomItem::RoomItem(QWidget *parent) : QWidget(parent)
{
    setMinimumSize(340, 70);
    setMaximumSize(380, 70);
    setStyleSheet("background: transparent;");

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(12);

    // Colored circle avatar button
    m_pb_icon = new QPushButton(this);
    m_pb_icon->setFixedSize(50, 50);
    m_pb_icon->setFlat(true);
    layout->addWidget(m_pb_icon);

    // Name + member count
    QVBoxLayout* textLayout = new QVBoxLayout();
    textLayout->setSpacing(2);

    m_lb_name = new QLabel(this);
    m_lb_name->setFont(QFont("Microsoft YaHei", 13));
    m_lb_name->setStyleSheet("color: #333333; background: transparent;");
    textLayout->addWidget(m_lb_name);

    m_lb_count = new QLabel(this);
    m_lb_count->setFont(QFont("Microsoft YaHei", 11));
    m_lb_count->setStyleSheet("color: #999999; background: transparent;");
    textLayout->addWidget(m_lb_count);

    layout->addLayout(textLayout);
    layout->addStretch();
}

void RoomItem::setRoomInfo(int roomId, QString roomName, int memberCount)
{
    m_roomId = roomId;
    m_roomName = roomName;

    m_lb_name->setText(roomName);
    m_lb_count->setText(QString("%1 人").arg(memberCount));

    // Colored circle avatar
    QStringList colors;
    colors << "#FF6B6B" << "#4ECDC4" << "#45B7D1" << "#96CEB4" << "#FFEAA7"
           << "#DDA0DD" << "#98D8C8" << "#F7DC6F" << "#BB8FCE" << "#85C1E9"
           << "#F8C471" << "#82E0AA" << "#F1948A" << "#73C6B6" << "#D2B4DE"
           << "#E59866" << "#AED6F1" << "#FAD7A0" << "#ABEBC6" << "#C39BD3";
    QString bgColor = colors[roomId % colors.size()];
    QString initial = roomName.isEmpty() ? QString("#") : QString(roomName.at(0));

    m_pb_icon->setStyleSheet(
        QString("QPushButton {"
                "background-color: %1;"
                "color: #FFFFFF;"
                "border: 2px solid %1;"
                "border-radius: 25px;"
                "font-size: 20px;"
                "font-weight: bold;"
                "}").arg(bgColor));
    m_pb_icon->setText(initial);

    // Click handler
    connect(m_pb_icon, &QPushButton::clicked, this, [this]() {
        Q_EMIT sig_clicked(m_roomId);
    });
}

// ========== RoomList ==========
RoomList::RoomList(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RoomList)
{
    ui->setupUi(this);

    // New vertical layout for room items
    m_playout = new QVBoxLayout;
    m_playout->setSpacing(3);
    m_playout->setContentsMargins(0, 0, 0, 0);
    ui->wdg_list->setLayout(m_playout);
}

RoomList::~RoomList()
{
    if (m_playout) {
        delete m_playout;
        m_playout = nullptr;
    }
    delete ui;
}

void RoomList::addRoomItem(int roomId, QString roomName, int memberCount)
{
    qDebug() << __func__ << roomId << roomName << memberCount;

    // Remove old item if exists
    if (m_mapRoomItems.contains(roomId)) {
        QWidget* oldItem = m_mapRoomItems[roomId];
        m_playout->removeWidget(oldItem);
        delete oldItem;
        m_mapRoomItems.remove(roomId);
    }

    RoomItem* item = new RoomItem(this);
    item->setRoomInfo(roomId, roomName, memberCount);

    connect(item, &RoomItem::sig_clicked, this, &RoomList::sig_selectRoom);

    m_playout->addWidget(item);
    m_mapRoomItems[roomId] = item;
}

void RoomList::clearRoomList()
{
    qDebug() << __func__;
    for (auto it = m_mapRoomItems.begin(); it != m_mapRoomItems.end(); ++it) {
        m_playout->removeWidget(it.value());
        delete it.value();
    }
    m_mapRoomItems.clear();
}

void RoomList::on_pb_create_clicked()
{
    qDebug() << __func__;
    Q_EMIT sig_createRoom();
}

void RoomList::on_pb_join_clicked()
{
    qDebug() << __func__;
    Q_EMIT sig_joinRoom();
}

void RoomList::on_pb_refresh_clicked()
{
    qDebug() << __func__;
    Q_EMIT sig_refreshRoomList();
}

void RoomList::on_pb_close_clicked()
{
    qDebug() << __func__;
    this->close();
}
