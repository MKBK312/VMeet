#include "customtitle.h"
#include "ui_customtitle.h"

#include<QMessageBox>
#include<QMouseEvent>


CustomTitle::CustomTitle(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CustomTitle)
{
    ui->setupUi(this);

    this->parentWidget()->setWindowFlag(Qt::FramelessWindowHint);
    m_mousePressed = false;
}

CustomTitle::~CustomTitle()
{
    delete ui;
}

void CustomTitle::setTitle(QString &title)
{
    ui->lb_title->setText(title );
}

void CustomTitle::setTitle(QIcon &icon, QString &title)
{
    ui->pb_icon->setIcon(icon);
    ui->lb_title->setText(title );
}



void CustomTitle::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
            m_mousePressed = true;
            mousePoint = event->globalPos() - /*this*/this->parentWidget()->pos();
            event->accept();
    }
    return QWidget::mousePressEvent(event);
}

void CustomTitle::mouseMoveEvent(QMouseEvent *event)
{
    if (m_mousePressed && (event->buttons() && Qt::LeftButton))
    {
//        this->move(event->globalPos() - mousePoint  );
        this->parentWidget()->move(event->globalPos() - mousePoint  );
        event->accept();
    }
    return QWidget::mouseMoveEvent(event);
}

void CustomTitle::mouseReleaseEvent(QMouseEvent *event)
{
    m_mousePressed = false;
    return QWidget::mouseReleaseEvent(event);
}

void CustomTitle::closeEvent(QCloseEvent *event)
{
    event->accept();
    Q_EMIT SIG_close();

    QWidget::closeEvent(event);
}


void CustomTitle::on_pb_min_clicked()
{
    //最小化
    this->parentWidget()->showMinimized();
}


void CustomTitle::on_pb_max_clicked()
{
    //最大化
    if (this->parentWidget()->isMaximized())
    {
        this->parentWidget()->showNormal();
    }
    else
    {
        this->parentWidget()->showMaximized();
    }
}


void CustomTitle::on_pb_close_clicked()
{
    if( QMessageBox::question( this , "退出提示","确定退出?" ) == QMessageBox::Yes )
    {
         this->parentWidget()->close();
    }
}

