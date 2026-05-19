#ifndef CUSTOMTITLE_H
#define CUSTOMTITLE_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class CustomTitle; }
QT_END_NAMESPACE

class QMouseEvent;
class QIcon;

class CustomTitle : public QWidget
{
    Q_OBJECT
signals:
    void SIG_close();
public:
    CustomTitle(QWidget *parent = nullptr);
    ~CustomTitle();

    void setTitle( QString& title );
    void setTitle( QIcon& icon, QString& title );

protected:
    QPoint mousePoint;
    bool m_mousePressed;

    void mouseMoveEvent(QMouseEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);

    void closeEvent(QCloseEvent *event);
private slots:
    void on_pb_min_clicked();

    void on_pb_max_clicked();

    void on_pb_close_clicked();

private:
    Ui::CustomTitle *ui;
};
#endif // CUSTOMTITLE_H
