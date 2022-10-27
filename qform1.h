#ifndef QFORM1_H
#define QFORM1_H

#include <QMainWindow>
#include <QtSerialPort/QSerialPort>
#include <QTimer>
#include "qserialsetup.h"
#include <QInputDialog>
#include <QTime>
#include <QtNetwork/QUdpSocket>
#include <QHostAddress>
#include <QDateTime>

QT_BEGIN_NAMESPACE
namespace Ui { class QForm1; }
QT_END_NAMESPACE

typedef union{
    uint8_t     u8[4];
    int8_t      i8[4];
    uint16_t    u16[2];
    int16_t     i16[2];
    uint32_t    u32;
    int32_t     i32;
    float       f;
} _uWork;

typedef struct{
    uint8_t *buf;
    uint8_t header;
    uint8_t timeout;
    uint8_t nbytes;
    uint8_t cks;
    uint8_t index;
    uint8_t type;
} _sRX;

class QForm1 : public QMainWindow
{
    Q_OBJECT

public:
    QForm1(QWidget *parent = nullptr);
    ~QForm1();

private slots:
    void OnQTimer1();
    void OnRxQSerialPort1();
    void OnRxQUdpSocket1();

    void on_pushButton_3_clicked();

    void on_pushButton_5_clicked();

    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_4_clicked();

private:
    Ui::QForm1 *ui;

    QTimer *QTimer1;
    QSerialPort *QSerialPort1;
    QSerialSetup *QSerialSetup1;
    QUdpSocket *QUdpSocket1;
    QHostAddress remoteHostAddress;
    quint16 remotePort;

    uint8_t rx[256], header, timeout, nbytes, cks, index;
    _uWork w, w1;
    QString strESP01DBG;

    uint8_t bufSerial[256];
    uint8_t bufUdp[256];

    _sRX rxSerial, rxUdp;

    uint16_t ADCBuf[8];

    uint32_t lastMs;


    void SendCmd(uint8_t *buf, uint8_t length, int type);
    void DecodeCmd(uint8_t *RX);
    void DecodeHeader(_sRX *aRX, uint8_t *buf, int length);

};
#endif // QFORM1_H
