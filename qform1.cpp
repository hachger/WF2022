#include "qform1.h"
#include "ui_qform1.h"

QForm1::QForm1(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::QForm1)
{
    ui->setupUi(this);

    QTimer1 = new QTimer(this);
    QSerialPort1 = new QSerialPort(this);
    QSerialSetup1 =  new QSerialSetup(this, QSerialPort1);
    QUdpSocket1 = new QUdpSocket(this);

    connect(QTimer1, &QTimer::timeout, this, &QForm1::OnQTimer1);
    connect(QSerialPort1, &QSerialPort::readyRead, this, &QForm1::OnRxQSerialPort1);
    connect(QUdpSocket1, &QUdpSocket::readyRead, this, &QForm1::OnRxQUdpSocket1);

    header = 0;

    ui->comboBox_2->addItem("ALIVE", 0x00F0);
    ui->comboBox_2->addItem("LAST ADC SAMPLE", 0xA000);
    ui->comboBox_2->addItem("ADC SAMPLEs START", 0xA10A);
    ui->comboBox_2->addItem("ADC SAMPLEs STOP", 0xA100);
    ui->comboBox_2->addItem("ADC SAMPLEs START P", 0xA20A);
    ui->comboBox_2->addItem("ADC SAMPLEs STOP P", 0xA200);
    ui->comboBox_2->addItem("SET EPS01DEBUG", 0x00B0);
    ui->comboBox_2->addItem("SET WIFI", 0x00B1);
    ui->comboBox_2->addItem("SET CONNECT UDP", 0x00B2);

    QTimer1->setInterval(20);
    QTimer1->start();

    rxSerial.buf = bufSerial;
    rxSerial.type = 0;
    rxSerial.header = 0;

    rxUdp.buf = bufUdp;
    rxUdp.type = 1;
    rxUdp.header = 0;
    remotePort = 0;
}

QForm1::~QForm1()
{
    delete ui;
}

void QForm1::OnQTimer1(){
    if(header){
        timeout--;
        if(!timeout)
            header = 0;
    }

}

void QForm1::OnRxQSerialPort1(){
    int count;
    uint8_t *buf;
    QString strHex;

    count = QSerialPort1->bytesAvailable();
    if(count <= 0)
        return;

    buf = new uint8_t[count];
    QSerialPort1->read((char *)buf, count);

    if(ui->checkBox->isChecked()){
        strESP01DBG = "";
        for (int i=0; i<count; i++)
            strESP01DBG = strESP01DBG + QString("%1").arg((char)buf[i]);
        ui->plainTextEdit->appendPlainText(strESP01DBG);

        delete [] buf;
        return;
    }

    strHex = "<-- 0x";
    for (int i=0; i<count; i++) {
        strHex = strHex + QString("%1").arg(buf[i], 2, 16, QChar('0')).toUpper();

        switch(header){
        case 0:
            if(buf[i] == 'U'){
                header = 1;
                timeout = 5;
            }
            else{
                if(buf[i] == '+'){
                    header = 100;
                    timeout = 5;
                }
            }
            break;
        case 1:
            if(buf[i] == 'N')
                header = 2;
            else{
                header = 0;
                i--;
            }
            break;
        case 2:
            if(buf[i] == 'E')
                header = 3;
            else{
                header = 0;
                i--;
            }
            break;
        case 3:
            if(buf[i] == 'R')
                header = 4;
            else{
                header = 0;
                i--;
            }
            break;
        case 4:
            nbytes = buf[i];
            header = 5;
            break;
        case 5:
            if(buf[i] == ':'){
                header = 6;
                index = 0;
                cks = 'U' ^ 'N' ^ 'E' ^ 'R' ^ ':' ^ nbytes;
            }
            else{
                header = 0;
                i--;
            }
            break;
        case 6:
            nbytes--;
            if(nbytes > 0){
                rx[index++] = buf[i];
                cks ^= buf[i];
            }
            else{
                header = 0;
//                DecodeCmd(rx);
                if(cks == buf[i])
                    DecodeCmd(rx);
                else
                    ui->plainTextEdit->appendPlainText("ERROR CHECKSUM");
            }
            break;
        case 100:
            if(buf[i] == '&')
                header = 101;
            else{
                header = 0;
                i--;
            }
            break;
        case 101:
            if(buf[i] == 'D')
                header = 102;
            else{
                header = 0;
                i--;
            }
            break;
        case 102:
            if(buf[i] == 'B')
                header = 103;
            else{
                header = 0;
                i--;
            }
            break;
        case 103:
            if(buf[i] == 'G'){
                header = 104;
                index = 0;
            }
            else{
                header = 0;
                i--;
            }
            break;
        case 104:
            if(buf[i] == '\n'){
                header = 0;
                rx[index] = '\0';
                ui->plainTextEdit->appendPlainText(QString().asprintf("%s", rx));
            }
            else
                rx[index++] = buf[i];
            break;
        default:
            header = 0;
        }
    }

//    ui->plainTextEdit->appendPlainText(strHex);

    delete[] buf;
}

void QForm1::OnRxQUdpSocket1(){
    char *buf;
    int count;

    while (QUdpSocket1->hasPendingDatagrams()) {
        count = QUdpSocket1->pendingDatagramSize();
        if(count <= 0)
            continue;

        buf = new char[count];
        QUdpSocket1->readDatagram(buf, count, &remoteHostAddress, &remotePort);
        ui->lineEdit_2->setText(remoteHostAddress.toString() + ":" + QString().number(remotePort));
        DecodeHeader(&rxUdp, (uint8_t *)buf, count);
        delete [] buf;
    }


}

void QForm1::DecodeHeader(_sRX *aRX, uint8_t *buf, int length){
    for (int i=0; i<length; i++) {
        switch(aRX->header){
        case 0:
            if(buf[i] == 'U'){
                aRX->header = 1;
                aRX->timeout = 5;
            }
            else{
                if(buf[i] == '+'){
                    aRX->header = 100;
                    aRX->timeout = 5;
                }
            }
            break;
        case 1:
            if(buf[i] == 'N')
                aRX->header = 2;
            else{
                aRX->header = 0;
                i--;
            }
            break;
        case 2:
            if(buf[i] == 'E')
                aRX->header = 3;
            else{
                aRX->header = 0;
                i--;
            }
            break;
        case 3:
            if(buf[i] == 'R')
                aRX->header = 4;
            else{
                aRX->header = 0;
                i--;
            }
            break;
        case 4:
            aRX->nbytes = buf[i];
            aRX->header = 5;
            break;
        case 5:
            if(buf[i] == ':'){
                aRX->header = 6;
                aRX->index = 0;
                aRX->cks = 'U' ^ 'N' ^ 'E' ^ 'R' ^ ':' ^ aRX->nbytes;
            }
            else{
                aRX->header = 0;
                i--;
            }
            break;
        case 6:
            aRX->nbytes--;
            if(aRX->nbytes > 0){
                aRX->buf[index++] = buf[i];
                aRX->cks ^= buf[i];
            }
            else{
                aRX->header = 0;
//                DecodeCmd(rx);
                if(cks == buf[i])
                    DecodeCmd(aRX->buf);
                else
                    ui->plainTextEdit->appendPlainText("ERROR CHECKSUM");
            }
            break;
        case 100:
            if(buf[i] == '&')
                aRX->header = 101;
            else{
                aRX->header = 0;
                i--;
            }
            break;
        case 101:
            if(buf[i] == 'D')
                aRX->header = 102;
            else{
                aRX->header = 0;
                i--;
            }
            break;
        case 102:
            if(buf[i] == 'B')
                aRX->header = 103;
            else{
                aRX->header = 0;
                i--;
            }
            break;
        case 103:
            if(buf[i] == 'G'){
                aRX->header = 104;
                aRX->index = 0;
            }
            else{
                aRX->header = 0;
                i--;
            }
            break;
        case 104:
            if(buf[i] == '\n'){
                aRX->header = 0;
                aRX->buf[aRX->index] = '\0';
                ui->plainTextEdit->appendPlainText(QString().asprintf("%s", rx));
            }
            else
                aRX->buf[aRX->index++] = buf[i];
            break;
        default:
            aRX->header = 0;
        }
    }
}

void QForm1::SendCmd(uint8_t *buf, uint8_t length, int type){
    uint8_t tx[24];
    uint8_t cks, i;
    QString strHex;

    if(type == 0){
        if(!QSerialPort1->isOpen())
            return;
    }

    if(type == 1){
        if(!QUdpSocket1->isOpen() || remotePort==0)
            return;
    }

    tx[0] = 'U';
    tx[1] = 'N';
    tx[2] = 'E';
    tx[3] = 'R';
    tx[4] = length + 1;
    tx[5] = ':';

    memcpy(&tx[6], buf, length);

    cks = 0;
    for (i=0; i<(length+6); i++) {
        cks ^= tx[i];
    }

    tx[i] = cks;

    strHex = "--> 0x";
    for (int i=0; i<length+7; i++) {
        strHex = strHex + QString("%1").arg(tx[i], 2, 16, QChar('0')).toUpper();
    }

    ui->plainTextEdit->appendPlainText(strHex);


    QSerialPort1->write((char *)tx, length+7);
}

void QForm1::DecodeCmd(uint8_t *RX){
    QString str;

    switch (RX[0]) {
    case 0xA0:
        memcpy(ADCBuf, &RX[1], 16);
        str = QString("%1").arg(ADCBuf[1], 4, 10, QChar('0'));
        ui->lcdNumber->display(str);
        str = QString("%1").arg(ADCBuf[2], 4, 10, QChar('0'));
        ui->lcdNumber_2->display(str);
        str = QString("%1").arg(ADCBuf[0], 4, 10, QChar('0'));
        ui->lcdNumber_3->display(str);
        str = QString("%1").arg(ADCBuf[5], 4, 10, QChar('0'));
        ui->lcdNumber_4->display(str);
        str = QString("%1").arg(ADCBuf[6], 4, 10, QChar('0'));
        ui->lcdNumber_5->display(str);
        str = QString("%1").arg(ADCBuf[4], 4, 10, QChar('0'));
        ui->lcdNumber_6->display(str);
        str = QString("%1").arg(ADCBuf[3], 4, 10, QChar('0'));
        ui->lcdNumber_7->display(str);
        str = QString("%1").arg(ADCBuf[7], 4, 10, QChar('0'));
        ui->lcdNumber_8->display(str);
        memcpy(ADCBuf, &RX[1+16], 16);
        str = "";
        for (int i = 0; i < 8; i++) {
            str = str + QString("ADC%1: ").arg(i);
            str = str + QString("%1\n").arg(ADCBuf[i], 4, 10, QChar('0'));
        }
        ui->plainTextEdit->appendPlainText(str);
        break;
    case 0xA1:
        if(RX[1] == 0x0D)
            ui->plainTextEdit->appendPlainText("OK 0xA1 START/STOP");
        if(RX[1] == 0x0A){
            w1.u32 = QTime().currentTime().msec();
            w.u32 = w1.u32 - lastMs;
            lastMs = w1.u32;
            ui->label_11->setText(QString().number(w.u32));
            memcpy(ADCBuf, &RX[3+RX[2]*16], 16);
            str = QString("%1").arg(ADCBuf[1], 4, 10, QChar('0'));//L90
            ui->lcdNumber->display(str);
            str = QString("%1").arg(ADCBuf[2], 4, 10, QChar('0'));//L45
            ui->lcdNumber_2->display(str);
            str = QString("%1").arg(ADCBuf[0], 4, 10, QChar('0'));//L0
            ui->lcdNumber_3->display(str);
            str = QString("%1").arg(ADCBuf[5], 4, 10, QChar('0'));//R0
            ui->lcdNumber_4->display(str);
            str = QString("%1").arg(ADCBuf[6], 4, 10, QChar('0'));//R45
            ui->lcdNumber_5->display(str);
            str = QString("%1").arg(ADCBuf[4], 4, 10, QChar('0'));//R90
            ui->lcdNumber_6->display(str);
            str = QString("%1").arg(ADCBuf[3], 4, 10, QChar('0'));//BF
            ui->lcdNumber_7->display(str);
            str = QString("%1").arg(ADCBuf[7], 4, 10, QChar('0'));//BR
            ui->lcdNumber_8->display(str);
        }
        break;
    case 0xA2:
        if(RX[1] == 0x0D)
            ui->plainTextEdit->appendPlainText("OK 0xA2 START/STOP");
        if(RX[1] == 0x0A){
            w1.u32 = QTime().currentTime().msec();
            w.u32 = w1.u32 - lastMs;
            lastMs = w1.u32;
            ui->label_11->setText(QString().number(w.u32));
            memcpy(ADCBuf, &RX[2], 16);
            str = QString("%1").arg(ADCBuf[1], 4, 10, QChar('0'));//L90
            ui->lcdNumber->display(str);
            str = QString("%1").arg(ADCBuf[2], 4, 10, QChar('0'));//L45
            ui->lcdNumber_2->display(str);
            str = QString("%1").arg(ADCBuf[0], 4, 10, QChar('0'));//L0
            ui->lcdNumber_3->display(str);
            str = QString("%1").arg(ADCBuf[5], 4, 10, QChar('0'));//R0
            ui->lcdNumber_4->display(str);
            str = QString("%1").arg(ADCBuf[6], 4, 10, QChar('0'));//R45
            ui->lcdNumber_5->display(str);
            str = QString("%1").arg(ADCBuf[4], 4, 10, QChar('0'));//R90
            ui->lcdNumber_6->display(str);
            str = QString("%1").arg(ADCBuf[3], 4, 10, QChar('0'));//BF
            ui->lcdNumber_7->display(str);
            str = QString("%1").arg(ADCBuf[7], 4, 10, QChar('0'));//BR
            ui->lcdNumber_8->display(str);
        }
        break;
    case 0xF0:
        if(RX[1] == 0x0D)
            ui->plainTextEdit->appendPlainText("I'M ALIVE");
        if(RX[1] == 0x0E)
            ui->label_13->setText(QDateTime().currentDateTime().toString("hh:mm:ss dd/MM/yyyy"));
        if(RX[1] == 0x0F)
            ui->label_12->setText(QDateTime().currentDateTime().toString("hh:mm:ss dd/MM/yyyy"));
        break;
    case 0xB1:
        strncpy(str.toUtf8().data(), (char *)&RX[1], 48);
        ui->plainTextEdit->appendPlainText("SSID: " + str);
        strncpy(str.toUtf8().data(), (char *)&RX[1+48], 32);
        ui->plainTextEdit->appendPlainText("PASSWORD: " + str);
        break;
    case 0xB:
        break;
    case 0xFF:
        ui->plainTextEdit->appendPlainText("NO CMD");
        break;
    }
}



void QForm1::on_pushButton_3_clicked()
{
    _uWork cmd;
    uint8_t buf[64];
    int n, iStr;
    QString aux;
    bool ok;



    cmd.u32 = 0;
    if(ui->comboBox_2->currentIndex() != -1){
        if(ui->comboBox_2->findText(ui->comboBox_2->currentText()) != -1){
            cmd.u16[0] = ui->comboBox_2->currentData().toInt();
            ui->plainTextEdit->appendPlainText(ui->comboBox_2->currentText());
        }
        else{
            if(ui->comboBox_2->currentText()!="" && ui->checkBox->isChecked()){
                int i;
                for (i = 0; i < ui->comboBox_2->currentText().length(); i++) {
                    buf[i] = ui->comboBox_2->currentText().toUtf8().data()[i];
                }
                buf[i++] = '\r';
                buf[i++] = '\n';

                QSerialPort1->write((char *)buf, i);

                ui->plainTextEdit->appendPlainText(ui->comboBox_2->currentText());

            }

            return;
        }
    }

    n = 0;
    switch(cmd.u16[0]){
    case 0x00F0:
    case 0x00B0:
    case 0xA000:
    case 0xA100:
    case 0xA200:
            n=1;
        break;
    case 0xA10A:
        w.u8[0] = QInputDialog::getInt(this, "RATE ms", "RATE: ", 10, 10, 60, 10, &ok);
        if(!ok)
            break;
        cmd.u8[0] = w.u8[0];
        n = 1;
        break;
    case 0xA20A:
        w.u8[0] = QInputDialog::getInt(this, "RATE ms", "RATE: ", 10, 10, 255, 1, &ok);
        if(!ok)
            break;
        cmd.u8[0] = w.u8[0];
        n = 1;
        break;
    case 0x00B1://SET WIFI
        aux = QInputDialog::getText(this, "SET WIFI", "SSID:", QLineEdit::Normal, "", &ok);
        iStr = 1;
        if(!ok)
            break;
        for(int i=0; i<aux.length(); i++){
            buf[iStr++] = aux.toUtf8().data()[i];
        }
        buf[iStr++] = '\0';
        aux = QInputDialog::getText(this, "SET WIFI", "PASSWORD:", QLineEdit::Normal, "", &ok);
        if(!ok)
            break;
        for(int i=0; i<aux.length(); i++){
            buf[iStr++] = aux.toUtf8().data()[i];
        }
        buf[iStr++] = '\0';
        n = iStr;
        break;
    case 0x00B2:
        break;
    }

    if(cmd.u8[1] == 0)
        buf[0] = cmd.u8[0];
    else {
        buf[0] = cmd.u8[1];
        buf[1] = cmd.u8[0];
        n++;
    }

    if(n)
        SendCmd(buf, n, 0);
}


void QForm1::on_pushButton_5_clicked()
{
    QSerialPort1->close();
    ui->pushButton->setText("OPEN");

    QSerialSetup1->getAvailablePorts();
    QSerialSetup1->exec();

    if(QSerialPort1->portName() != "")
        ui->pushButton_5->setText(QSerialPort1->portName());
    else
        ui->pushButton_5->setText("COM?");

    ui->statusbar->showMessage(QSerialSetup1->getSerialPortConfig());
}


void QForm1::on_pushButton_clicked()
{
    if(QSerialPort1->isOpen()){
        QSerialPort1->close();
        ui->pushButton->setText("OPEN");
    }
    else{
        if(ui->pushButton_5->text() == "COM?")
            return;
        if(QSerialPort1->open(QSerialPort::ReadWrite)){
            ui->pushButton->setText("CLOSE");
        }
        else{
            ui->plainTextEdit->appendPlainText("CAN'T OPEN PORT");
        }
    }
}


void QForm1::on_pushButton_2_clicked()
{
    if(QUdpSocket1->isOpen()){
        QUdpSocket1->abort();
        QUdpSocket1->close();
        ui->pushButton_2->setText("OPEN");
        remotePort = 0;
        ui->lineEdit_2->setText("");
    }
    else{
        bool ok;
        quint16 port;

        port = ui->lineEdit->text().toInt(&ok, 10);
        if(!ok){
            ui->plainTextEdit->appendPlainText("WRONG PORT number.");
            return;
        }

        if(!QUdpSocket1->bind(port)){
            ui->plainTextEdit->appendPlainText("CAN'T bind PORT");
            return;
        }
        if(!QUdpSocket1->open(QUdpSocket::ReadWrite)){
            ui->plainTextEdit->appendPlainText("CAN'T open PORT");
            return;
        }

        ui->pushButton_2->setText("CLOSE");
    }
}


void QForm1::on_pushButton_4_clicked()
{
    _uWork cmd;
    uint8_t buf[64];
    int n;
    bool ok;



    cmd.u32 = 0;
    if(ui->comboBox_2->currentIndex() != -1){
        if(ui->comboBox_2->findText(ui->comboBox_2->currentText()) != -1){
            cmd.u16[0] = ui->comboBox_2->currentData().toInt();
            ui->plainTextEdit->appendPlainText(ui->comboBox_2->currentText());
        }
        else{
            if(ui->comboBox_2->currentText()!="" && ui->checkBox->isChecked()){
                int i;
                for (i = 0; i < ui->comboBox_2->currentText().length(); i++) {
                    buf[i] = ui->comboBox_2->currentText().toUtf8().data()[i];
                }
                buf[i++] = '\r';
                buf[i++] = '\n';

                QSerialPort1->write((char *)buf, i);

                ui->plainTextEdit->appendPlainText(ui->comboBox_2->currentText());

            }

            return;
        }
    }

    n = 0;
    switch(cmd.u16[0]){
    case 0x00F0:
    case 0x00B0:
    case 0xA000:
    case 0xA100:
    case 0xA200:
            n=1;
        break;
    case 0xA10A:
        w.u8[0] = QInputDialog::getInt(this, "RATE ms", "RATE: ", 10, 10, 60, 10, &ok);
        if(!ok)
            break;
        cmd.u8[0] = w.u8[0];
        n = 1;
        break;
    case 0xA20A:
        w.u8[0] = QInputDialog::getInt(this, "RATE ms", "RATE: ", 10, 10, 255, 1, &ok);
        if(!ok)
            break;
        cmd.u8[0] = w.u8[0];
        n = 1;
        break;
    }

    if(cmd.u8[1] == 0)
        buf[0] = cmd.u8[0];
    else {
        buf[0] = cmd.u8[1];
        buf[1] = cmd.u8[0];
        n++;
    }

    if(n)
        SendCmd(buf, n, 1);

}

